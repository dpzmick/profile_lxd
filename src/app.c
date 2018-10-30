#include "additive_square.h"
#include "app.h"
#include "common.h"
#include "err.h"
#include "pulse_gen.h"

#include <assert.h>
#include <complex.h>
#include <fftw3.h>
#include <stdio.h>
#include <string.h>

struct app {
  uint64_t           strike_period_ns;        /* how often to strike the pulse gen */
  uint64_t           last_strike_ns;          /* time of the last strike in nanos */
  uint64_t           sample_rate_hz;
  fftwf_plan         plan;                    /* precomputed fft plan */
  size_t             fft_in_location;         /* current idx of fft_in */
  size_t             fft_in_space;            /* total fft_in space */

  /* Store a bunch of pointers into the trailing data, done for convenience */
  additive_square_t* sq;
  pulse_gen_t*       pgen;
  float*             fft_in;
  fftwf_complex*     fft_out;

  /* Trailing memory contains the additional components...... */
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
  /* FIXME fft size should be computed from the sample_rate and frequency of the square wave */
  int fft_size = 1024;

  size_t footprint = 0;
  footprint += additive_square_footprint();
  footprint += pulse_gen_footprint();

  /* aligning fft buffers to cache size will be more than sufficient for SIMD alignment. */

  footprint += ALIGN(footprint, CACHELINE);
  footprint += sizeof(float) * fft_size;

  footprint += ALIGN(footprint, CACHELINE);
  footprint += sizeof(fftwf_complex) * (fft_size/2)+1; /* doesn't require full size */

  size_t             tsize   = footprint + sizeof(app_t);
  int                err     = 0;
  void*              mem     = NULL;
  app_t*             ret     = NULL;
  additive_square_t* sq      = NULL;
  pulse_gen_t*       pgen    = NULL;
  float*             fft_in  = NULL;
  fftwf_complex*     fft_out = NULL;

  err = posix_memalign(&mem, CACHELINE, tsize);
  if (err != 0) {
    if (opt_err) *opt_err = APP_ERR_ALLOC;
    goto exit;
  }

  /* initialize the trailing region */
  char* ptr = (char*)mem + sizeof(*ret);
  sq = create_additive_square(ptr, sample_rate_hz, opt_err);
  if (!sq) goto exit; /* opt_err already set */
  ptr += additive_square_footprint();

  pgen = create_pulse_gen(ptr, 0.00005, opt_err);
  if (!pgen) goto exit; /* opt_err already set */
  ptr += pulse_gen_footprint();

  ptr = (char*)ALIGN((size_t)ptr, CACHELINE);
  fft_in = (float*)ptr;
  ptr += sizeof(float) * fft_size;

  ptr = (char*)ALIGN((size_t)ptr, CACHELINE);
  fft_out = (fftwf_complex*)ptr;
  ptr += sizeof(fftwf_complex) * (fft_size/2)+1; /* doesn't require full size */

  printf("%-30s %p\n", "Created app at",        (void*)mem);
  printf("%-30s %p\n", "Created square gen at", (void*)sq);
  printf("%-30s %p\n", "Created pgen at",       (void*)pgen);
  printf("%-30s %p\n", "Created fft_in at",     (void*)fft_in);
  printf("%-30s %p\n", "Created fft_out at",    (void*)fft_out);

  /* Build an fft_plan, this does some calculations to determine the fastest way.
     Don't alias these. */
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(fft_size, fft_in, fft_out, FFTW_MEASURE);

  /* build the returned value */
  memset(mem, 0, sizeof(*ret));     /* zero only the fields actually in the struct */
  ret                   = mem;
  ret->strike_period_ns = strike_period_ns;
  ret->last_strike_ns   = 0;
  ret->sample_rate_hz   = sample_rate_hz;
  ret->plan             = plan;
  ret->sq               = sq;
  ret->pgen             = pgen;
  ret->fft_in           = fft_in;
  ret->fft_out          = fft_out;
  return ret;

exit:
  if (mem) free(mem);
  return NULL;
}

void
destroy_app(app_t* app)
{
  if (!app) return;
  fftwf_destroy_plan(app->plan);
  free(app);
}

int
app_poll(app_t*                app,
         uint64_t              now_ns,
         size_t                nframes,
         float* restrict       square_wave_out,
         float* restrict       exciter_out,
         float const* restrict lxd_signal_in)
{
  /* Each sample represents (1/sample_rate) seconds of time */

  /* Square wave just ticks away, gen stores the last phase so we won't have any discontinuity */
  additive_square_generate_samples(app->sq, nframes, 440.0, square_wave_out);

  /* Figure out if we need to generate a pulse at some point in this interval. */

  if (app->last_strike_ns == 0) app->last_strike_ns = now_ns-app->strike_period_ns;
  uint64_t next_pulse     = app->last_strike_ns + app->strike_period_ns;
  uint64_t nsec_per_frame = (1e9/app->sample_rate_hz);
  uint64_t frame_end_ns   = now_ns + nsec_per_frame*nframes;

  size_t pulse_frames = nframes;
  if (next_pulse <= frame_end_ns) {
    uint64_t frames_before_pulse = 0;
    if (next_pulse > now_ns) {
      frames_before_pulse = (next_pulse-now_ns)/nsec_per_frame;
      pulse_gen_generate_samples(app->pgen, frames_before_pulse, exciter_out);
    }
    pulse_gen_strike(app->pgen);
    app->last_strike_ns = now_ns + (frames_before_pulse+1)*nsec_per_frame;
  }

  pulse_gen_generate_samples(app->pgen, pulse_frames, exciter_out);

  for (size_t i = 0; i < nframes; ++i) {
    app->fft_in[app->fft_in_location] = lxd_signal_in[i];
    if (app->fft_in_location >= app->fft_in_space) {
      fftwf_execute(app->plan);
      app->fft_in_location = 0;

      /* what do I do with this information */

      /* Ship pointer over to other thread, if it doesn't give it back in time
         we'll explode or something idk */
    }
  }

  return APP_SUCCESS;
}

/* need to generate an extremely accurate square wave */
