#include "envelope.h"

#include "err.h"

#include <float.h>
#include <math.h>
#include <string.h>

struct envelope {
  float              state;
  envelope_setting_t setting[1];
  // store a jump table on the type or something?
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

  // FIXME
  if (initial_setting->type != ENVELOPE_EXPONENTIAL) {
    if (opt_err) *opt_err = APP_ERR_INVAL;
    return NULL;
  }

  // FIXME check alignment of mem
  envelope_t* ret = (envelope_t*)mem;
  *(ret->setting) = *initial_setting;
  ret->state = 0.0;
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
  e->state = 1.0;
  return APP_SUCCESS;
}

int
envelope_zero(envelope_t* e)
{
  e->state = 0.0;
  return APP_SUCCESS;
}

int
envelope_change_setting(envelope_t*               e,
                        envelope_setting_t const* setting)
{
  if (setting->type != ENVELOPE_EXPONENTIAL) return APP_ERR_INVAL; // FIXME
  *(e->setting) = *setting;
  return APP_SUCCESS;
}

int
envelope_generate_samples(envelope_t* e,
                          size_t      nframes,
                          float*      buffer)
{
  memset(buffer, 0, nframes*sizeof(*buffer));
  if (e->state < FLT_EPSILON) {
    e->state = 0.0;
    return APP_SUCCESS;
  }

  for (size_t i = 0; i < nframes; ++i) {
    buffer[i] = e->state;
    e->state += -(e->setting->u.exponential->lambda) * e->state;
    if (e->state < FLT_EPSILON) {
      e->state = 0.0;
      return APP_SUCCESS;
    }
  }
  return APP_SUCCESS;
}
