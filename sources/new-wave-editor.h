#pragma once
#include <QDialog>
#include <memory>

namespace Ui { class NewWaveEditor; }
class QAbstractButton;

class NewWaveEditor : public QDialog
{
    Q_OBJECT

public:
    explicit NewWaveEditor(QWidget *parent = nullptr);
    ~NewWaveEditor();

    static constexpr unsigned waveSize()
        { return sizeof(m_waveCurrent) / sizeof(m_waveCurrent[0]); }
    const double *waveData() const noexcept
        { return m_waveCurrent; }

signals:
    void editingFinished();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
    void regenWave();
    void onTriggeredGeneratorAction();

private:
    void updateWaveDisplay();
    bool computeWave(double *out, unsigned length, const QString &waveCode, double phaseDistort);
    static double distortPhase(double phase, double amt);

    void initGenerators();
    void initWaveCode();

    void showError(const QString &error);

    std::unique_ptr<Ui::NewWaveEditor> m_ui;

    struct WaveOptions
    {
        bool phaseDist = false;
        double phaseDistAmount = 0.0;
    };

    QString m_waveCode;
    WaveOptions m_waveOpts;
    double m_waveCurrent[1024];
};
