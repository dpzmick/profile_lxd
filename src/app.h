#pragma once

#include <stdint.h>
#include <stdlib.h>

/* opaque app */
typedef struct app app_t;

app_t*
create_app(uint64_t sample_rate_hz,
           uint64_t strike_period_ns,
           int*     opt_err);

void
destroy_app(app_t* app);

int
app_poll(app_t*                app,
         uint64_t              now_ns,           /* monotonically increasing nanosecond time */
         size_t                nframes,
         float* restrict       square_wave_out,
         float* restrict       exciter_out,
         float const* restrict lxd_signal_in);
