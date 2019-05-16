#include "main.h"
#include "dot-editor-widget.h"
#include "wave-generator.h"
#include "wave-io.h"
#include "wave-io-dialog.h"
#include "new-wave-editor.h"
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QStatusBar>
#include <QAudioOutput>
#include <QFile>
#include <QDebug>
#include <boost/scope_exit.hpp>
#include <iostream>
#include <fstream>

int dotsize = 1;
int gridwidth = 1024;
int gridheight = 512;

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("Dessiner un son");

  prepare_audio();
  prepare_gui();

  return app.exec();
}

void prepare_gui() {
  QMainWindow *win = new QMainWindow;

  QMenuBar *mb = win->menuBar();
  QMenu *fileMenu = mb->addMenu("File");
  QAction *actGenerate = fileMenu->addAction(QIcon::fromTheme("document-new"), "Generate &new...");
  actGenerate->setShortcut(QKeySequence("Ctrl+N"));
  QAction *actOpen = fileMenu->addAction(QIcon::fromTheme("document-open"), "&Open");
  actOpen->setShortcut(QKeySequence("Ctrl+O"));
  QAction *actSave = fileMenu->addAction(QIcon::fromTheme("document-save"), "&Save");
  actSave->setShortcut(QKeySequence("Ctrl+S"));

  QToolBar *tb = new QToolBar;
  win->addToolBar(tb);

  QAction *actSmooth = tb->addAction("Smooth");
  QDoubleSpinBox *valSmooth = new QDoubleSpinBox;
  valSmooth->setRange(0.05, 1);
  valSmooth->setSingleStep(0.05);
  valSmooth->setValue(0.8);
  tb->addWidget(valSmooth);
  tb->addSeparator();

  QAction *actWindow = tb->addAction("Window");
  QDoubleSpinBox *valWindow = new QDoubleSpinBox;
  valWindow->setRange(0.05, 1);
  valWindow->setSingleStep(0.05);
  valWindow->setValue(0.5);
  tb->addWidget(valWindow);
  tb->addSeparator();

  tb->addWidget(new QLabel("Shift"));
  QAction *actShiftL2 = tb->addAction("↞");
  QAction *actShiftL1 = tb->addAction("←");
  QAction *actShiftR1 = tb->addAction("→");
  QAction *actShiftR2 = tb->addAction("↠");
  tb->addSeparator();

  tb->addWidget(new QLabel("Mirror"));
  QAction *actMirrorRToL = tb->addAction("⥒");
  QAction *actMirrorLToR = tb->addAction("⥓");
  tb->addSeparator();

  tb->addWidget(new QLabel("Invert"));
  QAction *actInvertL = tb->addAction("Left");
  QAction *actInvertR = tb->addAction("Right");
  tb->addSeparator();

  tb->addWidget(new QLabel("Sound"));
  QAction *actSoundPlay = tb->addAction("Play");
  QAction *actSoundStop = tb->addAction("Stop");
  QDoubleSpinBox *valSoundFreq = new QDoubleSpinBox;
  valSoundFreq->setRange(10.0, 8000.0);
  valSoundFreq->setSingleStep(10.0);
  valSoundFreq->setValue(220.0);
  tb->addWidget(valSoundFreq);
  tb->addSeparator();

  BasicDotEditorWidget *editor;
  if (dotsize > 1)
    editor = new DotEditorWidget(gridwidth, gridheight, dotsize);
  else
    editor = new CompactDotEditorWidget(gridwidth, gridheight);
  editor->initialize();
  win->setCentralWidget(editor);

  QStatusBar *statusBar = new QStatusBar;
  win->setStatusBar(statusBar);

  QObject::connect(actSave, &QAction::triggered,
                   editor, [editor]() { save_wavedata(editor->dotData()); });
  QObject::connect(actOpen, &QAction::triggered,
                   editor, [editor]() {
                     std::vector<double> &dotdata = editor->dotData();
                     if (load_wavedata(dotdata))
                       editor->update();
                   });
  QObject::connect(actGenerate, &QAction::triggered,
                   editor, [editor]() {
                               std::vector<double> &dotdata = editor->dotData();
                               if (gen_wavedata(dotdata))
                                   editor->update();
                           });

  QObject::connect(actSmooth, &QAction::triggered,
                   editor, [editor, valSmooth]() { editor->smooth(valSmooth->value()); });

  QObject::connect(actWindow, &QAction::triggered,
                   editor, [editor, valWindow]() { editor->window(valWindow->value()); });

  QObject::connect(actShiftL2, &QAction::triggered,
                   editor, [editor]() { editor->shiftData(-5); });
  QObject::connect(actShiftL1, &QAction::triggered,
                   editor, [editor]() { editor->shiftData(-1); });
  QObject::connect(actShiftR1, &QAction::triggered,
                   editor, [editor]() { editor->shiftData(+1); });
  QObject::connect(actShiftR2, &QAction::triggered,
                   editor, [editor]() { editor->shiftData(+5); });

  QObject::connect(actMirrorRToL, &QAction::triggered,
                   editor, [editor]() { editor->mirror(DotEditorWidget::MirRightToLeft); });
  QObject::connect(actMirrorLToR, &QAction::triggered,
                   editor, [editor]() { editor->mirror(DotEditorWidget::MirLeftToRight); });

  QObject::connect(actInvertL, &QAction::triggered,
                   editor, [editor]() { editor->invert(DotEditorWidget::LeftSide); });
  QObject::connect(actInvertR, &QAction::triggered,
                   editor, [editor]() { editor->invert(DotEditorWidget::RightSide); });

  QObject::connect(editor, &DotEditorWidget::hoveredGridCoord,
                   statusBar, [statusBar](QPoint gridpoint) {
                     QString status = QString("X %0 Y %1")
                       .arg(gridpoint.x(), -8).arg(gridpoint.y(), -8);
                     statusBar->showMessage(status);
                   });

  QObject::connect(actSoundPlay, &QAction::triggered,
                   ::audio_out, [editor, valSoundFreq]() {
                     ::audio_out->stop();
                     ::wave_generator->setWavetable(editor->dotData());
                     ::wave_generator->setFrequency(valSoundFreq->value());
                     ::audio_out->start(::wave_generator);
                   });
  QObject::connect(actSoundStop, &QAction::triggered,
                   ::audio_out, []() { ::audio_out->stop(); });
  QObject::connect(valSoundFreq, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                   ::audio_out, [](double value) { ::wave_generator->setFrequency(value); });

  win->show();
}

QAudioFormat audio_format;
QAudioOutput *audio_out;
WaveGenerator *wave_generator;

void prepare_audio() {
  QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
  audio_format = info.preferredFormat();
  audio_format.setChannelCount(1);
  audio_format.setSampleSize(8 * sizeof(float));
  audio_format.setCodec("audio/pcm");
  audio_format.setSampleType(QAudioFormat::Float);

  audio_out = new QAudioOutput(info, audio_format);
  std::cerr << "audio sampling rate is " << audio_format.sampleRate() << "\n";
  std::cerr << "audio buffer size is " << ::audio_out->bufferSize() << "\n";

  ::wave_generator = new WaveGenerator(audio_out);
  ::wave_generator->start(::audio_out->bufferSize(), ::audio_format.sampleRate());
}

bool save_wavedata(const std::vector<double> &wavedata) {
  WaveSaveDialog *dlg = new WaveSaveDialog;
  BOOST_SCOPE_EXIT(dlg) { delete dlg; } BOOST_SCOPE_EXIT_END;

  if (dlg->exec() != WaveSaveDialog::Accepted)
    return true;

  QString outfilename = dlg->waveFilename();
  if (outfilename.isEmpty())
    return true;

  int outsize = dlg->waveOutputSize();
  WaveFormat outfmt = WaveFormat(dlg->waveOutputFormat());
  WaveDataType outtype = WaveDataType(dlg->waveOutputDataType());

  std::vector<float> fdata(wavedata.begin(), wavedata.end());
  std::ofstream out(outfilename.toStdString(), std::ios::binary);
  write_wave(fdata.data(), fdata.size(), outsize, outfmt, outtype, out);
  out.flush();

  if (!out) {
    QFile::remove(outfilename);
    return false;
  }

  return true;
}

bool load_wavedata(std::vector<double> &wavedata) {
  WaveOpenDialog *dlg = new WaveOpenDialog;
  BOOST_SCOPE_EXIT(dlg) { delete dlg; } BOOST_SCOPE_EXIT_END;

  if (dlg->exec() != WaveOpenDialog::Accepted)
    return true;

  QString infilename = dlg->waveFilename();
  if (infilename.isEmpty())
    return true;

  WaveFormat infmt = WaveFormat(dlg->waveInputFormat());
  unsigned inchannel = dlg->waveInputChannel();

  std::ifstream in(infilename.toStdString(), std::ios::binary);
  std::vector<float> fdata(wavedata.begin(), wavedata.end());
  if (!read_wave_from_stream(fdata.data(), fdata.size(), in, infmt, inchannel))
    return false;

  wavedata.assign(fdata.begin(), fdata.end());
  return true;
}

bool gen_wavedata(std::vector<double> &wavedata)
{
    NewWaveEditor waveEd;
    if (waveEd.exec() != QDialog::Accepted)
        return false;

    std::vector<float> in_samples(waveEd.waveData(), waveEd.waveData() + waveEd.waveSize());
    std::unique_ptr<float[]> out_samples(new float[wavedata.size()]);

    resample(in_samples.data(), in_samples.size(),
             out_samples.get(), wavedata.size());

    wavedata.assign(&out_samples[0], &out_samples[wavedata.size()]);
    return true;
}
