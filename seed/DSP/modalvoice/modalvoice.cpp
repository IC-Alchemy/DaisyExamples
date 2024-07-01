#include "daisy_seed.h"
#include "daisysp.h"
#include <stdlib.h>

using namespace daisy;
using namespace daisysp;
MidiUartHandler midi;

/** FIFO to hold messages as we're ready to print them */
FIFO<MidiEvent, 128> event_log;
DaisySeed      hw;
ModalVoice     modal;
Metro          tick;
Oscillator     lfo;
static Encoder enc1, enc2;
static Switch  button1, button2, button3;
static GateIn  gateIn1, gateIn2;
GPIO           gateOut1, gateOut2, gateOut3, gateOut4;
// A minor pentatonic
float freqs[5] = {440.f, 523.25f, 587.33f, 659.25f, 783.99f};
bool  sus      = false;


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
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    button1.Debounce();
    button2.Debounce();

    bool poo = button1.RisingEdge();
    bool poo1 = button1.RisingEdge();
    for(size_t i = 0; i < size; i++)
    {
        bool t = tick.Process();
        if(poo1)
        {
            modal.SetSustain(sus = !sus);
        }

        if(poo || modal.GetAux() > .1f)
        {
            modal.SetFreq(freqs[rand() % 5]);
        }

        float sig = fabsf(lfo.Process());
        modal.SetStructure(sig);
        modal.SetBrightness(.1f + sig * .1f);

        out[0][i] = out[1][i] = modal.Process(t);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();
    initialize();


    midi.StartReceive();
    modal.Init(sample_rate);
    modal.SetDamping(.5);


    tick.Init(2.f, sample_rate);

    lfo.Init(sample_rate);
    lfo.SetFreq(.005f);
    lfo.SetAmp(1.f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
