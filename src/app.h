#pragma once
#include <stdint.h>
#include <stdlib.h>

/* opaque app */
typedef struct app app_t;

#define APP_ERRORS(_) \
  _(APP_ERR_ALLOC,  "failed to allocate")\
  _(APP_ERR_UNIMPL, "not implemented")

enum {
  APP_SUCCESS = 0,

#define ELT(e,h) e,
  APP_ERRORS(ELT)
#undef ELT
};

char const*
app_errstr(int err);

app_t*
create_app(int* opt_err);

void
destroy_app(app_t* app);

int
app_poll(app_t*       app,
         size_t       nframes,
         float*       square_wave_out,
         float*       exciter_out,
         float const* lxd_signal_in);
