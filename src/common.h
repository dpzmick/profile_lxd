#pragma once

#include <stdio.h>
#include <stdlib.h>

// !!(x) because the first arg must be an int
#define UNLIKELY(cond) __builtin_expect(!!(cond), 0)
#define LIKELY(cond) __builtin_expect(!!(cond), 1)

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*arr))
#define ALIGN(x, align) ((((x) + ((align) - 1)) / (align)) * (align))
#define BUG(cond, ...) do {                                                     \
  if (UNLIKELY(cond)) {                                                         \
    fprintf(stderr, "Bug! !(%s) at %s:%d.", "" # cond, __FILE__, __LINE__);     \
    fprintf(stderr, __VA_ARGS__);                                               \
    abort();                                                                    \
  }                                                                             \
} while (0);

// config stuff

#define OUTPUT_DATA_FILE "/scratch/data_out"
#define RINGBUFFER_SIZE  (4096ul*16ul)
