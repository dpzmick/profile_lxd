#pragma once

#include <stdint.h>
#include <stdlib.h>

/* opaque app */
typedef struct app app_t;

app_t*
create_app(size_t sample_rate_hz,
           int*   opt_err);

void
destroy_app(app_t* app);

int
app_poll(app_t*                app,
         size_t                nframes,
         float* restrict       square_wave_out,
         float* restrict       exciter_out,
         float const* restrict lxd_signal_in);
