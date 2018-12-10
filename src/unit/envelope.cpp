#include "catch.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>

extern "C" {
#include "../envelope.h"
#include "../err.h"
}

namespace {
constexpr float EFFECTIVE_ZERO = 1e-8;
constexpr uint64_t s2ns(uint64_t seconds) { return seconds * 1000000000; }

constexpr uint64_t samples_for_ns(uint64_t ns, uint64_t sample_rate)
{
  // always get at least one sample
  return std::max(
      1ul,
      (uint64_t)(((double)sample_rate/1e9) * (double)ns));
}

template <typename T, typename F>
void for_some(std::initializer_list<T> ts, F f)
{
  for (auto t : ts) {
    f(t);
  }
}

template <typename F>
void for_some_sample_rates(F&& f)
{
  for_some<uint64_t>({1, 44100, 48000, 192000}, f);
}

template <typename F>
void for_some_decays(F&& f)
{
  for_some<uint64_t>({s2ns(4), s2ns(12), s2ns(3), 5000}, f);
}

template <typename F>
void for_all_decaying_types(F&& f)
{
  for_some<int>({ENVELOPE_LINEAR, ENVELOPE_EXPONENTIAL, ENVELOPE_LOGARITHMIC}, f);
  // don't include constant, since it doesn't decay
}

} // anon namespace

TEST_CASE("decays actually work", "[envelope]")
{
  for_all_decaying_types([](int type) {
    for_some_sample_rates([&](uint64_t sample_rate) {
      for_some_decays([&](uint64_t decay_ns) {
        int                err      = 0;
        void*              mem      = nullptr;
        envelope_t*        envelope = nullptr;
        float*             buffer   = nullptr;
        size_t             how_many = 0;
        envelope_setting_t setting[1];

        // fprintf(stderr, "type=%d srate=%zu decay_ns=%zu\n", type, sample_rate, decay_ns);

        populate_envelope_setting(type, decay_ns, sample_rate, setting);
        REQUIRE(err == APP_SUCCESS);

        // now, if we instantiate an envelope, poll it for the adequate number of
        // samples

        mem = malloc(envelope_footprint());
        REQUIRE(mem);

        // add one, we should be at zero after the last sample of decay_ns samples
        how_many = 1 + samples_for_ns(decay_ns, sample_rate);
        buffer   = new float[how_many]; // throws on failure

        envelope = create_envelope(mem, setting, &err);
        REQUIRE(envelope);

        err = envelope_strike(envelope);
        REQUIRE(err == APP_SUCCESS);

        err = envelope_generate_samples(envelope, how_many, buffer);
        REQUIRE(err == APP_SUCCESS);

        for (size_t i = 0; i < how_many-1; ++i) {
          REQUIRE(!std::isnan(buffer[i]));
          REQUIRE(buffer[i] + FLT_EPSILON > EFFECTIVE_ZERO); // close enough
        }
        REQUIRE(!std::isnan(buffer[how_many-1]));
        REQUIRE(EFFECTIVE_ZERO - buffer[how_many-1] <= FLT_EPSILON);

        // run for a bit longer and make sure everything stays positive
        err = envelope_generate_samples(envelope, how_many, buffer);
        REQUIRE(err == APP_SUCCESS);

        for (size_t i = 0; i < how_many-1; ++i) {
          REQUIRE(buffer[i] >= 0.0);
        }

        delete buffer;
        destroy_envelope(envelope);
        free(mem);
      });
    });
  });
}
