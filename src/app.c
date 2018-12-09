#include "additive_square.h"
#include "app.h"
#include "common.h"
#include "disk.h"
#include "disk_thread.h"
#include "err.h"
#include "inc_fftw.h"
#include "envelope.h"

#include <assert.h>
#include <jack/ringbuffer.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct app {
  bool               running;                 /* store if we're running up or not */
  uint64_t           strike_period_ns;        /* how often to strike the pulse gen */
  uint64_t           last_strike_ns;          /* time of the last strike in nanos */
  uint64_t           sample_rate_hz;
  fftwf_plan         plan;                    /* precomputed fft plan (typefed ptr) */
  size_t             fft_in_location;         /* current idx of fft_in */
  size_t             fft_in_space;            /* total fft_in space */
  size_t             fft_out_space;           /* number of elements in fft_out buffer */
  jack_ringbuffer_t* rb;                      /* can't be inlined */

  /* Store a bunch of pointers into the trailing data, done for convenience */
  additive_square_t* sq;
  envelope_t*        cv_gen;
  disk_thread_t*     dthread;
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
  /* FIXME fft size should be computed from the sample_rate and frequency of the
     square wave, but this seems to be doing a pretty good job.

     Since freq will be changing, probably need to window the fft input */

  size_t fft_in_size  = 1024;
  size_t fft_out_size = (fft_in_size/2)+1;

  size_t footprint = 0;
  footprint = ALIGN(footprint, additive_square_align());
  footprint += additive_square_footprint();

  footprint = ALIGN(footprint, envelope_footprint());
  footprint += envelope_footprint();

  footprint = ALIGN(footprint, disk_thread_align());
  footprint += disk_thread_footprint();

  /* aligning fft buffers to cache size will be more than sufficient for SIMD alignment. */

  footprint += ALIGN(footprint, CACHELINE);
  footprint += sizeof(float) * fft_in_size;

  footprint += ALIGN(footprint, CACHELINE);
  footprint += sizeof(fftwf_complex) * fft_out_size;

  size_t             tsize   = footprint + sizeof(app_t);
  int                err     = 0;

  /* allocations */
  void*              mem     = NULL;
  fftwf_plan         plan    = NULL; /* actually a secret pointer */
  jack_ringbuffer_t* rb      = NULL;

  /* trailing memory */
  additive_square_t* sq      = NULL;
  envelope_t*        cv_gen  = NULL;
  disk_thread_t*     dthread = NULL;
  float*             fft_in  = NULL;
  fftwf_complex*     fft_out = NULL;

  err = posix_memalign(&mem, CACHELINE, tsize);
  if (err != 0) {
    if (opt_err) *opt_err = APP_ERR_ALLOC;
    goto exit;
  }

  /* Grab this first since it does another allocation */

  rb = jack_ringbuffer_create(RINGBUFFER_SIZE);
  if (!rb) {
    if (opt_err) *opt_err = APP_ERR_ALLOC;
    goto exit;
  }

  /* initialize the trailing region */
  char* ptr = (char*)mem + sizeof(app_t);

  ptr = (char*)ALIGN((size_t)ptr, additive_square_align());
  sq = create_additive_square(ptr, sample_rate_hz, opt_err);
  if (!sq) goto exit; /* opt_err already set */
  ptr += additive_square_footprint();

  envelope_setting_t setting[1];
  setting->type = ENVELOPE_EXPONENTIAL;
  setting->u.exponential->lambda = 0.00005;

  ptr = (char*)ALIGN((size_t)ptr, envelope_align());
  cv_gen = create_envelope(ptr, setting, opt_err);
  if (!cv_gen) goto exit; /* opt_err already set */
  ptr += envelope_footprint();

  ptr = (char*)ALIGN((size_t)ptr, disk_thread_align());
  dthread = create_disk_thread(ptr, rb, opt_err);
  if (!sq) goto exit; /* opt_err already set */
  ptr += disk_thread_footprint();

  ptr = (char*)ALIGN((size_t)ptr, CACHELINE);
  fft_in = (float*)ptr;
  ptr += sizeof(float) * fft_in_size;

  ptr = (char*)ALIGN((size_t)ptr, CACHELINE);
  fft_out = (fftwf_complex*)ptr;
  ptr += sizeof(fftwf_complex) * fft_out_size;

  /* Build an fft_plan, this does some calculations to determine the fastest way. */

  plan = fftwf_plan_dft_r2c_1d(fft_in_size, fft_in, fft_out, FFTW_MEASURE);
  if (!plan) {
    if (opt_err) *opt_err = APP_ERR_ALLOC; /* FIXME? */
    goto exit;
  }

  printf("%-30s %zu\n", "Created app with size", footprint);
  printf("%-30s %p\n",  "Created app at",        (void*)mem);
  printf("%-30s %p\n",  "Created square gen at", (void*)sq);
  printf("%-30s %p\n",  "Created cv_gen at",     (void*)cv_gen);
  printf("%-30s %p\n",  "Created dthread at",    (void*)dthread);
  printf("%-30s %p\n",  "Created fft_in at",     (void*)fft_in);
  printf("%-30s %p\n",  "Created fft_out at",    (void*)fft_out);
  printf("%-30s %p\n",  "Created rb at",         (void*)rb);

  /* build the returned value */
  app_t* ret = mem;
  ret->running          = false;
  ret->strike_period_ns = strike_period_ns;
  ret->last_strike_ns   = 0;
  ret->sample_rate_hz   = sample_rate_hz;
  ret->plan             = plan;
  ret->fft_in_location  = 0;
  ret->fft_in_space     = fft_in_size;
  ret->fft_out_space    = fft_out_size;
  ret->rb               = rb;
  ret->sq               = sq;
  ret->cv_gen           = cv_gen;
  ret->dthread          = dthread;
  ret->fft_in           = fft_in;
  ret->fft_out          = fft_out;
  return ret;

exit:
  if (plan)    fftwf_destroy_plan(plan);
  if (cv_gen)  destroy_envelope(cv_gen);
  if (dthread) destroy_disk_thread(dthread);
  if (sq)      destroy_additive_square(sq);
  if (rb)      jack_ringbuffer_free(rb);
  if (mem)     free(mem);

  fftwf_cleanup();
  return NULL;
}

void
destroy_app(app_t* app)
{
  if (!app) return;
  assert(!app->running); /* not valid if the app is still running */

  if (app->plan)    fftwf_destroy_plan(app->plan);
  if (app->cv_gen)  destroy_envelope(app->cv_gen);
  if (app->dthread) destroy_disk_thread(app->dthread);
  if (app->sq)      destroy_additive_square(app->sq);
  if (app->rb)      jack_ringbuffer_free(app->rb);

  free(app);
  fftwf_cleanup();
}

int
app_start(app_t* app)
{
  if (!app) return APP_ERR_INVAL;
  int ret = disk_thread_start(app->dthread);
  if (ret != APP_SUCCESS) return ret;
  app->running = true;
  return APP_SUCCESS;
}

int
app_stop(app_t* app)
{
  if (!app) return APP_ERR_INVAL;
  int ret = disk_thread_flush_and_stop(app->dthread);
  if (ret != APP_SUCCESS) return ret;
  app->running = false;
  return APP_SUCCESS;

  /* FIXME consider setting running to false even if this failed */
}

int
app_poll(app_t*                app,
         uint64_t              now_ns,
         size_t                nframes,
         float* restrict       square_wave_out,
         float* restrict       exciter_out,
         float const* restrict lxd_signal_in)
{
  if (!app)          return APP_ERR_INVAL;
  if (!app->running) return APP_ERR_INVAL;

  int err = APP_SUCCESS;

  /* Each sample represents (1/sample_rate) seconds of time */

  /* Square wave just ticks away, gen stores the last phase so we won't have any discontinuity */
  err = additive_square_generate_samples(app->sq, nframes, 440.0, square_wave_out);
  if (err != APP_SUCCESS) return err;

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
      err = envelope_generate_samples(app->cv_gen, frames_before_pulse, exciter_out);
      if (err != APP_SUCCESS) return err;
    }
    envelope_strike(app->cv_gen);
    app->last_strike_ns = now_ns + (frames_before_pulse+1)*nsec_per_frame;
  }

  err = envelope_generate_samples(app->cv_gen, pulse_frames, exciter_out);
  if (err != APP_SUCCESS) return err;

  bool write_fft = false;
  for (size_t i = 0; i < nframes; ++i) {
    app->fft_in[app->fft_in_location] = lxd_signal_in[i];
    app->fft_in_location += 1;
    if (app->fft_in_location >= app->fft_in_space) {
      fftwf_execute(app->plan);
      app->fft_in_location = 0;
      write_fft = true;
    }
  }

  size_t fft_bin_count = write_fft ? app->fft_out_space : 0;
  size_t message_size  = sample_set_footprint(nframes, fft_bin_count);

  /* Write into this thing, then copy into ringbuffer since the copy might cross
     from the end to the beginning of the buffer */
  static char mem[SAMPLE_SET_MAX];
  assert(message_size <= sizeof(mem)); // FIXME move to the app struct

  sample_set_t* sset = create_sample_set(mem, nframes, fft_bin_count, NULL);
  if (!sset) {
    printf("too big\n"); // FIXME
    return APP_DROP;
  }

  memcpy(sample_set_square_samples(sset), square_wave_out, nframes*sizeof(float));
  memcpy(sample_set_pulse_samples(sset),  exciter_out,     nframes*sizeof(float));
  memcpy(sample_set_lxd_in_samples(sset), lxd_signal_in,   nframes*sizeof(float));

  /* FIXME there's probably a cleverer way to do this */
  for (size_t i = 0; i < fft_bin_count; ++i) {
    sample_set_fft_bins(sset)[i] = cabsf(app->fft_out[i]);
  }

  size_t written = jack_ringbuffer_write(app->rb, mem, message_size);
  if (written != message_size) {
    printf("written=%zu size=%zu\n", written, message_size);
    return APP_DROP;
  }

  return APP_SUCCESS;
}
