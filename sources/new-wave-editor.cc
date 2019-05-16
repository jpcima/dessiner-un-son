#include "new-wave-editor.h"
#include "ui_new-wave-editor.h"
#include <QAbstractButton>
#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QDebug>
#include <lua.hpp>
#include <string.h>
#include <math.h>

NewWaveEditor::NewWaveEditor(QWidget *parent)
    : QDialog(parent),
      m_ui(new Ui::NewWaveEditor)
{
    m_ui->setupUi(this);

    //
    initGenerators();

    //
    connect(m_ui->valWaveCode, SIGNAL(textChanged()), this, SLOT(regenWave()));
    connect(m_ui->chkPhaseDist, SIGNAL(toggled(bool)), this, SLOT(regenWave()));
    connect(m_ui->valPhaseDist, SIGNAL(valueChanged(int)), this, SLOT(regenWave()));

    //
    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    //
    initWaveCode();
    m_waveOpts = WaveOptions();
    m_ui->valWaveCode->setPlainText(m_waveCode);
    regenWave();
}

NewWaveEditor::~NewWaveEditor()
{
}

void NewWaveEditor::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole role = m_ui->buttonBox->buttonRole(button);

    if(role == QDialogButtonBox::ApplyRole)
        emit editingFinished();
}

void NewWaveEditor::regenWave()
{
    m_waveCode = m_ui->valWaveCode->toPlainText();
    m_waveOpts.phaseDist = m_ui->chkPhaseDist->isChecked();
    m_waveOpts.phaseDistAmount = m_ui->valPhaseDist->value() * 1e-2;
    computeWave(m_waveCurrent, 1024, m_waveCode,
        m_waveOpts.phaseDist ? m_waveOpts.phaseDistAmount : 0);
    updateWaveDisplay();
}

void NewWaveEditor::onTriggeredGeneratorAction()
{
    QAction *act = qobject_cast<QAction *>(sender());
    m_ui->valWaveCode->setPlainText(act->data().toString());
}

bool NewWaveEditor::computeWave(double *out, unsigned length, const QString &waveCode, double phaseDistort)
{
    memset(out, 0, length * sizeof(out[0]));

    ///
    struct LuaDeleter {
        void operator()(lua_State *x) { lua_close(x); }
    };

    auto l_alloc = [](void *ud, void *ptr, size_t osize, size_t nsize) -> void *
    {
        (void)ud; (void)osize;
        if(nsize == 0) { free(ptr); return nullptr; }
        return realloc(ptr, nsize);
    };

    std::unique_ptr<lua_State, LuaDeleter> L;
    L.reset(lua_newstate(+l_alloc, nullptr));
    luaL_openlibs(L.get());

    if (luaL_dostring(L.get(), waveCode.toUtf8().constData())) {
        showError(lua_tostring(L.get(), -1));
        return false;
    }

    ///
    for(unsigned i = 0; i < 1024; ++i)
    {
        int gen = lua_getglobal(L.get(), "wave");
        if (gen == 0) {
            lua_pop(L.get(), 1);
            showError(tr("the function \"wave\" is not defined"));
            return false;
        }

        double phase = (double)i / 1024;
        phase = distortPhase(phase, phaseDistort);

        lua_pushnumber(L.get(), phase);

        if(lua_pcall(L.get(), 1, 1, 0) != 0)
        {
            showError(QString::fromUtf8(lua_tostring(L.get(), -1)));
            return false;
        }

        int isnumber;
        double sample = lua_tonumberx(L.get(), -1, &isnumber);
        lua_pop(L.get(), 1);
        if(!isnumber)
        {
            showError(tr("the result is not a number"));
            return false;
        }

        out[i] = sample;
    }

    showError("");
    return true;
}

double NewWaveEditor::distortPhase(double phase, double amt)
{
    phase = phase * 2 - 1;

    if(amt > 0) // positive distortion
    {
        double amin = 0.5;
        double amax = 5;
        double a = amin + amt * (amax - amin);
        double p = tanh(phase * a);
        phase = p / fabs(tanh(-a));
    }
    else if(amt < 0) // negative distortion
    {
        amt = -16 * amt;

        auto g = [](double x, double a) -> double
                     { return pow(2, -a * (1 - x)); };
        double g0 = g(0, amt);
        double g1 = g(1, amt);
        double p = (g(fabs(phase), amt) - g0) / (g1 - g0);
        phase = (phase < 0) ? -p : +p;
    }

    phase = (phase + 1) * 0.5;

    return phase;
}

void NewWaveEditor::updateWaveDisplay()
{
    m_ui->waveCurrent->setData(m_waveCurrent, 1024);
}

static QPair<const char *, const char *> generators[] =
{
    {"function wave(p)\n"
     "  return math.sin(p * 2 * math.pi)\n"
     "end\n", "Sine"},

    {"function wave(p)\n"
     "  if p > 0.5 then return 0 end\n"
     "  return math.sin(p * 2 * math.pi)\n"
     "end\n", "Half-Sine"},

    {"function wave(p)\n"
     "  return math.abs(math.sin(p * 2 * math.pi))\n"
     "end\n", "Absolute Sine"},

    {"function wave(p)\n"
     "  if (math.floor(p * 4) & 1) == 1 then return 0 end\n"
     "  return math.abs(math.sin(p * 2 * math.pi))\n"
     "end\n", "Pulse-Sine"},

    {"function wave(p)\n"
     "  if p > 0.5 then return 0 end\n"
     "  return math.sin(p * 4 * math.pi)\n"
     "end\n", "Sine - even periods only"},

    {"function wave(p)\n"
     "  if p > 0.5 then return 0 end\n"
     "  return math.abs(math.sin(p * 4 * math.pi))\n"
     "end\n", "Abs-Sine - even periods only"},

    {"function wave(p)\n"
     "  if p < 0.5 then return 1 else return -1 end\n"
     "end\n", "Square"},

    {"function wave(p)\n"
     "  local q = math.abs(0.5 - p)\n"
     "  local s = math.pow(2, - q * 32)\n"
     "  if p < 0.5 then s = -s end\n"
     "  return s\n"
     "end\n", "Derived Square"},

    // Extra

    {"function wave(p)\n"
     "  return p * 2 - 1\n"
     "end\n", "Ramp"},

    {"function wave(p)\n"
     "  return 1 - p * 2\n"
     "end\n", "Saw"},

    {"function wave(p)\n"
     "  local duty = 0.25\n"
     "  if p < duty then return 1 else return -1 end\n"
     "end\n", "Pulse"},

    {"function wave(p)\n"
     "  local s = 4 * p\n"
     "  if p > 0.75 then s = s - 4\n"
     "  elseif p > 0.25 then s = 2 - s end\n"
     "  return s\n"
     "end\n", "Triangle"},

    {"function wave(p)\n"
     "  local n = math.floor(p * 4)\n"
     "  local q = p - 0.25 * n\n"
     "  local s = math.sin((3 - n) * math.pi / 2 + q * 2 * math.pi)\n"
     "  if p < 0.5 then s = s + 1 else s = s - 1 end\n"
     "  return s\n"
     "end\n", "Spike"},

    {"function wave(p)\n"
     "  local duty = 0.75\n"
     "  local s\n"
     "  if p < duty then s = math.sin(p / duty * 0.5 * math.pi)\n"
     "  else s = 1 - math.sin((p - duty) / (1 - duty) * 0.5 * math.pi) end\n"
     "  return s * 2 - 1\n"
     "end\n", "Charge"},
};

void NewWaveEditor::initGenerators()
{
    unsigned i = 0;
    for(const QPair<const char *, const char *> gen : generators)
    {
        QAction *act = new QAction(gen.second);
        QToolButton *btn = (i < 8) ? m_ui->btnBasicWave : m_ui->btnExtraWave;
        btn->addAction(act);
        act->setData(gen.first);

        QPixmap iconPixmap(32, 32);
        QPainter iconPainter(&iconPixmap);
        double data[1024];
        computeWave(data, 1024, gen.first, 0);
        showError("");
        NewWaveView::paintIntoRect(iconPainter, iconPixmap.rect(), data, 1024);
        act->setIcon(iconPixmap);

        connect(act, SIGNAL(triggered()), this, SLOT(onTriggeredGeneratorAction()));
        ++i;
    }

    m_ui->btnBasicWave->setPopupMode(QToolButton::InstantPopup);
    m_ui->btnExtraWave->setPopupMode(QToolButton::InstantPopup);
}

void NewWaveEditor::initWaveCode()
{
    m_waveCode = generators[0].first;
}

void NewWaveEditor::showError(const QString &error)
{
    m_ui->txtCodeErrors->setPlainText(error);
    m_ui->txtCodeErrors->setVisible(!error.isEmpty());
}
