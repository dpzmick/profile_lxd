#include "envelope.h"

#include "common.h"
#include "err.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

/* This is audio-land, define a 'reasonable zero' that we will hard round to zero.
   Number picked for no good reason other than trying to get the tests to do a
   reasonable thing. Not sure if totally sane.. */

#define REASONABLE_ZERO_VALUE 1e-8

int
populate_envelope_setting(int                 type,
                          uint64_t            decay_time,
                          uint64_t            sample_rate_hz,
                          envelope_setting_t* out_setting)
{
  double nsec_per_frame = (1e9/(double)sample_rate_hz);
  double t              = MAX(1, (double)decay_time/nsec_per_frame);

  out_setting->type = type;
  switch (type) {
    case ENVELOPE_LINEAR: {
      // 0 = 1 - tm
      out_setting->u.linear->m = 1. / t;
      break;
    }
    case ENVELOPE_EXPONENTIAL: {
      // we can't ever reach zero, but, we can reach reasonable zero
      // REASONABLE_ZERO_VALUE = e^(mt)
      // ln(RZV) = mt * ln(e)
      // ln(RZV) = mt
      // m = ln(RZV) / t
      out_setting->u.exponential->lambda = log(REASONABLE_ZERO_VALUE) / t;
      break;
    }
    case ENVELOPE_LOGARITHMIC: {
      // RZV = 1.0 - ln(t)/m
      // ln(t)/m = 1.0 - RZV
      // m = ln(t)/(1.0 - RZV)

      // when the pulse is shorter than one sample, we don't want to return zero
      // (will result in divide by zero later on). Instead return something that
      // ensures we will always return zeros.
      float tmp = log(t)/(1.0 - REASONABLE_ZERO_VALUE);
      out_setting->u.logarithmic->m = MAX(REASONABLE_ZERO_VALUE, tmp);
      break;
    }
    default: return APP_ERR_INVAL;
  }
  return APP_SUCCESS;
}

struct envelope {
  uint32_t           n_samples;
  envelope_setting_t setting[1];
};

size_t
envelope_footprint(void)
{
  return sizeof(envelope_t);
}

size_t
envelope_align(void)
{
  return 1;
}

envelope_t*
create_envelope(void*                     mem,
                envelope_setting_t const* initial_setting,
                int*                      opt_err)
{
  if (opt_err) *opt_err = APP_SUCCESS;

  // FIXME check alignment of mem
  envelope_t* ret = (envelope_t*)mem;
  *(ret->setting) = *initial_setting;
  ret->n_samples = 0;
  return ret;
}

void*
destroy_envelope(envelope_t* e)
{
  return (void*)e;
}

int
envelope_strike(envelope_t* e)
{
  e->n_samples = 0;
  return APP_SUCCESS;
}

int
envelope_zero(envelope_t* e)
{
  // at "infinity", all of our different envelopes are at zero
  e->n_samples = UINT32_MAX;
  return APP_SUCCESS;
}

int
envelope_change_setting(envelope_t*               e,
                        envelope_setting_t const* setting)
{
  *(e->setting) = *setting;
  return APP_SUCCESS;
}

int
envelope_generate_samples(envelope_t* e,
                          size_t      nframes,
                          float*      buffer)
{
  size_t previous_samples = e->n_samples;
  size_t final_samples    = previous_samples + nframes;
  int    type             = e->setting->type;

  /* Use a closed form of the decay equation given to try and prevent error
     from accumulating */

  for (size_t i = 0; i < nframes; ++i) {
    size_t const this_sample = previous_samples + i;

    switch (type) {
      case ENVELOPE_CONSTANT: {
        buffer[i] = e->setting->u.constant->value;
        break;
      }
      case ENVELOPE_LINEAR: {
        buffer[i] = 1.0 - e->setting->u.linear->m * this_sample;
        break;
      }
      case ENVELOPE_EXPONENTIAL: {
        float lambda = e->setting->u.exponential->lambda;
        buffer[i] = exp(lambda * this_sample);
        break;
      }
      case ENVELOPE_LOGARITHMIC: {
        float m = e->setting->u.logarithmic->m;
        buffer[i] = 1.0 - log(this_sample)/(double)m;
        break;
      }
      default: BUG(true, "unhandled envelope type");
    }

    if (buffer[i] < REASONABLE_ZERO_VALUE) buffer[i] = 0.0;
  }

#ifndef NDEBUG
  for (size_t i = 0; i < nframes; ++i) assert(!isnan(buffer[i]));
#endif

  if (final_samples < previous_samples) {
    // overflow, well defined for unsigned
    e->n_samples = UINT32_MAX;
  }
  else {
    e->n_samples = final_samples;
  }

  return APP_SUCCESS;
}
