#pragma once

#include "err.h"

#include <stddef.h>

#define SAMPLE_SET_MAX (4096ul)

/* Data file format. Output file is a bunch of these in a row */

typedef struct sample_set sample_set_t;

struct __attribute__((packed)) sample_set {
  size_t n_samples;
  size_t n_fft_bins;
  float  data[];

  /* square_out samples */
  /* pulse_out  samples */
  /* lxd_in     samples */
  /* fft_bin    data */
};

static inline size_t
sample_set_footprint(size_t n_samples,
                     size_t n_fft_bins)
{
  return sizeof(sample_set_t)
         + 3*n_samples*sizeof(float)
         + n_fft_bins*sizeof(float);
}

static inline sample_set_t*
create_sample_set(void*   mem,                     /* assmumed to be adequately sized */
                  size_t  n_samples,
                  size_t  n_fft_bins,
                  int*    opt_err)
{
  if (sample_set_footprint(n_samples, n_fft_bins) > SAMPLE_SET_MAX) {
    if (opt_err) *opt_err = APP_ERR_INVAL;
    return NULL;
  }

  sample_set_t * sset = (sample_set_t*)mem;
  sset->n_samples  = n_samples;
  sset->n_fft_bins = n_fft_bins;
  return sset;
}

static inline float*
sample_set_square_samples(sample_set_t* sset)
{
  return sset->data;
}

static inline float*
sample_set_pulse_samples(sample_set_t* sset)
{
  return sset->data + sset->n_samples;
}

static inline float*
sample_set_lxd_in_samples(sample_set_t* sset)
{
  return sset->data + 2*sset->n_samples;
}

static inline float*
sample_set_fft_bins(sample_set_t* sset)
{
  return sset->data + 3*sset->n_samples;
}
