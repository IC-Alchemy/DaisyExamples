#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Resonator res;
Metro     tick;
Oscillator     lfo;
static Encoder enc1, enc2;
static Switch  button1, button2, button3;
static GateIn  gateIn1, gateIn2;
GPIO           gateOut1, gateOut2, gateOut3, gateOut4;
float frac;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    bool poop = gateIn1.Trig();
    for(size_t i = 0; i < size; i++)
    {
        float t_sig = tick.Process();

        if(poop)
        {
            res.SetFreq(rand() * frac * 770.f + 110.f); //110 - 880
            res.SetStructure(rand() * frac);
            res.SetBrightness(rand() * frac * .7f);
            res.SetDamping(rand() * frac * .7f);

            tick.SetFreq(rand() * frac * 7.f + 1);
        }
        float sig = res.Process(poop);
        out[0][i] = out[1][i] = sig;
    }
}
void initialize()
{
    button1.Init(seed::D28,
                 1000.f,
                 Switch::TYPE_MOMENTARY,
                 Switch::POLARITY_INVERTED,
                 Switch::PULL_UP);
    button2.Init(seed::D25,
                 1000.f,
                 Switch::TYPE_MOMENTARY,
                 Switch::POLARITY_INVERTED,
                 Switch::PULL_UP);

    gateIn1.Init(seed::D16);
    gateIn2.Init(seed::D15);
    gateOut1.Init(seed::D4, GPIO::Mode::OUTPUT);
    gateOut2.Init(seed::D3, GPIO::Mode::OUTPUT);
    gateOut3.Init(seed::D5, GPIO::Mode::OUTPUT);
    gateOut4.Init(seed::D6, GPIO::Mode::OUTPUT);

    enc1.Init(seed::D20, seed::D21, seed::D26);
    enc2.Init(seed::D24, seed::D25, seed::D27);
}
int main(void)
{
    hw.Configure();
    hw.Init();
    initialize();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    res.Init(.015, 24, sample_rate);
    res.SetStructure(-7.f);

    tick.Init(1.f, sample_rate);

    frac = 1.f / RAND_MAX;

    hw.StartAudio(AudioCallback);
    while(1) {}
}