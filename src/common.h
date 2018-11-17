#pragma once

#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*arr))
#define ALIGN(x, align) ((((x) + ((align) - 1)) / (align)) * (align))
#define BUG(cond, ...) do {                                                     \
  if (!(cond)) {                                                                \
    fprintf(stderr, "Bug! !(%s) at %s:%d.", "" # cond, __FILE__, __LINE__);     \
    fprintf(stderr, __VA_ARGS__);                                               \
    abort();                                                                    \
  }                                                                             \
} while (0);

// config stuff

#define OUTPUT_DATA_FILE "/scratch/data_out"
#define RINGBUFFER_SIZE  (4096ul*16ul)
