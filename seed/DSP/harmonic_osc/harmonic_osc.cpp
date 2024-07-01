#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed              hw;
static HarmonicOscillator<16> harm;
static Oscillator             lfo;
static AdEnv                  env;
static Encoder                enc1, enc2;
static Switch                 button1, button2, button3;
static GateIn                 gateIn1, gateIn2;
GPIO                          gateOut1, gateOut2, gateOut3, gateOut4;
float scale[] = {55.f, 65.41f, 73.42f, 82.41f, 98.f, 110.f};
int   note    = 0;
void  initialize()
{
    button1.Init(seed::D16,
                 1000.f,
                 Switch::TYPE_MOMENTARY,
                 Switch::POLARITY_INVERTED,
                 Switch::PULL_UP);
    button2.Init(seed::D25,
                 1000.f,
                 Switch::TYPE_MOMENTARY,
                 Switch::POLARITY_INVERTED,
                 Switch::PULL_UP);

    //gateIn1.Init(seed::D16);
    gateIn2.Init(seed::D15);
    gateOut1.Init(seed::D4, GPIO::Mode::OUTPUT);
    gateOut2.Init(seed::D3, GPIO::Mode::OUTPUT);
    gateOut3.Init(seed::D5, GPIO::Mode::OUTPUT);
    gateOut4.Init(seed::D6, GPIO::Mode::OUTPUT);

    enc1.Init(seed::D20, seed::D21, seed::D26);
    enc2.Init(seed::D24, seed::D25, seed::D27);
}
static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    button1.Debounce();
    float knob1 = hw.adc.GetFloat(0);
    float knob2 =hw.adc.GetFloat(1);
    float knob3 = hw.adc.GetFloat(2);
    float knob4 = hw.adc.GetFloat(3);
    for(size_t i = 0; i < size; i += 2)
    {
        //retrig env on EOC and go to next note
       
            harm.SetFreq(mtof(note));
            note = (note + 1) % 6;
    
        harm.SetSingleAmp(knob1, 0);
        harm.SetSingleAmp(knob2, 1);
        harm.SetSingleAmp(knob3, 2);
        harm.SetSingleAmp(knob4, 3);
        harm.SetSingleAmp(knob1, 4);
        harm.SetSingleAmp(knob2, 5);
        harm.SetSingleAmp(knob3, 6);
        harm.SetSingleAmp(knob4, 7);
        harm.SetSingleAmp(knob1, 8);
        harm.SetSingleAmp(knob2, 9);
                harm.SetSingleAmp(knob3, 10);
        harm.SetSingleAmp(knob4, 11);
        harm.SetSingleAmp(knob1, 12);
        harm.SetSingleAmp(knob2, 13);
        harm.SetSingleAmp(knob3, 14);
        harm.SetSingleAmp(knob4, 15);


      
        out[i] = out[i + 1] = harm.Process();
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    hw.Configure();
    hw.Init();
    initialize();

    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    //This is our ADC configuration
    AdcChannelConfig adcConfig;
    //Configure pin 21 as an ADC input. This is where we'll read the knob.
    adcConfig.InitSingle(hw.GetPin(22));
    adcConfig.InitSingle(hw.GetPin(23));
    adcConfig.InitSingle(hw.GetPin(24));
    adcConfig.InitSingle(hw.GetPin(25));

    //Initialize the adc with the config we just made
    hw.adc.Init(&adcConfig, 1);
    //Start reading values
    hw.adc.Start();

    //init harmonic oscillator
    harm.Init(sample_rate);
    harm.SetFirstHarmIdx(1);

    //init envelope
    env.Init(sample_rate);
    env.SetTime(ADENV_SEG_ATTACK, 0.05f);
    env.SetTime(ADENV_SEG_DECAY, 0.35f);
    env.SetMin(0.0);
    env.SetMax(15.f);
    env.SetCurve(0); // linear

    // start callback
    hw.StartAudio(AudioCallback);

    while(1) {}
}
