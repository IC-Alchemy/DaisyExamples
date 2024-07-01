#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "ladder.h"
#include "ladder.cpp"
#include "SmoothADCReader.h"
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
DaisyPatchSM      hw;
Overdrive         drv1, drv2;
static Oscillator lfo, lfo1, lfo2, lfo3, osc,osc1,osc2;
Adsr              env1, env2;
uint8_t           counter = 0;
LadderFilter      flt, flt2;
//static ReverbSc   verb;
static DcBlock    blk[2];
volatile bool     env_state1 = 0;
volatile bool     env_state2 = 0;
float             CV5, cv8, cv4, rezKnob, lfoFreq, lfoKnob, env11, env22;
VoctCalibration   cali,cali2;
//Autowah           wah;
SmoothADCReader adcReader;
float           LFOLFO;
bool            env_state;
bool            switcher;
float           averageLfo;
Resonator       rez;

void            EnvelopeCallback(uint16_t** output, size_t size)
{

   

    for(size_t i = 0; i < size; i++)

    {
       


        output[0][i] = env11* 4095.0f;
        output[1][i] = env22 * 4095.0f;
        //output[1][i] = ((lfo2.Process() + 1.f) * .5f)
        //               + ((lfo2.Process() + 1.f) * .5f) * 4095.0f;
    }
}
float ffff;
bool  wahwah = 0;
// Function Definitions
void InitHardware()
{
    hw.Init();
    hw.SetAudioBlockSize(1);
    float sample_rate = hw.AudioSampleRate();

    env1.Init(sample_rate);
    env2.Init(sample_rate);
    adcReader.Init(sample_rate);
    flt2.Init(sample_rate);

    drv1.SetDrive(.289f);
    flt.Init(sample_rate);
    flt.SetInputDrive(1.8f);
    flt.SetPassbandGain(.2f);
    flt2.SetInputDrive(2.f);
    flt2.SetPassbandGain(.24f);

    osc.Init(sample_rate);
    osc1.Init(sample_rate);
    osc2.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc1.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc2.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    // verb.Init(sample_rate);
    //verb.SetFeedback(0.66f);
    //verb.SetLpFreq(5000.0f);

    blk[0].Init(sample_rate);
    blk[1].Init(sample_rate);

    lfo1.Init(sample_rate);
    lfo2.Init(sample_rate);

    rez.Init(.015, 24, sample_rate);
    rez.SetBrightness(.5f);
    rez.SetDamping(.5f);
    rez.SetStructure(.5f);
    lfo.SetAmp(1.f);
    lfo1.SetAmp(1.0f);
    lfo2.SetAmp(0.2f);

    // Set initial parameters for LFOs
    lfo1.SetFreq(.1f);
    lfo2.SetFreq(.0005f);

   cali.Record(.162f, .553f);
   cali2.Record(.158f , .550f );
}
float dalekKnob,bri;
void UpdateParameters()
{
    CV6 = hw.GetAdcValue(CV_6) +0.028f;
    CV7 = hw.GetAdcValue(CV_7) + 0.024f;
    CV8 = hw.GetAdcValue(CV_8) + 0.031f;

    env_state1 = hw.gate_in_1.State();
    env_state2 = hw.gate_in_2.State();


    lfoKnob   = 1.f - hw.GetAdcValue(ADC_10);
    bri   = 1.f - hw.GetAdcValue(ADC_9);
    dalekKnob = 1.f - hw.GetAdcValue(ADC_11);
    //float envknob       = 1.f - hw.GetAdcValue(ADC_10);

    //float envSize       = fmap(cv8, 200.f, 2300.f, Mapping::LINEAR);
    lfoFreq             = fmap(lfoKnob, 0.00011f, 8.f, Mapping::EXP);
    float att  = fmap(CV5, 0.0001f, .1f, Mapping::LINEAR);
    float att4          = fmap(cv4, 0.0001f, .1f, Mapping::LINEAR);

    float dec   = fmap(CV5, 0.1f, .51f, Mapping::LINEAR);
    float dec4   = fmap(cv4, 0.001f, .21f, Mapping::LINEAR);

    float rel = fmap(CV5, 0.3f, .01f, Mapping::LINEAR);
    float rel4 = fmap(cv4, 0.1f, .7f, Mapping::LINEAR);


    env1.SetAttackTime(att*att4, .95f);
    env1.SetTime(ADSR_SEG_DECAY, dec);
    env1.SetTime(ADSR_SEG_RELEASE, dec4+rel+att);
    env1.SetSustainLevel(.5f);


    env2.SetAttackTime(att*att, 1.25f);
    env2.SetTime(ADSR_SEG_DECAY, dec4+rel);
    env2.SetTime(ADSR_SEG_RELEASE, rel4);
    env2.SetSustainLevel(.5f);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    float dryL, dryR, note,note2;

    hw.ProcessAllControls();

    UpdateParameters();

    
        note2 = cali2.ProcessInput(cv8);

        float freq = mtof(cv8*4.f + 24.f);
        float freqRez =freq;


        //switcher=  hw.gate_in_2.Trig();  // bool  shTrig = hw.gate_in_1.Trig();
        //switcher=  hw.gate_in_2.Trig();
        //note2 = cali2.ProcessInput(cv8);


        for(size_t i = 0; i < size; ++i)

        {


            rez.SetFreq(freqRez);
            rez.SetStructure((cv4*.5f)+.4f);
            rez.SetDamping((CV5*.6f)+.2f);
            rez.SetBrightness((bri*.8f)+.2f);

            //osc2.SetFreq(freq2);
            float lfo1Freq = fmap(
                1.f - hw.GetAdcValue(lfoKnob), 0.001f, 12.f, Mapping::LINEAR);
            float lfo2Freq = lfo1Freq * 0.33333f;
            lfo1.SetFreq(lfo2Freq);
            lfo2.SetFreq(lfo1Freq);


            float lfo1Value = lfo1.Process();
            float lfo2Value = lfo2.Process();
            // Normalize the LFO values to the range [0, 1]
            float normalizedLfo1 = (lfo1Value + 1.0f) * 0.5f;
            float normalizedLfo2 = (lfo2Value + 1.0f) * 0.5f;

            // Average the normalized LFO values
            averageLfo = (normalizedLfo1 + normalizedLfo2) * 0.5f;

            env22 = env2.Process(env_state2);
            env11 = env1.Process(env_state1);

            osc.SetAmp(.9F);
            osc.SetFreq(averageLfo*180.f);

            //float driveCV = CV5 * .2f;

            //float driveCV = CV5 * .2f;
            float driveCV = CV5 * .15f;
            drv1.SetDrive(.28 + driveCV);

            float inL     = IN_R[i];
            float buffed1 = inL;
       float rezOut=     rez.Process(buffed1);

            float dalek   = osc.Process() *( inL*1.5f);



            dryL = rezOut;
            dryR = drv1.Process(dalek) ;

            // Send Signal to Reverb
       //  float   sendL = dryL * .6f;
           // float sendR = dryR * .65f;

            //verb.Process(sendL, sendR, &wetL, &wetR);

            // Dc Block
            //drl = blk[0].Process(wetL);
            //  wetR = blk[1].Process(wetR);

            // Out 1 and 2 are Mixed
            out[0][i] = (dryL);
            out[1][i] = (dryR);
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    InitHardware();

    hw.StartDac(EnvelopeCallback);


    // start callback
    hw.StartAudio(AudioCallback);


    while(1) {}
}
