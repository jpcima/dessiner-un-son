#pragma once
#include <vector>
#include <iosfwd>

enum WaveFormat {
  WaveDat,
  WaveCpp,
};

#define WAVE_FORMAT_NAME_FILTERS                \
  {"Data points (*.dat)",                       \
   "C++ source (*.h)"}
#define WAVE_FORMAT_SUFFIXES                    \
  {".dat", ".h"}

enum WaveDataType {
  WaveFloat,
  WaveInt16,
  WaveInt8,
};

#define WAVE_DATA_TYPE_NAMES                    \
  {"32-bit float", "16-bit signed integer", "8-bit signed integer"}

void write_wave(const float *in_samples,
                unsigned in_sample_count,
                unsigned out_sample_count,
                WaveFormat fmt,
                WaveDataType type,
                std::ostream &out);

bool read_wave_from_stream(float *out_samples,
                           unsigned out_sample_count,
                           std::istream &in,
                           WaveFormat fmt,
                           unsigned channel);

bool read_wave_from_string(float *out_samples,
                           unsigned out_sample_count,
                           const std::string &in,
                           WaveFormat fmt,
                           unsigned channel);

void resample(const float *in_samples,
              unsigned in_sample_count,
              float *out_samples,
              unsigned out_sample_count);
