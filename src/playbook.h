#pragma once

#include "envelope.h"

#include <stddef.h>
#include <stdint.h>

typedef struct scene scene_t;

struct scene {
  uint64_t           scene_duration;

  envelope_setting_t wave_gen_frequency;
  uint64_t           wave_gen_freq_env_period;
  float              wave_gen_freq_min;
  float              wave_gen_freq_max;

  envelope_setting_t wave_gen_amplitude;
  uint64_t           wave_gen_amp_env_period;

  envelope_setting_t cv_out;
  uint64_t           cv_out_env_period;
};

// the playbook should somehow abstract the time moving forward thing and just
// tell app what the next thing to do is?
