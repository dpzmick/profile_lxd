#include "additive_square.h"
#include "pulse_gen.h"
#include "app.h"
#include "err.h"

#include <assert.h>
#include <stdio.h>

/* Should this be statically sized? */

struct app {
  uint64_t           strike_period_ns;        /* how often to strike the pulse gen */
  uint64_t           last_strike_ns;          /* time of the last strike in nanos */
  uint64_t           sample_rate_hz;

  additive_square_t* sq;                      /* pointer into the trailing data */
  pulse_gen_t*       pgen;                    /* pointer into the trailing data */

  /* Trailing memory contains the additional components...... */

  /* square wave generator is also storing the sample rate, this could be
     another case for the "customizable layout struct" idea */
};

char const*
app_errstr(int err)
{
#define ELT(e,h) case e: return h;
  switch (err) {
    APP_ERRORS(ELT)
    default: return "Unknown";
  }
#undef ELT
}

app_t*
create_app(uint64_t sample_rate_hz,
           uint64_t strike_period_ns,
           int*     opt_err)
{
  size_t footprint = 0;
  footprint += additive_square_footprint();
  footprint += pulse_gen_footprint();

  size_t tsize = footprint + sizeof(app_t);
  app_t* ret = calloc(tsize, 1);
  if (!ret) {
    if (opt_err) *opt_err = APP_ERR_ALLOC;
    goto exit;
  }

  ret->strike_period_ns = strike_period_ns;
  ret->last_strike_ns   = 0;
  ret->sample_rate_hz   = sample_rate_hz;

  char* ptr = (char*)ret + sizeof(*ret);
  ret->sq = create_additive_square(ptr, sample_rate_hz, opt_err);
  if (!ret->sq) goto exit; /* opt_err already set */
  ptr += additive_square_footprint();

  ret->pgen = create_pulse_gen(ptr, 0.00005, opt_err);
  if (!ret->pgen) goto exit; /* opt_err already set */
  ptr += pulse_gen_footprint();

  return ret;

exit:
  if (ret) free(ret);
  return NULL;
}

void
destroy_app(app_t* app)
{
  (void)app;
}

int
app_poll(app_t*                app,
         uint64_t              now_ns,
         size_t                nframes,
         float* restrict       square_wave_out,
         float* restrict       exciter_out,
         float const* restrict lxd_signal_in)
{
  (void)lxd_signal_in;

  /* Each sample represents (1/sample_rate) seconds of time */

  /* Square wave just ticks away, gen stores the last phase so we won't have any discontinuity */
  additive_square_generate_samples(app->sq, nframes, 440.0, square_wave_out);

  /* Figure out if we need to generate a pulse at some point in this interval. */

  // FIXME clean this mess up
  if (app->last_strike_ns == 0) app->last_strike_ns = now_ns-app->strike_period_ns;
  uint64_t next_pulse     = app->last_strike_ns + app->strike_period_ns;
  uint64_t nsec_per_frame = (1e9/app->sample_rate_hz);
  uint64_t frame_end_ns   = now_ns + nsec_per_frame*nframes;

  size_t pulse_frames = nframes;
  if (next_pulse <= frame_end_ns) {
    uint64_t frames_before_pulse = 0;
    if (next_pulse > now_ns) {
      frames_before_pulse = (next_pulse-now_ns)/nsec_per_frame;
      printf("next_pulse %lu last %lu period %lu nsec_per %lu now %lu frame_end %lu\n",
              next_pulse, app->last_strike_ns, app->strike_period_ns,
              nsec_per_frame, now_ns, frame_end_ns);
      printf("pulsing, frames_before %lu\n", frames_before_pulse);
      pulse_gen_generate_samples(app->pgen, frames_before_pulse, exciter_out);
    }
    pulse_gen_strike(app->pgen);
    app->last_strike_ns = now_ns + (frames_before_pulse+1)*nsec_per_frame;
  }

  pulse_gen_generate_samples(app->pgen, pulse_frames, exciter_out);

  return APP_SUCCESS;
}

/* need to generate an extremely accurate square wave */
