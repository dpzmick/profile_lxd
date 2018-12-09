#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct envelope         envelope_t;
typedef struct envelope_setting envelope_setting_t;

enum {
  ENVELOPE_CONSTANT = 0,
  ENVELOPE_LINEAR,
  ENVELOPE_EXPONENTIAL,
  ENVELOPE_LOGARITHMIC,
};

struct envelope_setting {
  int type;

  // FIXME need better conversion from these unitless things to times

  union {
    // ds/dt = 1
    // never zero
    struct {
      float value;
    } constant[1];

    // ds/dt = -ms
    struct {
      float m;
    } linear[1];

    // ds/dt = -lambda * s
    struct {
      float lambda;
    } exponential[1];

    // ds/dt = -1/(m*s)
    struct {
      float m;
    } logarithmic[1];
  } u;
};

/* Populate an evelope setting which will take `decay_time_ns` nanoseconds to
   decay to zero, after being struck.

   An envelope type of ENVELOPE_CONSTANT will return ERR_INVALID.

   FIXME explain rounding behavior, since we can't exactly represent all
   timestamps

   FIXME return these by value so its easy to put them into a playbook?

   Returns an error code if something goes wrong, populates the provided setting
   struct if something goes right. */

int
populate_envelope_setting(int                 type,
                          uint64_t            decay_time,
                          uint64_t            sample_rate_hz,
                          envelope_setting_t* out_setting);

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
