#include "wave-io.h"
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/copy.hpp>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <cassert>

static int sat16(int x) {
  return (x < INT16_MIN) ? INT16_MIN : (x > INT16_MAX) ? INT16_MAX : x;
}
static int sat8(int x) {
  return (x < INT8_MIN) ? INT8_MIN : (x > INT8_MAX) ? INT8_MAX : x;
}

static float satfloat(float x) {
  if (!std::isfinite(x))
    return 0.0f;
  return (x < -1.0f) ? -1.0f : (x > 1.0f) ? 1.0f : x;
}

static void convert_to_float(float *samples,
                             unsigned sample_count,
                             WaveDataType type) {
  for (unsigned i = 0; i < sample_count; ++i) {
    float s = samples[i];
    switch (type) {
     case WaveFloat: break;
     case WaveInt16: s /= INT16_MAX; break;
     case WaveInt8: s /= INT8_MAX; break;
     default: assert(false);
    }
    samples[i] = satfloat(s);
  }
}

static void convert_from_float(float *samples,
                               unsigned sample_count,
                               WaveDataType type) {
  for (unsigned i = 0; i < sample_count; ++i) {
    float s = samples[i];
    switch (type) {
    case WaveFloat: s = satfloat(s); break;
    case WaveInt16: s = sat16(std::lround(samples[i] * INT16_MAX)); break;
    case WaveInt8: s = sat8(std::lround(samples[i] * INT8_MAX)); break;
    default: assert(false);
    }
    samples[i] = s;
  }
}

void write_wave(const float *in_samples,
                unsigned in_sample_count,
                unsigned out_sample_count,
                WaveFormat fmt,
                WaveDataType type,
                std::ostream &out) {
  std::vector<float> out_samples(out_sample_count);
  resample(in_samples, in_sample_count,
           out_samples.data(), out_sample_count);

  convert_from_float(out_samples.data(), out_sample_count, type);

  switch (fmt) {
   case WaveDat:
    for (int i = 0; i < out_sample_count; ++i)
      out << out_samples[i] << '\n';
    break;

   case WaveCpp:
   case WaveC: {
     const char *ctypename =
       (type == WaveFloat) ? "float" :
       (type == WaveInt16) ? "int16_t" :
       (type == WaveInt8) ? "int8_t" : nullptr;
     assert(ctypename);
     if (fmt == WaveCpp)
         out << "#include <array>\n" "#include <cstdint>\n\n"
             "[[gnu::unused]] static constexpr std::array<"
             << ctypename << ", " << out_sample_count << "> table {\n";
     else
         out << "#include <stdint.h>\n\n"
             "static const "
             << ctypename << " table [" << out_sample_count << "] = {\n";
     for (int i = 0; i < out_sample_count; ++i)
       out << ' ' << out_samples[i] << ',';
     out << " };\n";
     break;
   }

   default:
     throw std::runtime_error("unsupported wave output format");
  }
}

bool read_wave_from_stream(float *out_samples,
                           unsigned out_sample_count,
                           std::istream &in,
                           WaveFormat fmt,
                           unsigned channel) {
  std::ostringstream tmp(std::ios::binary);
  boost::iostreams::copy(in, tmp);

  if (in.bad())
    return false;

  return read_wave_from_string(
    out_samples, out_sample_count, tmp.str(), fmt, channel);
}

static WaveDataType detect_data_type(const float *samples,
                                     unsigned sample_count) {
  if (sample_count == 0)
    return WaveFloat;

  bool allinteger = true;
  float min = samples[0];
  float max = samples[0];

  for (unsigned i = 0; i < sample_count; ++i) {
    float s = samples[i];
    if (s < min) min = s;
    if (s > max) max = s;
    if (s != int(s)) allinteger = false;
  }

  WaveDataType type = WaveFloat;
  if (allinteger) {
    if (min >= INT8_MIN && max <= INT8_MAX)
      type = WaveInt8;
    else
      type = WaveInt16;
  }

  static const char *datatypenames[] = WAVE_DATA_TYPE_NAMES;
  std::cerr << "detected data type " << datatypenames[type] << '\n';

  return type;
}

static bool read_wave_from_dat(float *out_samples,
                               unsigned out_sample_count,
                               const std::string &in_,
                               unsigned channel) {
  std::vector<float> in_samples;
  in_samples.reserve(8192);

  std::istringstream in(in_, std::ios::binary);

  std::string line;
  line.reserve(256);
  std::vector<float> row;
  row.reserve(16);

  do {
    line.clear();
    std::getline(in, line);

    size_t commentpos = line.rfind('#');
    if (commentpos != line.npos)
      line.resize(commentpos);
    boost::trim_if(line, boost::is_any_of("\t "));

    if (!line.empty()) {
      float sample;
      row.clear();
      for (std::istringstream linestream(line, std::ios::binary);
           linestream >> sample;)
        row.push_back(sample);
    }

    in_samples.push_back(
      (channel < row.size()) ? row[channel] : 0.0f);
  } while (in);

  WaveDataType type = detect_data_type(in_samples.data(), in_samples.size());
  convert_to_float(in_samples.data(), in_samples.size(), type);

  resample(in_samples.data(), in_samples.size(),
           out_samples, out_sample_count);
  return true;
}

static bool read_wave_from_cpp(float *out_samples,
                               unsigned out_sample_count,
                               const std::string &in,
                               unsigned channel) {
  std::vector<float> in_samples;
  in_samples.reserve(8192);

  ///

  size_t index = 0;

  auto is_whitespace = [&](char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
  auto nth_char = [&](size_t n) {
    return (index + n >= in.size()) ? '\0' : in[index + n]; };
  auto skip = [&](size_t n) {
    index = std::min(in.size(), index + n); };
  auto consume_whitespace = [&]() {
    while (is_whitespace(nth_char(0))) skip(1); };
  auto end_of_input = [&]() {
    return index >= in.size(); };

  ///
  auto read_sample_array = [&]() -> bool {
    if (nth_char(0) != '{')
      return false;
    skip(1);
    in_samples.clear();
    while (nth_char(0) != '}') {
      consume_whitespace();
      float num {};
      unsigned nscanf {};
      /* TODO more C++ literal forms than scanf can handle */
      if (sscanf(in.c_str() + index, "%f%n", &num, &nscanf) != 1)
        return false;
      index += nscanf;
      in_samples.push_back(num);
      consume_whitespace();
      if (nth_char(0) == '}') {
      } else if (nth_char(0) == ',') {
        skip(1);
        consume_whitespace();
      } else
        return false;
    }
    return true;
  };

  ///
  bool have_samples = false;
  for (unsigned c = 0; !have_samples && c <= channel;) {
    while (!end_of_input() && nth_char(0) != '{')
      skip(1);
    if (end_of_input())
      break;
    if (!read_sample_array())
      continue;
    if (c == channel)
      have_samples = true;
    ++c;
  }
  if (!have_samples)
    return false;

  ///
  WaveDataType type = detect_data_type(in_samples.data(), in_samples.size());
  convert_to_float(in_samples.data(), in_samples.size(), type);

  resample(in_samples.data(), in_samples.size(),
           out_samples, out_sample_count);
  return true;
};

bool read_wave_from_string(float *out_samples,
                           unsigned out_sample_count,
                           const std::string &in,
                           WaveFormat fmt,
                           unsigned channel) {
  switch (fmt) {
    case WaveDat:
      return read_wave_from_dat(
        out_samples, out_sample_count, in, channel);

    case WaveCpp:
    case WaveC:
      return read_wave_from_cpp(
        out_samples, out_sample_count, in, channel);

   default:
     throw std::runtime_error("unsupported wave input format");
  }

  return false;
}
