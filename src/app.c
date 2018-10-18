#include "additive_square.h"
#include "app.h"
#include "err.h"

/* Should this be statically sized? */

struct app {
  additive_square_t* sq; /* pointer into the trailing data */
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
create_app(size_t sample_rate_hz,
           int*   opt_err)
{
  size_t footprint = 0;
  footprint += additive_square_footprint();

  size_t tsize = footprint + sizeof(app_t);
  app_t* ret = calloc(tsize, 1);
  if (!ret) {
    if (opt_err) *opt_err = APP_ERR_ALLOC;
    goto exit;
  }

  char* ptr = (char*)ret + sizeof(ret);
  ret->sq = create_additive_square(ptr, sample_rate_hz, opt_err);
  if (!ret->sq) goto exit; /* opt_err already set */

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
         size_t                nframes,
         float* restrict       square_wave_out,
         float* restrict       exciter_out,
         float const* restrict lxd_signal_in)
{
  (void)exciter_out;
  (void)lxd_signal_in;

  additive_square_generate_samples(app->sq, nframes, 440.0, square_wave_out);

  return APP_SUCCESS;
}

/* need to generate an extremely accurate square wave */
