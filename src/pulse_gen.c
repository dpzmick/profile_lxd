#include "err.h"
#include "pulse_gen.h"

#include <float.h>
#include <math.h>
#include <string.h>

struct pulse_gen {
  float lambda;
  float state;
};

size_t
pulse_gen_footprint(void)
{
  return sizeof(pulse_gen_t);
}

/* can be anywhere */

size_t
pulse_gen_align(void)
{
  return 1;
}

pulse_gen_t*
create_pulse_gen(void* mem,
                 float lambda,
                 int*  opt_err)
{
  if (opt_err) *opt_err = APP_SUCCESS;

  // FIXME check alignment of mem
  pulse_gen_t* ret = (pulse_gen_t*)mem;
  ret->lambda = lambda;
  ret->state = 0.0;
  return ret;
}

void*
destroy_pulse_gen(pulse_gen_t* pgen)
{
  return (void*)pgen;
}

int
pulse_gen_strike(pulse_gen_t* pgen)
{
  pgen->state = 1.0;
  return APP_SUCCESS;
}

int
pulse_gen_generate_samples(pulse_gen_t* pgen,
                           size_t       nframes,
                           float*       buffer)
{
  memset(buffer, 0, nframes*sizeof(*buffer));
  if (pgen->state < FLT_EPSILON) {
    pgen->state = 0.0;
    return APP_SUCCESS;
  }

  for (size_t i = 0; i < nframes; ++i) {
    buffer[i] = pgen->state;
    pgen->state += -pgen->lambda * pgen->state;
    if (pgen->state < FLT_EPSILON) {
      pgen->state = 0.0;
      return APP_SUCCESS;
    }
  }
  return APP_SUCCESS;
}
