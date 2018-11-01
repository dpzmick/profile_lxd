#include "disk.h"
#include "disk_thread.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

struct disk_thread {
  /* shared between main thread and background thread */
  pthread_t          t;
  bool               thread_valid;
  atomic_bool        flush;

  /* stuff only accessed from the thread */
  jack_ringbuffer_t* read_ring;

  /* pointers into trailing regions */
  sample_set_t* sample_set;

  /* trailing preallocated sample_set */
};

static void*
thread(void* arg)
{
  disk_thread_t* dt = (disk_thread_t*)arg;
  assert(dt->thread_valid); /* this would be weird */

  while (true) {
    bool flushing = atomic_load(&dt->flush);
    size_t r = jack_ringbuffer_read(dt->read_ring, (char*)dt->sample_set, 4096*4);
    if (r) printf("Read %zu bytes from rb, flushing=%d\n", r, flushing);
    if (flushing && r == 0) break;
    usleep(10000);
  }

  return NULL;
}

size_t
disk_thread_footprint(void)
{
  size_t footprint = sizeof(disk_thread_t);
  footprint += SAMPLE_SET_MAX; /* not aligning */
  return footprint;
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

  disk_thread_t* dt = (disk_thread_t*)mem;
  dt->t             = 0; /* no portable way to init */
  dt->thread_valid  = false;
  /* flush follows */
  dt->read_ring     = read_ring;
  dt->sample_set    = (sample_set_t*)((char*)mem + sizeof(disk_thread_t));

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

  int ret = pthread_create(&dt->t, NULL, thread, dt);
  if (0 != ret) {
    if (ret == EINVAL) return APP_ERR_INVAL;
    else               return APP_ERR_ALLOC;
  }

  dt->thread_valid = true;
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
  else          return APP_SUCCESS;
}
