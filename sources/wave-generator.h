#pragma once
#include <QIODevice>

class WaveGenerator : public QIODevice {
  Q_OBJECT;

 public:
  explicit WaveGenerator(QObject *parent = nullptr);
  void start(int bufferSize, int sampleRate);
  void stop();

  qint64 readData(char *data, qint64 len) override;
  qint64 writeData(const char *data, qint64 len) override;

  // qint64 bytesAvailable() const override;

  void setWavetable(const std::vector<double> &table);
  void setFrequency(double freq);

 private:
  int bufferSize_ {};
  int sampleRate_ {};
  const std::vector<double> *wavetable_;
  double wavePos_ {};
  double freq_ = 220.0;
};
