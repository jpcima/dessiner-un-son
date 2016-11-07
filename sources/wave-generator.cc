#include "wave-generator.h"
#include <QDebug>

static const int channel_count = 1;

WaveGenerator::WaveGenerator(QObject *parent)
  : QIODevice(parent) {
}

void WaveGenerator::start(int bufferSize, int sampleRate) {
  this->open(QIODevice::ReadOnly);
  wavePos_ = 0;
  bufferSize_ = bufferSize;
  sampleRate_ = sampleRate;
}

void WaveGenerator::stop() {
  wavePos_ = 0;
  bufferSize_ = {};
  sampleRate_ = {};
}

qint64 WaveGenerator::readData(char *data, qint64 len) {
  float *output_buffer = (float *)data;
  const int frame_count = len / channel_count / sizeof(float);

  const std::vector<double> *wavetable = wavetable_;
  if (!wavetable) {
    std::fill(output_buffer, output_buffer + frame_count * channel_count, 0);
  } else {
    const double *wave = wavetable->data();
    int wavelen = wavetable->size();
    double wavepos = wavePos_;
    double increment = freq_ / sampleRate_;
    for (int i = 0; i < frame_count; ++i) {
      double index = wavepos * (wavelen - 1);

      int i1 = index;
      int i2 = (i1 + 1) % wavelen;
      double mu = index - i1;

      // linear interpolation
      double s = wave[i1] + mu * (wave[i2] - wave[i1]);

      for (int c = 0; c < channel_count; ++c)
        output_buffer[i * channel_count + c] = s;

      wavepos += increment;
      while (wavepos > 1.0)
        wavepos -= 1.0;
    }
    wavePos_ = wavepos;
  }

  return frame_count * channel_count * sizeof(float);
}

// qint64 WaveGenerator::bytesAvailable() const {
//   qint64 avail = bufferSize_ * channel_count * sizeof(float);
//   return avail;
// }

qint64 WaveGenerator::writeData(const char *data, qint64 len) {
  return 0;
}

void WaveGenerator::setWavetable(const std::vector<double> &table) {
  wavetable_ = &table;
}

void WaveGenerator::setFrequency(double freq) {
  freq_ = freq;
}
