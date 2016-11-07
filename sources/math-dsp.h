#pragma once

// window function, x in [0,1]
double tukey_window(double a, double x);

// resampling a full signal
void resample(const float *in_samples,
              unsigned in_sample_count,
              float *out_samples,
              unsigned out_sample_count);
