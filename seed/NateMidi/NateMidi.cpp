/** Example of setting reading MIDI Input via UART
 *  
 * 
 *  This can be used with any 5-pin DIN or TRS connector that has been wired up
 *  to one of the UART Rx pins on Daisy.
 *  This will use D14 as the UART 1 Rx pin
 * 
 *  This example will also log incoming messages to the serial port for general MIDI troubleshooting
 */
#include "daisy_seed.h"
#include "daisysp.h"
#include <stdio.h>
#include <string.h>


/** This prevents us from having to type "daisy::" in front of a lot of things. */
using namespace daisy;
using namespace daisysp;


/** Global Hardware access */
DaisySeed       hw;
MidiUartHandler midi;
ModalVoice      modal;
Metro           tick;
Oscillator      lfo;
static Encoder  enc1, enc2;
static Switch   button1, button2, button3;
static GateIn   gateIn1, gateIn2;
GPIO            gateOut1, gateOut2, gateOut3, gateOut4;
/** FIFO to hold messages as we're ready to print them */
FIFO<MidiEvent, 128> event_log;
bool                 midiGate = 0;

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
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();

            // hw.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
            // This is to avoid Max/MSP Note outs for now..
            if(m.data[1] != 0)
            {
                midiGate = 1;
                p        = m.AsNoteOn();
                modal.SetFreq(mtof(p.note));
                modal.SetAccent((p.velocity / 127.0f));
            }
            break;
        }
        case NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();
            midiGate       = 0;
            break;
        }
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    // CC 1 for cutoff.
                    modal.SetBrightness((float)p.value / 127.0f);
                    break;
                case 2:
                    // CC 2 for res.
                    modal.SetStructure((float)p.value / 127.0f);
                    break;
                default: break;
            }
            break;
        }
        case ChannelPressure:
        {
            ChannelPressureEvent p = m.AsChannelPressure();
            modal.SetDamping(((float)p.pressure / 127.0f));
            break;
        }
        default: break;
    }
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    float sig;
    for(size_t i = 0; i < size; i++)
    {
        sig       = modal.Process(midiGate);
        out[0][i] = out[1][i];
    }
}

int main(void)
{
    hw.Init();


    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);
    float sample_rate = hw.AudioSampleRate();

    hw.SetAudioBlockSize(4);
    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);
    midi.StartReceive();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);

    modal.Init(sample_rate);
    modal.SetDamping(.5);

    lfo.Init(sample_rate);
    lfo.SetFreq(.005f);
    lfo.SetAmp(1.f);

    /** Infinite Loop */
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

