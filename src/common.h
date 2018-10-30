#pragma once

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*arr))
#define ALIGN(x, align) ((((x) + ((align) - 1)) / (align)) * (align))
