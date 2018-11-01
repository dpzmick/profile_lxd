#pragma once

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*arr))
#define ALIGN(x, align) ((((x) + ((align) - 1)) / (align)) * (align))

#define OUTPUT_DATA_FILE "/scratch/data_out"
#define RINGBUFFER_SIZE  (4096ul*16ul)
