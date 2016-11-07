#pragma once
#include <QDialog>
#include <memory>

class WaveSaveDialog : public QDialog {
  Q_OBJECT;
 public:
  explicit WaveSaveDialog(QWidget *parent = nullptr);
  ~WaveSaveDialog();

  int waveOutputFormat() const;
  int waveOutputDataType() const;
  int waveOutputSize() const;
  QString waveFilename() const;

 public slots:
  void chooseFile();

 private:
  struct Impl;
  std::unique_ptr<Impl> P;
};

///
class WaveOpenDialog : public QDialog {
  Q_OBJECT;
 public:
  explicit WaveOpenDialog(QWidget *parent = nullptr);
  ~WaveOpenDialog();

  int waveInputFormat() const;
  unsigned waveInputChannel() const;
  QString waveFilename() const;

 public slots:
  void chooseFile();

 private:
  struct Impl;
  std::unique_ptr<Impl> P;
};
