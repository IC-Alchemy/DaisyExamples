#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "reverbsc.h"
#include "ladder.h"
#include "ladder.cpp"
using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM hw;
ReverbSc     reverb;
DcBlock      DC1, DC2;

Overdrive         drv1, drv2;
static Oscillator lfo, lfo1, lfo2, lfo3, osc, osc1, osc2;
Adsr              env1, env2;
uint8_t           counter = 0;
LadderFilter      flt, flt2;

volatile bool   env_state1 = 0;
volatile bool   env_state2 = 0;
float           CV5, cv8, cv4, rezKnob, lfoFreq, lfoKnob, env11, env22;
VoctCalibration cali, cali2;
//Autowah           wah;
//SmoothADCReader adcReader;
float LFOLFO;
bool  env_state;
bool  switcher;
float averageLfo;
float distortion = .3f;
float adc12      = 0.f;
bool  wahwah     = 0;


void EnvelopeCallback(uint16_t** output, size_t size)
{
    ///float lfo1Freq = fmap(1.f - hw.GetAdcValue(verbKnob), 0.01f, 12.f, Mapping::LOG);
    //float lfo2Freq = lfo1Freq * 2.f;
    //  float LFOLFO;
    // bool  shTrig = hw.gate_in_1.Trig();
    //switcher=  hw.gate_in_2.Trig();

    // adcReader.SetFreq(slew);
    // LFOLFO = adcReader.Process(cv8, 3.f, shTrig);

    float lfo1Freq = fmap(lfoFreq, 0.001f, 9.f, Mapping::LINEAR);
    float lfo2Freq = lfo1Freq * 0.33333f;
    lfo1.SetFreq(lfo2Freq);
    lfo2.SetFreq(lfo1Freq);


    for(size_t i = 0; i < size; i++)

    {
        float lfo1Value = lfo1.Process();
        float lfo2Value = lfo2.Process();
        // Normalize the LFO values to the range [0, 1]
        float normalizedLfo1 = (lfo1Value + 1.0f) * 0.5f;
        float normalizedLfo2 = (lfo2Value + 1.0f) * 0.5f;


        // Average the normalized LFO values
        averageLfo   = (normalizedLfo1 + normalizedLfo2) * 0.5f;
        output[0][i] = averageLfo * 4095.0f;
        output[1][i] = env11 * env11 * 4095.0f;
        //output[1][i] = ((lfo2.Process() + 1.f) * .5f)
        //               + ((lfo2.Process() + 1.f) * .5f) * 4095.0f;
    }
}

// Function Definitions
void InitHardware()
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    env1.Init(sample_rate);
    env2.Init(sample_rate);
    //adcReader.Init(sample_rate);
    flt2.Init(sample_rate);

    drv1.SetDrive(.289f);
    flt.Init(sample_rate);
    flt.SetInputDrive(1.8f);
    flt.SetPassbandGain(.2f);
    flt2.SetInputDrive(1.2f);
    flt2.SetPassbandGain(.14f);
    flt2.SetFilterMode(LadderFilter::FilterMode::BP24);
    osc.Init(sample_rate);
    osc1.Init(sample_rate);
    osc2.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc1.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc2.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    // verb.Init(sample_rate);
    //verb.SetFeedback(0.66f);
    //verb.SetLpFreq(5000.0f);

    //blk[0].Init(sample_rate);
    // blk[1].Init(sample_rate);
    reverb.Init(sample_rate);
    lfo1.Init(sample_rate);
    lfo2.Init(sample_rate);

    reverb.Init(sample_rate);
    DC1.Init(sample_rate);
    DC2.Init(sample_rate);

    lfo.SetAmp(1.f);
    lfo1.SetAmp(1.0f);
    lfo2.SetAmp(0.2f);

    // Set initial parameters for LFOs
    lfo1.SetFreq(.1f);
    lfo2.SetFreq(.0005f);
}

void UpdateParameters()
{
    cv4 = hw.GetAdcValue(CV_4);
    CV5 = hw.GetAdcValue(CV_5) + 0.0118f;
    cv8 = hw.GetAdcValue(CV_8);

    env_state1 = hw.gate_in_1.State();
    //env_state2 = hw.gate_in_2.State();


    lfoKnob    = 1.f - hw.GetAdcValue(10);
    adc12      = 1.f - hw.GetAdcValue(11);
    distortion = 1.f - hw.GetAdcValue(9);

    //float envknob       = 1.f - hw.GetAdcValue(ADC_10);

    //float envSize       = fmap(cv8, 200.f, 2300.f, Mapping::LINEAR);
    lfoFreq             = fmap(lfoKnob, 0.00011f, 8.f, Mapping::EXP);
    float outputAttack  = fmap(CV5, 0.0001f, .0115f, Mapping::LINEAR);
    float outputDecay   = fmap(CV5, 0.001f, .11f, Mapping::LINEAR);
    float outputRelease = fmap(CV5, 0.03f, .4f, Mapping::LINEAR);


    env1.SetAttackTime(outputAttack, 1.f);
    env1.SetTime(ADSR_SEG_DECAY, outputDecay);
    env1.SetTime(ADSR_SEG_RELEASE, outputRelease);
    env1.SetSustainLevel(.5f);
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAnalogControls();
    UpdateParameters();
    /** Update Params with the four knobs */
    float time       = .79f;
    float damp       = 7500.f;
    float in_level   = .8f;
    float send_level = .55f;
    float distortion1
        = fmap(distortion + (cv4 * .3f), .07f, .45f, Mapping::LINEAR);
    float distortion2
        = fmap(distortion + (cv8 * .3f), .2f, .6f, Mapping::LINEAR);

    drv1.SetDrive(distortion1);
    drv2.SetDrive(distortion2);


    reverb.SetFeedback(time);
    reverb.SetLpFreq(damp);

    for(size_t i = 0; i < size; i++)
    {
        float dingle = IN_R[i];
        float dryl   = drv2.Process(dingle);
        float dryr   = drv1.Process(dingle);
        env11        = env1.Process(env_state1);

        float ffff  = fmap(adc12, 2.f, 4600.f, Mapping::LINEAR);
        float ffff1 = fmap(adc12, 300.f, 7000.f, Mapping::LINEAR);

        flt.SetFreq(ffff * .5f
                    + (2000.f + ffff * (env11 * ((cv4 * 1.3f) + .25f))));
        flt2.SetFreq(ffff1 * .3f
                     + (1500.f + ffff1 * (env11 * ((cv4 * 1.3f) + .25f))));

        float drylFilt = flt.Process(dryl) wsdewa;
        float dryrFilt = flt2.Process(dryr);

        float sendl = drylFilt * send_level;
        float sendr = dryrFilt * send_level;

        float wetl, wetr;

        reverb.Process(sendl, sendr, &wetl, &wetr);

        float poo1 = DC1.Process(wetl);
        float poo2 = DC2.Process(wetr);

        OUT_L[i] = SoftClip(drylFilt + poo1);
        OUT_R[i] = SoftClip(dryrFilt + poo2);
    }
}

int main(void)
{
    InitHardware();

    hw.StartDac(EnvelopeCallback);


    // start callback
    hw.StartAudio(AudioCallback);

    while(1) {}
}
