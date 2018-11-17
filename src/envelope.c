#include "envelope.h"

#include "common.h"
#include "err.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

struct envelope {
  float              state;
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
  if (e->setting->type == ENVELOPE_CONSTANT) e->state = e->setting->u.constant->value;
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
  *(e->setting) = *setting;
  return APP_SUCCESS;
}

// FIXME determine if early return is worth adding noise to the
// code and inconsistent processing time.
int
envelope_generate_samples(envelope_t* e,
                          size_t      nframes,
                          float*      buffer)
{
  memset(buffer, 0, nframes*sizeof(*buffer));

  /* If the current state is at zero, the buffer is already fully populated.
     User can't call `strike` in the middle of processing, so bail out. */

  if (e->state < FLT_EPSILON) {
    e->state = 0.0;
    return APP_SUCCESS;
  }

  for (size_t i = 0; i < nframes; ++i) {
    buffer[i] = e->state;
    switch (e->setting->type) {
      case ENVELOPE_CONSTANT: {
        e->state = e->setting->u.constant->value;
        break;
      }
      case ENVELOPE_EXPONENTIAL: {
        e->state += -(e->setting->u.exponential->lambda) * e->state;
        break;
      }
      case ENVELOPE_LINEAR: {
        e->state -= e->setting->u.linear->m;
        break;
      }
      case ENVELOPE_LOGARITHMIC: {
        e->state += -1./(e->setting->u.logarithmic->m * e->state);
        break;
      }
      default: BUG(true, "unhandled envelope type");
    }

    /* Similarly, if we've reached zero, the remaining buffer is already fully populated. */
    if (e->state < FLT_EPSILON) {
      e->state = 0.0;
      return APP_SUCCESS;
    }
  }

  return APP_SUCCESS;
}
