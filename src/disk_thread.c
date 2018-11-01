#include "common.h"
#include "disk.h"
#include "disk_thread.h"

#include <assert.h>
#include <errno.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct disk_thread {
  /* shared between main thread and background thread */
  pthread_t          t;
  bool               thread_valid;
  atomic_bool        flush;

  /* stuff only accessed from the thread */
  jack_ringbuffer_t* read_ring;
  int                fd;
  char               buffer[4096*4]; // FIXME size?
};

static void*
thread(void* arg)
{
  disk_thread_t* dt = (disk_thread_t*)arg;
  assert(dt->thread_valid); /* this would be weird */
  int ret = pthread_setname_np(dt->t, "profile_lxd:dsk");
  if (0 != ret) {
    fprintf(stderr, "couldn't set thread name, why=\n");
  }

  while (true) {
    bool flushing = atomic_load(&dt->flush);
    size_t r = jack_ringbuffer_read(dt->read_ring, dt->buffer, sizeof(dt->buffer));
    if (flushing && r == 0) break;

    char*  ptr  = dt->buffer;
    size_t left = r;
    while (left) {
      ssize_t ret = write(dt->fd, ptr, left);
      if (-1 == ret) {
        fprintf(stderr, "Write failed with '%s'\n", strerror(errno));
        /* FIXME main thread should watchdog the other threads */
        return NULL;
      }

      assert((size_t)ret <= left);
      left -= (size_t)ret;
    }

    usleep(1000);
  }

  return NULL;
}

size_t
disk_thread_footprint(void)
{
  return sizeof(disk_thread_t);
}

size_t
disk_thread_align(void)
{
  return 1;
}

disk_thread_t*
create_disk_thread(void*              mem,
                   jack_ringbuffer_t* read_ring,
                   int*               opt_err)
{
  /* FIXME check alignment */

  int fd = open(OUTPUT_DATA_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (-1 == fd) {
    if (opt_err) *opt_err = APP_ERR_OPEN;
    fprintf(stderr, "Failed to open file with '%s'", strerror(errno));
    return NULL;
  }

  disk_thread_t* dt = (disk_thread_t*)mem;
  dt->t             = 0; /* no portable way to init */
  dt->thread_valid  = false;
  /* flush follows */
  dt->read_ring     = read_ring;
  dt->fd            = fd;
  memset(dt->buffer, 0, sizeof(dt->buffer)); /* why not */
  atomic_store(&dt->flush, false);

  if (opt_err) *opt_err = APP_SUCCESS;
  return dt;
}

void*
destroy_disk_thread(disk_thread_t* dt)
{
  if (!dt) return NULL;
  assert(!dt->thread_valid);

  /* doesn't actually do anything */
  return (void*)dt;
}

int
disk_thread_start(disk_thread_t* dt)
{
  if (!dt) return APP_ERR_INVAL;

  dt->thread_valid = true;
  int ret = pthread_create(&dt->t, NULL, thread, dt);
  if (0 != ret) {
    dt->thread_valid = false;
    if (ret == EINVAL) return APP_ERR_INVAL;
    else               return APP_ERR_ALLOC;
  }

  return APP_SUCCESS;
}

int
disk_thread_flush_and_stop(disk_thread_t* dt)
{
  if (!dt)               return APP_ERR_INVAL;
  if (!dt->thread_valid) return APP_ERR_INVAL;

  atomic_store(&dt->flush, true);

  int ret = pthread_join(dt->t, NULL);
  dt->thread_valid = false;
  if (0 != ret) return APP_ERR_THREAD_JOIN;

  ret = close(dt->fd);
  if (-1 == ret) {
    /* not recoverable */
    fprintf(stderr, "close failed with '%s'\n", strerror(errno));
  }

  return APP_SUCCESS;
}
