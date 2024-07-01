modalmidi
#include "daisy_seed.h"
#include "daisysp.h"
#include <stdlib.h>

    using namespace daisy;
using namespace daisysp;
MidiUartHandler midi;

/** FIFO to hold messages as we're ready to print them */
FIFO<MidiEvent, 128> event_log;
DaisySeed            hw;
ModalVoice           modal;
Metro                tick;
Oscillator           lfo;
static Encoder       enc1, enc2;
static Switch        button1, button2, button3;
static GateIn        gateIn1, gateIn2;
GPIO                 gateOut1, gateOut2, gateOut3, gateOut4;
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
    /*button2.Init(seed::D25,
                 1000.f,
                 Switch::TYPE_MOMENTARY,
                 Switch::POLARITY_INVERTED,
                 Switch::PULL_UP);
   */
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
    bool poo = gateIn1.State();
    for(size_t i = 0; i < size; i++)
    {
        float sig = fabsf(lfo.Process());
        modal.SetStructure(sig);
        modal.SetBrightness(.1f + sig * .1f);

        out[0][i] = out[1][i] = modal.Process(poo);
    }
}
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            modal.Trig();
            // hw.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
            // This is to avoid Max/MSP Note outs for now..
            if(m.data[1] != 0)
            {
                p = m.AsNoteOn();
                modal.SetFreq(mtof(p.note));
                modal.SetAccent(p.velocity / 127.0f);
            }
        }
        break;
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    // CC 1 for cutoff.
                    //   filt.SetFreq(mtof((float)p.value));
                    break;
                case 2:
                    // CC 2 for res.
                    //  filt.SetRes(((float)p.value / 127.0f));
                    break;
                default: break;
            }
            break;
        }
        default: break;
    }
}
int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();
    initialize();

    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);

    midi.StartReceive();
    modal.Init(sample_rate);
    modal.SetDamping(.5);


    tick.Init(2.f, sample_rate);

    lfo.Init(sample_rate);
    lfo.SetFreq(.005f);
    lfo.SetAmp(1.f);

    hw.StartAudio(AudioCallback);
    while(1)
    {
        /** Process MIDI in the background */
        midi.Listen();

        /** Loop through any MIDI Events */
        while(midi.HasEvents())
        {
            HandleMidiMessage(midi.PopEvent());
        }
    }
}
