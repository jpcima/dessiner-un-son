#pragma once
#include <vector>

void prepare_gui();
void prepare_audio();

class QAudioFormat;
extern QAudioFormat audio_format;
class QAudioOutput;
extern QAudioOutput *audio_out;
class WaveGenerator;
extern WaveGenerator *wave_generator;

bool save_wavedata(const std::vector<double> &wavedata);
bool load_wavedata(std::vector<double> &wavedata);
