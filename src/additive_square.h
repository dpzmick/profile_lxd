#pragma once

#include <stddef.h>

/* Inefficient, but highly accurate, additive square wave generator.

   This generator adds successive sine waves up until the nyquist frequency */

/* Opaque? */
typedef struct additive_square additive_square_t;

/* Return the size of the wave generator in bytes */

size_t
additive_square_footprint(void);

/* Return required alignment in bytes for the first byte of the structure */

size_t
additive_square_align(void);

/* Create a square wave generator in the appropriately sized memory region */

additive_square_t*
create_additive_square(void*  mem,
                       size_t sample_rate_hz,
                       int*   opt_err);

/* delete it */

void*
destroy_additive_square(additive_square_t* square);

/* Put n frames into the provided buffer of floats with the given frequency */

int
additive_square_generate_samples(additive_square_t* square,
                                 size_t             n_frames,
                                 float              frequency_hz,
                                 float*             out_buffer);
