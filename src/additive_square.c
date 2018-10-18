#include "additive_square.h"
#include "err.h"

#include <string.h>
#include <tgmath.h>

struct additive_square {
  float nyquist;        /* max frequency we can represent at sample rate */
  float theta;          /* track the last angle we used */
};

size_t
additive_square_footprint(void)
{
  return sizeof(additive_square_t);
}

size_t
additive_square_align(void)
{
  /* Can be placed anywhere */
  return 1;
}

additive_square_t*
create_additive_square(void*  mem,
                       size_t sample_rate_hz,
                       int*   opt_err)
{
  if (opt_err) *opt_err = APP_SUCCESS;
  additive_square_t* ret = (additive_square_t*)mem;
  ret->theta       = 0;
  ret->nyquist     = (float)(sample_rate_hz)/2.;
  return ret;
}

void*
destroy_additive_square(additive_square_t* square) { return (void*)square; }

/* Put n frames into the provided buffer of floats */

int
additive_square_generate_samples(additive_square_t* square,
                                 size_t             n_frames,
                                 float              frequency,
                                 float*             out_buffer)
{
  float nyq = square->nyquist;
  float t   = square->theta;

  /* Square wave is 1st,3rd,5th,... harmonics summed.
     max harmonic we can represent is determined by nyquist freq */

  memset(out_buffer, 0, n_frames*sizeof(float));
  for (size_t i = 0; i < n_frames; ++i) {
    for (float harmonic = 1.; (harmonic*frequency < nyq); harmonic += 2.) {
      out_buffer[i] += sin(2*M_PI*harmonic*t) / (float)harmonic;
    }
    out_buffer[i] *= 4. / M_PI;
    t += frequency / (nyq*2.);
    if (t >= 1.0) t = 0;
  }

  square->theta = t;
  return APP_SUCCESS;
}
