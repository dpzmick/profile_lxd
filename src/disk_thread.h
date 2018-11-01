#pragma once

#include <jack/ringbuffer.h>

typedef struct disk_thread disk_thread_t;

size_t
disk_thread_footprint(void);

size_t
disk_thread_align(void);

disk_thread_t*
create_disk_thread(void*              mem,
                   jack_ringbuffer_t* read_ring,
                   int*               opt_err);

void*
destroy_disk_thread(disk_thread_t* dt);

int
disk_thread_start(disk_thread_t* dt);

int
disk_thread_flush_and_stop(disk_thread_t* dt);
