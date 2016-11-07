#include "wave-io-dialog.h"
#include "wave-io.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QDebug>

static const QStringList namefilters = WAVE_FORMAT_NAME_FILTERS;
static const QStringList suffixes = WAVE_FORMAT_SUFFIXES;
static const QStringList datatypenames = WAVE_DATA_TYPE_NAMES;
///

struct WaveSaveDialog::Impl {
  QSpinBox *valOutputSize {};
  QLineEdit *valFilename {};
  QComboBox *selOutputFormat {};
  QComboBox *selOutputType {};
};

WaveSaveDialog::WaveSaveDialog(QWidget *parent)
  : QDialog(parent), P(new Impl) {
  QVBoxLayout *layout = new QVBoxLayout;
  this->setLayout(layout);

  QFormLayout *form = new QFormLayout;
  layout->addLayout(form);

  QHBoxLayout *layoutFileSelect = new QHBoxLayout;
  P->valFilename = new QLineEdit;
  layoutFileSelect->addWidget(P->valFilename);
  QPushButton *btnFileSelect = new QPushButton("Choose...");
  layoutFileSelect->addWidget(btnFileSelect);
  form->addRow("File name", layoutFileSelect);

  P->valOutputSize = new QSpinBox;
  P->valOutputSize->setRange(32, 8192);
  P->valOutputSize->setValue(1024);
  form->addRow("Output size", P->valOutputSize);

  P->selOutputFormat = new QComboBox;
  for (int i = 0; i < namefilters.size(); ++i) {
    WaveDataType datatype = WaveDataType(i);
    P->selOutputFormat->addItem(namefilters[i], datatype);
  }
  form->addRow("Output format", P->selOutputFormat);

  P->selOutputType = new QComboBox;
  for (int i = 0; i < datatypenames.size(); ++i) {
    WaveDataType datatype = WaveDataType(i);
    P->selOutputType->addItem(datatypenames[i], datatype);
  }
  form->addRow("Output type", P->selOutputType);

  QDialogButtonBox *buttonbox = new QDialogButtonBox(
    QDialogButtonBox::Save|QDialogButtonBox::Cancel);
  layout->addWidget(buttonbox);

  QObject::connect(btnFileSelect, &QPushButton::clicked,
                   this, &WaveSaveDialog::chooseFile);

  QObject::connect(buttonbox, &QDialogButtonBox::accepted,
                   this, &WaveSaveDialog::accept);
  QObject::connect(buttonbox, &QDialogButtonBox::rejected,
                   this, &WaveSaveDialog::reject);
}

WaveSaveDialog::~WaveSaveDialog() {
}

int WaveSaveDialog::waveOutputDataType() const {
  return WaveDataType(P->selOutputType->currentData().toInt());
}

int WaveSaveDialog::waveOutputFormat() const {
  return WaveFormat(P->selOutputFormat->currentData().toInt());
}

int WaveSaveDialog::waveOutputSize() const {
  return P->valOutputSize->value();
}

QString WaveSaveDialog::waveFilename() const {
  return P->valFilename->text();
}

void WaveSaveDialog::chooseFile() {
  QFileDialog *filedialog = new QFileDialog(this, "Save file");
  filedialog->setAcceptMode(QFileDialog::AcceptSave);
  filedialog->setNameFilters(namefilters);

  if (filedialog->exec() == QDialog::Accepted) {
    QString selNameFilter = filedialog->selectedNameFilter();
    WaveFormat fmt = (WaveFormat)namefilters.indexOf(selNameFilter);
    P->selOutputFormat->setCurrentIndex(P->selOutputFormat->findData(fmt));

    QString filename = filedialog->selectedFiles().front();
    if (QFileInfo(filename).suffix().isEmpty())
      filename += suffixes[fmt];
    P->valFilename->setText(filename);
  }

  delete filedialog;
}

///

struct WaveOpenDialog::Impl {
  QLineEdit *valFilename {};
  QComboBox *selInputFormat {};
  QSpinBox *valInputChannel {};
};

WaveOpenDialog::WaveOpenDialog(QWidget *parent)
  : QDialog(parent), P(new Impl) {
  QVBoxLayout *layout = new QVBoxLayout;
  this->setLayout(layout);

  QFormLayout *form = new QFormLayout;
  layout->addLayout(form);

  QHBoxLayout *layoutFileSelect = new QHBoxLayout;
  P->valFilename = new QLineEdit;
  layoutFileSelect->addWidget(P->valFilename);
  QPushButton *btnFileSelect = new QPushButton("Choose...");
  layoutFileSelect->addWidget(btnFileSelect);
  form->addRow("File name", layoutFileSelect);

  P->selInputFormat = new QComboBox;
  for (int i = 0; i < namefilters.size(); ++i) {
    WaveDataType datatype = WaveDataType(i);
    P->selInputFormat->addItem(namefilters[i], datatype);
  }
  form->addRow("Input format", P->selInputFormat);

  P->valInputChannel = new QSpinBox;
  P->valInputChannel->setRange(0, 16);
  P->valInputChannel->setValue(0);
  form->addRow("Input channel", P->valInputChannel);

  QDialogButtonBox *buttonbox = new QDialogButtonBox(
    QDialogButtonBox::Open|QDialogButtonBox::Cancel);
  layout->addWidget(buttonbox);

  QObject::connect(btnFileSelect, &QPushButton::clicked,
                   this, &WaveOpenDialog::chooseFile);

  QObject::connect(buttonbox, &QDialogButtonBox::accepted,
                   this, &WaveOpenDialog::accept);
  QObject::connect(buttonbox, &QDialogButtonBox::rejected,
                   this, &WaveOpenDialog::reject);
}

WaveOpenDialog::~WaveOpenDialog() {
}

int WaveOpenDialog::waveInputFormat() const {
  return WaveFormat(P->selInputFormat->currentData().toInt());
}

unsigned WaveOpenDialog::waveInputChannel() const {
  return P->valInputChannel->value();
}

QString WaveOpenDialog::waveFilename() const {
  return P->valFilename->text();
}

void WaveOpenDialog::chooseFile() {
  QFileDialog *filedialog = new QFileDialog(this, "Open file");
  filedialog->setAcceptMode(QFileDialog::AcceptOpen);
  filedialog->setNameFilters(namefilters);

  if (filedialog->exec() == QDialog::Accepted) {
    QString selNameFilter = filedialog->selectedNameFilter();
    WaveFormat fmt = (WaveFormat)namefilters.indexOf(selNameFilter);
    P->selInputFormat->setCurrentIndex(P->selInputFormat->findData(fmt));

    QString filename = filedialog->selectedFiles().front();
    if (QFileInfo(filename).suffix().isEmpty())
      filename += suffixes[fmt];
    P->valFilename->setText(filename);
  }

  delete filedialog;
}
