#include "app.h"

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
create_app(int* opt_err)
{
  if (opt_err) *opt_err = APP_ERR_ALLOC;
  return NULL;
}

void
destroy_app(app_t* app)
{
  (void)app;
}

int
app_poll(app_t*       app,
         size_t       nframes,
         float*       square_wave_out,
         float*       exciter_out,
         float const* lxd_signal_in)
{
  (void)app;
  (void)nframes;
  (void)square_wave_out;
  (void)exciter_out;
  (void)lxd_signal_in;
  return APP_ERR_UNIMPL;
}

/* need to generate an extremely accurate square wave */
