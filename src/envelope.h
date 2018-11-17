#pragma once

#include <stddef.h>

typedef struct envelope         envelope_t;
typedef struct envelope_setting envelope_setting_t;

enum {
  ENVELOPE_CONSTANT = 0,
  ENVELOPE_EXPONENTIAL,
  ENVELOPE_LINEAR,
  ENVELOPE_LOGARITHMIC,
};

struct envelope_setting {
  int type;

  // FIXME need better conversion from these unitless things to times

  union {
    struct {
      float value;
    } constant[1];

    struct {
      float lambda;
    } exponential[1];

    struct {
      float m;
    } linear[1];

    struct {
      float m;
    } logarithmic[1];
  } u;
};

size_t
envelope_footprint(void);

size_t
envelope_align(void);

/* Create an envelope. Any values needed from the settings are copied in this
   function. */

envelope_t*
create_envelope(void*                     mem,
                envelope_setting_t const* initial_settings,
                int*                      opt_err);

void*
destroy_envelope(envelope_t* e);

/* Increase the current value of the envelope to '1.0' and decay according to
   the current settings */

int
envelope_strike(envelope_t* e);

/* Force the envelope to go to zero */

int
envelope_zero(envelope_t* e);

/* Change the current decay function to the function described by the provided
   settings. If the envelope is currently in the middle of decaying something,
   the decay function will switch to the newly specified decay function.

   Any values needed from the settings are copied. */

int
envelope_change_setting(envelope_t*               e,
                        envelope_setting_t const* setting);

/* Stick nframes into the buffer provided. If the envelope needs to be struck,
   it should be struckbetween calls to the sample generation function */

int
envelope_generate_samples(envelope_t* e,
                          size_t      nframes,
                          float*      buffer);
