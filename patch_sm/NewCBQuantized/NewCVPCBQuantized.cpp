#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
Overdrive  drv;
Wavefolder fld;
// Constants
constexpr float    MAX_FREQ            = 35.0f;  // 30ms period
constexpr float    MIN_FREQ            = 0.333f; // 3s period
constexpr uint32_t DEFAULT_BPM         = 80;
#define MAX_SIZE (48000 * 60 * 2) // 5 minutes of floats at 48 khz

constexpr size_t   BUFFER_SIZE         = 48000 * 100; // 45 seconds at 48kHz
constexpr int      LOOP_LENGTH_DEFAULT = 8;
constexpr int      BPM_MIN             = 30;
constexpr int      BPM_MAX             = 240;
float              current_out1;
float              current_out2;
// DaisyPatchSM hardware
DaisyPatchSM hw;

// Oscillators and Envelopes
Oscillator lfo1, lfo2;
Adsr       env1, env2;

// Switches
Switch button1, button2, button3;

// Loopers
Looper looper1, looper2;

// DC Block
static DcBlock blk[2];

// Buffers
float DSY_SDRAM_BSS buffer_1[MAX_SIZE];
float DSY_SDRAM_BSS buffer_2[MAX_SIZE];

// Flags and Counters
volatile bool isLooping1 = false;
volatile bool isArmed1   = false;
volatile bool isLooping2 = false;
volatile bool isArmed2   = false;
bool          lfoFlag    = false;
bool          lfoFlag1    = false;
bool          trig1, trig2, trigenv1, trigenv2, dualEnvFlag;
bool          tapping       = false;
bool          averaging     = false;
bool          use_tt        = false;
int           triggerCount1 = 0;
int           triggerCount2 = 0;
int           loopLength    = LOOP_LENGTH_DEFAULT;
int           loopLength1   = 0;
int           loopLength2   = 0;
int           count         = 0;
int           MAX_COUNT, MIN_COUNT;
int           seqCount = 0;
uint32_t      clockBPM = DEFAULT_BPM;

// Parameters
Parameter freq_k;
float     knob1, knob2, knob3, knob4, cv5, cv6, cv7, cv8;
float     prev_k1 = -1.0f;
float     BPS;
float     freq_tt;
float     randy1, randy2, randy3;

// Function Prototypes
void tap_tempo();
void buttons();
void envelopeCallback(uint16_t** output, size_t size);
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size);


void tap_tempo()
{
    if(tapping)
    {
        count++;

        if((trig1) && count > MIN_COUNT)
        {
            if(averaging) //3rd plus tap
                freq_tt = 0.6 * (freq_tt)
                          + 0.4 * (BPS / count); // Weighted Averaging
            else
            {
                //2nd tap
                freq_tt   = BPS / count; //frequency = BPS/count
                averaging = true;
            }
            clockBPM = freq_tt;

            count = 0;
        }
        else if(count == MAX_COUNT)
        { // After 1/MIN_FREQ seconds no tap, reset values
            count     = 0;
            tapping   = false;
            averaging = false;
        }
    }
    else if(trig1) //1st tap
        tapping = true;
}

void looperLength(float mode, int& loopLength)
{
    if(mode < .5f)
    {
        loopLength = 64;
    }
    else if(mode < 1.5f)
    {
        loopLength = 16;
    }
    else if(mode < 2.75f)
    {
        loopLength = 8;
    }
    else if(mode < 4.5f)
    {
        loopLength = 4;
    }
    else if(mode < 5.0f)
    {
        loopLength = 2;
    }
}


void buttons()
{
    button1.Debounce();
    button2.Debounce();
    button3.Debounce();
    // Check for the rising edge on button1 to either trigger recording or arm the looper.
    if(button1.RisingEdge() && !button3.Pressed())
    {
        // If already looping, trigger recording.
        if(isLooping1)
        {
            looper1.TrigRecord();
        }
        // If not looping, set the looper as armed.
        else
        {
            isArmed1 = true;
        }
    }

    if(button2.RisingEdge() && !button3.Pressed() )
    {
        // If already looping, trigger recording.
        if(isLooping2)
        {
            looper2.TrigRecord();
        }
        // If not looping, set the looper as armed.
        else
        {
            isArmed2 = true;
        }
    }

if (button3.Pressed() && button2.RisingEdge())

{
    looper1.IncrementMode();
    looper2.IncrementMode();


    // If the new mode is REPLACE or NORMAL, increment mode again.
    if((looper1.GetMode() == Looper::Mode::REPLACE)
       || (looper1.GetMode() == Looper::Mode::NORMAL))
    {
        looper1.IncrementMode();
        looper2.IncrementMode();
    }
}
if(button3.Pressed()  && button1.RisingEdge())
{
    lfoFlag = !lfoFlag;
}


    if(button1.TimeHeldMs() > (500.f))
    {
        looper1.Clear();
        isLooping1    = false;
        triggerCount1 = 0;
        isArmed1      = false;
    }
    if(button2.TimeHeldMs() > (500.f))
    {
        looper2.Clear();
        isLooping2    = false;
        triggerCount2 = 0;
        isArmed2      = false;
    }
  
    if(button3.TimeHeldMs() > (1000.f))
    {
        looper1.Clear();
        looper2.Clear();
    }
    //   my_led2.Write(looper2.Recording());
}


float lfoSpeed[] = {1.f / 256.f,
                    1.f / 128.f,
                    1.f / 64.f,
                    1.f / 48.f,
                    1.f / 32.f,
                    1.f / 24.f,
                    1.f / 16.f,
                    1.f / 12.f,
                    1.f / 8.f,
                    1.f / 6.f,
                    1.f / 4.f,
                    1.f / 3.f,
                    1.f / 2.f,
                    1.f,
                    1.25f,
                    1.5f,
                    1.75f,
                    2.f};
void  ReadStuff()
{
    hw.ProcessAllControls();
    button1.Debounce();
    button2.Debounce();
    button3.Debounce();
    knob1 = hw.GetAdcValue(CV_1);
    knob2 = hw.GetAdcValue(CV_2);
    knob3 = hw.GetAdcValue(CV_3);
    knob4 = hw.GetAdcValue(CV_4);

    cv5 = hw.GetAdcValue(CV_5);
    cv6 = hw.GetAdcValue(CV_6);
    cv7 = hw.GetAdcValue(CV_7);
    cv8 = hw.GetAdcValue(CV_8);


    looperLength(knob4 * 5.f, loopLength1);
    looperLength(knob2 * 5.f, loopLength2);


    if(lfoFlag)
    {
        int lfoShape1 = static_cast<int>((knob3 *  5) + 0.5f);
        lfo1.SetWaveform(lfoShape1);
        int lfoShape2 = static_cast<int>((knob1 * 5) + 0.5f);
        lfo2.SetWaveform(lfoShape2);
    }
    else
    {
        int lfoFreq1 = static_cast<int>((knob3 * 17) + 0.5f);
        lfo1.SetFreq(clockBPM * .125f * lfoSpeed[lfoFreq1]);
        int lfoFreq2 = static_cast<int>((knob1 * 17) + 0.5f);
        lfo2.SetFreq(clockBPM * .125f * lfoSpeed[lfoFreq2]);
    }
}
void envelopeCallback(uint16_t** output, size_t size)
{

    tap_tempo();


    //   dsy_gpio_write(&hw.gate_out_1, trigenv1);
    //  dsy_gpio_write(&hw.gate_out_2, trigenv2);

    for(size_t i = 0; i < size; i++)
    {
        //  float sh1 = samp1.Process(
        //      trig1, fabsf(cv7), SampleHold::Mode::MODE_SAMPLE_HOLD);
        //   float sh2
        //      = samp2.Process(trig2, fabsf(cv8), SampleHold::Mode::MODE_TRACK_HOLD);

         

        output[0][i] = (lfo1.Process() * 4095.0f);

        output[1][i] = (lfo2.Process() * 4095.0f);
    }
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    ReadStuff();
    buttons();

    trig1 = hw.gate_in_1.Trig();
    trig2 = hw.gate_in_2.Trig();
    if(trig1 || trig2)
    {
        //  randy1 = frac * rand() * .16f * loopKnob2;
        // randy2 = frac * rand() * .2f * loopKnob2;

        // randy3 = frac * rand() * loopKnob1 * .3f;

        if(isArmed1)
        {
            if(!isLooping1)
            {

                looper1.TrigRecord();
                isLooping1    = true;
                triggerCount1 = 1;
            }
            else if(++triggerCount1 > loopLength1)
            {
                lfo1.Reset();
                lfo2.Reset();
                looper1.TrigRecord();
                triggerCount1 = 0;
                isArmed1      = false;
            }
        }

        if(isArmed2)
        {
            if(!isLooping2)
            {
                looper2.TrigRecord();
                isLooping2    = true;
                triggerCount2 = 1;
            }
            else if(++triggerCount2 > loopLength2)
            {
                looper2.TrigRecord();
                triggerCount2 = 0;
                isArmed2      = false;
            }
        }
    }

  //  drv.SetDrive(.25f + cv8 * .15f);
    for(size_t i = 0; i < size; i++)

    {
        float inL   = IN_R[i];
        float dryL  = inL;
        float multL = dryL;
        //float inR   = IN_L[i];

        float loop1    = looper1.Process(inL);
        float loop2    = looper2.Process(multL);


        float poo = SoftClip(((loop1 + loop2) * .75f));
        OUT_R[i]  = poo;
        OUT_L[i]  = poo;
}

}
int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    hw.StartDac(envelopeCallback);
    float samplerate = hw.AudioSampleRate();

    //  env1.Init(samplerate);
    // env2.Init(samplerate);

    looper1.Init(buffer_1, BUFFER_SIZE);
    looper2.Init(buffer_2, BUFFER_SIZE);
    looper1.SetMode(Looper::Mode::ONETIME_DUB);
    looper2.SetMode(Looper::Mode::ONETIME_DUB);
    lfo1.Init(samplerate);
    lfo1.SetAmp(1.f);
    lfo2.Init(samplerate);
    lfo2.SetAmp(1.f);
    loopLength1 = 8;

    loopLength2 = 8;
    button1.Init(DaisyPatchSM::D5, hw.AudioCallbackRate() / 48);
    button2.Init(DaisyPatchSM::D4, hw.AudioCallbackRate() / 48);
    button3.Init(DaisyPatchSM::D3, hw.AudioCallbackRate() / 48);
    BPS = hw.AudioSampleRate()
          / hw.AudioBlockSize(); // Blocks per second = sample_rate/block_size
    MAX_COUNT = round(BPS / MIN_FREQ); // count = BPS/frequency
    MIN_COUNT = round(BPS / MAX_FREQ);
    while(1) {}
}
