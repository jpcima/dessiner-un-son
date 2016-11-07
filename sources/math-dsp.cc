#include "math-dsp.h"
#include <speex/speex_resampler.h>
#include <boost/scope_exit.hpp>
#include <algorithm>
#include <stdexcept>
#include <cmath>

double tukey_window(double a, double x) {
  if (x < a / 2) {
    return (1.0 + std::cos(M_PI * (2.0 * x / a - 1.0))) / 2.0;
  } else if (x > 1.0 - a / 2) {
    return (1.0 + std::cos(M_PI * (2.0 * (x - 1.0) / a + 1.0))) / 2.0;
  } else {
    return 1.0;
  }
}

void resample(const float *in_samples,
              unsigned in_sample_count,
              float *out_samples,
              unsigned out_sample_count) {
  if (in_sample_count == 0) {
    std::fill(out_samples, out_samples + out_sample_count, 0.0f);
    return;
  }

  int err {};

  SpeexResamplerState *resampler = speex_resampler_init(
    1, in_sample_count, out_sample_count, SPEEX_RESAMPLER_QUALITY_MAX, &err);

  if (!resampler) {
    const char *errmsg = speex_resampler_strerror(err);
    throw std::runtime_error(std::string("speex_resampler_init: ") + errmsg);
  }

  BOOST_SCOPE_EXIT(resampler) {
    speex_resampler_destroy(resampler);
  } BOOST_SCOPE_EXIT_END;

  speex_resampler_skip_zeros(resampler);

  while (out_sample_count > 0) {
    unsigned in_count = in_sample_count;
    unsigned out_count = out_sample_count;
    speex_resampler_process_float(
      resampler, 0, in_samples, &in_count, out_samples, &out_count);
    in_samples += in_count;
    in_sample_count -= in_count;
    out_samples += out_count;
    out_sample_count -= out_count;
    if (out_count == 0)
      break;
  }

  static const unsigned nzeros = 32;
  static const float zero[nzeros] {};

  while (out_sample_count > 0) {
    unsigned in_count = nzeros;
    unsigned out_count = out_sample_count;
    speex_resampler_process_float(
      resampler, 0, zero, &in_count, out_samples, &out_count);
    out_samples += out_count;
    out_sample_count -= out_count;
  }
}
