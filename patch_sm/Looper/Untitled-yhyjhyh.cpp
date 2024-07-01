#include "sync.h"
#include "daisy_patch_sm.h"
#include "daisysp.h"
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
DaisyPatchSM hw;
Switch       button1, button2, button3;
float        rel1;

int loopLength1 = 0;
int loopLength2 = 0;
//ReverbSc reverb;
float frac;


float     count = 0.f;
Resonator rez;
GPIO      my_led1;
GPIO      my_led2;
float     rezOn = 0.f;

const uint32_t kMicroSecInSec = 1e6;
int            clock_pin      = 0;
Adsr           env1;
Adsr           env2;
float          notes[] = {0.f,  5.f,  7.f,  0.f,  12.f, 5.f,  12.f, 5.f,  0.f,
                 7.f,  12.f, 17.f, 12.f, 19.f, 22.f, 17.f, 12.f, 19.f,
                 24.f, 17.f, 12.f, 19.f, 12.f, 7.f,  5.f};
uint8_t        scale   = 0;

////SyntheticBassDrum  bd;
//SyntheticSnareDrum snr;
Oscillator lfo1, lfo2, lfo3;
int        freq_multiplier = 1;
bool       gateTrigger     = 0;
bool       flip1           = 0;
bool       flip2           = 0;
float      decay           = 0.001f;
bool       flip3           = 0;
bool       flip4           = 0;
// Define these constants for better management of magic numbers
constexpr float     kGainReduction = 0.75f;
constexpr int       kHoldTimeMS    = 1000;
constexpr size_t    kBuffSize      = 48000 * 100; // 45 seconds at 48kHz
Looper              looper1, looper2;
static DcBlock      blk[2];
float               randy1, randy2, randy3, randy21, randy12;
int                 seqCount = 0;
float DSY_SDRAM_BSS buffer_1[kBuffSize];
float DSY_SDRAM_BSS buffer_2[kBuffSize];

// Global or static variables to maintain state
volatile bool isLooping1    = false;
static int    triggerCount1 = 0;
volatile bool isArmed1      = false;
volatile bool isLooping2    = false;
static int    triggerCount2 = 0;
volatile bool isArmed2      = false;
int           loopLength    = 8;


/*
struct sampHoldStruct
{
    SampleHold       sampHold;
    SampleHold::Mode mode;
    float            output;

    void Process(bool trigger, float input)
    {
        output = sampHold.Process(trigger, input, mode);
    }
};


sampHoldStruct sampHolds[2];
*/


int dot = 0;


void IncrementAndWrapInt(int& count, int maxSize)
{
    count++;
    if(count > maxSize)
    {
        count = 0;
    }
}

void IncrementAndWrapFloat(float& count,
                           float  minSize,
                           float  maxSize,
                           float  incrementSize)
{
    count += incrementSize;
    if(count > maxSize)
    {
        count = minSize;
    }
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

struct sampHoldStruct
{
    SampleHold       sampHolds;
    SampleHold::Mode mode;
    float            output;

    void Process(bool trigger, float input, SampleHold::Mode mode)
    {
        output = sampHolds.Process(trigger, input, mode);
    }
};


sampHoldStruct sampHolds[2];


bool  modeSwitch = 0;
float structure, loopKnob1, loopKnob2, lfofreq;
bool  switchy;

void EnvelopeCallback(uint16_t** output, size_t size)
{
    structure = 1.f - hw.GetAdcValue(ADC_11);

    //  float cvraw1     = hw.GetAdcValue(CV_1) + 0.028;
    //  float cvraw1     = hw.GetAdcValue(CV_1) + 0.028;
    bool env_state1 = hw.gate_in_1.State();
    bool env_state2 = hw.gate_in_2.State();
    // bool env_state2 = hw.gate_in_2.State();
    //  float cvraw1     = hw.GetAdcValue(CV_1) + 0.028;
    //  float cvraw7     = hw.GetAdcValue(CV_7) + 0.029;
    // float cv8 = hw.GetAdcValue(CV_8) + 0.028;
    // float cvraw6     = hw.GetAdcValue(CV_6) + 0.032;
    float cv4 = hw.GetAdcValue(CV_4) + 0.028f;

    // rel1 = 1.f - (hw.GetAdcValue(ADC_11));

    loopKnob1 = hw.GetAdcValue(ADC_10);
    looperLength(loopKnob1 * 5.f, loopLength1);

    loopKnob2 = 1.f - hw.GetAdcValue(ADC_9);


    env1.SetAttackTime(0.01f * randy2 + 0.005);
    env1.SetTime(ADSR_SEG_DECAY, (cv4 * .2f) +0.016f + randy12);
    env1.SetTime(ADSR_SEG_RELEASE, (cv4 * .4f) + .08f + (randy2) + randy12);

    env2.SetAttackTime(0.007f);
    env2.SetTime(ADSR_SEG_DECAY, (cv4 * .4f) - randy1 + randy3 + 0.001f);
    env2.SetTime(ADSR_SEG_RELEASE, (cv4 * .6f) + randy1 * 2.f + randy3 + 0.01f);


    // looperLength(loopKnob2 * 5.f, loopLength2);


    for(size_t i = 0; i < size; i++)

    {
        output[0][i] = env1.Process(env_state1) * 4095.0f;
        output[1][i] = env2.Process(env_state2) * 4095.0f;
    }
}

/**
 * Handles button interactions for triggering different looper functionalities.
 * 
 * This function checks the state of two buttons to control a looper's behavior.
 * It includes functionalities for starting/stopping recording, arming the looper,
 * clearing the loop, and cycling through looper modes.
 */
void buttons()
{
    // Check for the rising edge on button1 to either trigger recording or arm the looper.
    if(button2.RisingEdge())
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


    if(button2.TimeHeldMs() > (500.f))
    {
        looper1.Clear();
        isLooping1    = false;
        triggerCount1 = 0;
        isArmed1      = false;
    }


    if(button1.TimeHeldMs() > (400.f))
    {
        switchy = !switchy;
        if(switchy)
        {
            rezOn = 1.f;
            my_led2.Write(true);
        }
        else
        {
            rezOn = 0.f;
            my_led2.Write(false);
        }
    }


    if(button1.RisingEdge())
    {
        looper1.ToggleReverse();
    }

    if(button3.RisingEdge())
    {
        looper1.IncrementMode();
        //looper2.IncrementMode();


        // If the new mode is REPLACE or NORMAL, increment mode again.
        if((looper1.GetMode() == Looper::Mode::REPLACE)
           || (looper1.GetMode() == Looper::Mode::NORMAL))
        {
            looper1.IncrementMode();
            //  looper2.IncrementMode();
        }
    }
    //   my_led2.Write(looper2.Recording());
    my_led1.Write(looper1.Recording());


    if(looper1.IsNearBeginning())

    {
        my_led1.Write(true);
    }
}
/**
 * Processes audio input and applies effects based on user interactions and control states.
 * 
 * This function is designed to be called as an audio callback, which processes blocks of audio
 * samples. It handles button debouncing, gate triggering for recording, and applies a looping effect
 * with variable feedback based on external controls. Additionally, it applies a reverb effect to the
 * processed audio.
 * 
 * @param in Input buffer containing the incoming audio samples.
 * @param out Output buffer where the processed audio samples are stored.
 * @param size The number of audio samples in the input and output buffers.
 */
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    button1.Debounce();
    button2.Debounce();
    button3.Debounce();


    buttons();
    gateTrigger       = hw.gate_in_1.Trig();
    bool gateTrigger2 = hw.gate_in_2.Trig();


    if(gateTrigger2)
    {
        randy2  = frac * rand() * .4f * loopKnob2;
        randy12 = frac * rand() * .2f;
        scale++;
        if(scale > 24 - 1)
        {
            scale = 0;
        }
    }
        gateTrigger       = hw.gate_in_1.Trig();

    if(gateTrigger)   //  If we recieve a trigger
    {
      

        if(isArmed1)    //  if the arm button has been pressed
        {
            if(!isLooping1)    // if we are not already looping 
            { 
                looper1.TrigRecord();      // begin looping
                isLooping1    = true;  //  we are looping  
                triggerCount1 = 1;   //  make sure this is counted as a step 
            }
            //  if we are looping count up, and if the count is larger than loop length 
            else if(++triggerCount1 > loopLength1)   
            {
                //  end the loop   This sets the loops length from now on, until cleared
                looper1.TrigRecord();  
                triggerCount1 = 0;  //  reset the counter
                isArmed1      = false;  //  reset the arm button
            }
        }

        /*        if(isArmed2)
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
    */
    }
    for(size_t i = 0; i < size; i++)
    {
        rez.SetFreq(mtof(notes[scale] + 48.f));
        rez.SetStructure((structure));
        rez.SetDamping((loopKnob1 * .6f));
        rez.SetBrightness((loopKnob2 * .9f) + randy1);
        float inL  = IN_L[i];
        float inR  = inL;
        float rezo = rez.Process(inR);

        float loop1 = looper1.Process(inL);
        //float loop2    = looper2.Process(inR);
        float softLoop = SoftClip((loop1 + (rezo * rezOn)) * .75);


        OUT_R[i] = softLoop;
        OUT_L[i] = softLoop;
        ;
    }
}

int main(void)
{
    // Initialize and check for errors
    hw.Init();

    // Start the audio callback
    hw.StartAudio(AudioCallback);
    hw.StartDac(EnvelopeCallback);

    looper1.Init(buffer_1, kBuffSize);
    // looper2.Init(buffer_2, kBuffSize);
    env1.Init(hw.AudioSampleRate());
    env2.Init(hw.AudioSampleRate());

    float sample_rate = hw.AudioSampleRate();

    ///verb.Init(sample_rate);
    /// verb.SetFeedback(0.85f);
    /// verb.SetLpFreq(18000.0f);

    lfo1.Init(sample_rate);
    lfo2.Init(sample_rate);
    lfo1.SetFreq(1.f); // Set initial frequency for LFO1
    lfo2.SetAmp(.75f);
    lfo1.SetWaveform(Oscillator::WAVE_TRI);
    lfo2.SetWaveform(Oscillator::WAVE_SIN);

    lfo1.SetAmp(1.);
    blk[0].Init(sample_rate);


    my_led1.Init(hw.D2, GPIO::Mode::OUTPUT);
    my_led2.Init(hw.D3, GPIO::Mode::OUTPUT);


    rez.Init(.015, 24, sample_rate);
    rez.SetBrightness(.5f);
    rez.SetDamping(.5f);
    rez.SetStructure(.5f);
    button1.Init(hw.D1);
    button2.Init(hw.D10);
    button3.Init(hw.D5);
    looper1.SetMode(Looper::Mode::ONETIME_DUB);
    looper2.SetMode(Looper::Mode::ONETIME_DUB);
    frac = 1.f / RAND_MAX;


    while(1) {}
}
