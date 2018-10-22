#pragma once

#include <stddef.h>

typedef struct pulse_gen pulse_gen_t;

size_t
pulse_gen_footprint(void);

size_t
pulse_gen_align(void);

/* Create a pulse generator. The pulse generator is stateful exponentially
   decaying envelope */

pulse_gen_t*
create_pulse_gen(void*  mem,
                 float  lambda, /* FIXME units */
                 int*   opt_err);

void*
destroy_pulse_gen(pulse_gen_t* pgen);

/* Strike the pulse generator. A strike will always start a new decaying note. */
int
pulse_gen_strike(pulse_gen_t* pgen);

/* Stick nframes into the buffer provided. If the generator needs to be struck,
   it should be struck between calls to the sample generator */

int
pulse_gen_generate_samples(pulse_gen_t* pgen,
                           size_t       nframes,
                           float*       buffer);
