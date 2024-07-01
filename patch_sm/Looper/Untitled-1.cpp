#include "daisy_patch_sm.h"
#include "daisysp.h"
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
DaisyPatchSM hw;
Switch       button1;
Switch       button2, button3;
//#define LoopKnobber CV_1
#define cvLFOamp CV_8
#define relCV CV_4
//#define LoopKnobber CV_5
#define freq1Knob ADC_9
#define cvLFOFreq CV_8
//#define freq1Knob CV_8
int      loopLength1 = 0;
int      loopLength2 = 0;
ReverbSc reverb;

#define LoopKnobber ADC_11
#define verbKnob ADC_10
#define relKnob CV_6
#define attck1Knob ADC_12
float count = 0.f;

GPIO my_led;

// Initialize it to pin D1 as an OUTPUT
static float send;

Adsr env1;
Adsr env2;


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
constexpr float  kGainReduction = 0.75f;
constexpr int    kHoldTimeMS    = 1000;
constexpr size_t kBuffSize      = 48000 * 100; // 45 seconds at 48kHz
Looper           looper1, looper2;
static DcBlock   blk[2];

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

int   dot                  = 0;
float presetAttackTimes[]  = {0.0005f,
                             0.0006f,
                             0.001f,
                             0.001f,
                             0.0015f,
                             0.0016f,
                             0.002f,
                             0.009f,
                             0.0114f,
                             0.0185,
                             0.055f,
                             0.01f,

                             .140f};
float presetDecayTimes[]   = {0.005f,
                            0.015f,
                            0.02f,
                            0.03f,
                            0.042f,
                            0.051f,
                            0.08f,
                            0.11f,
                            0.14,
                            0.23f,
                            0.225f,
                            0.2,
                            .25f};
float presetReleaseTimes[] = {0.09f,
                              0.1f,
                              0.12f,
                              0.132f,
                              0.145f,
                              0.16f,
                              0.18f,
                              0.2f,
                              0.23f,
                              0.3f,
                              0.4f,
                              .6f,
                              .68f};

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

void envelopePresets(int currentPreset, Adsr& env)
{
    env.SetAttackTime(presetAttackTimes[currentPreset]);
    env.SetTime(ADSR_SEG_DECAY, presetDecayTimes[currentPreset]);
    env.SetTime(ADSR_SEG_RELEASE, presetReleaseTimes[currentPreset]);
}
sampHoldStruct sampHolds[2];


bool modeSwitch = 0;
void EnvelopeCallback(uint16_t** output, size_t size)
{
    float lfo1Freq
        = fmap(1.f - hw.GetAdcValue(verbKnob), 0.01f, 12.f, Mapping::LOG);
    float lfo2Freq = lfo1Freq * 0.25f;

    bool env_state1 = hw.gate_in_1.State();
    //  float cvraw1     = hw.GetAdcValue(CV_1) + 0.028;
    //  float cvraw7     = hw.GetAdcValue(CV_7) + 0.029;
    float cv8 = hw.GetAdcValue(CV_8) + 0.028;
    // float cvraw6     = hw.GetAdcValue(CV_6) + 0.032;
    float cv2 = hw.GetAdcValue(CV_6) + 0.028f;


    float rel1           = 1.f - (hw.GetAdcValue(ADC_11) + 0.028f);
    int   cv1PresetIndex = static_cast<int>(cv8 * 13.0f);
    int   cv2PresetIndex = static_cast<int>(cv2 * 13.f);

    envelopePresets(cv1PresetIndex, env1);
    envelopePresets(cv2PresetIndex, env2);

    float loopKnob1 = hw.GetAdcValue(ADC_10) * 5.f;
    looperLength(loopKnob1, loopLength1);

    float loopKnob2 = hw.GetAdcValue(ADC_9) * 5.f;
    looperLength(loopKnob2, loopLength2);


    for(size_t i = 0; i < size; i++)

    {
        lfo1.SetFreq(lfo1Freq);
        lfo2.SetFreq(lfo2Freq);


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
    if(button1.RisingEdge())
    {
        // If already looping, trigger recording.
        if(isLooping1)
        {
            looper1.TrigRecord();
        }
        // If not looping, set the looper as armed.
        else if(!isLooping1)
        {
            isArmed1 = true;
        }
    }


    if(button1.TimeHeldMs() > (500.f))
    {
        looper1.Clear();
        isLooping1    = false;
        triggerCount1 = 0;
        isArmed1      = false;
    }


    if(button2.RisingEdge())
    {
        // If already looping, trigger recording.
        if(isLooping2)
        {
            looper2.TrigRecord();
        }
        // If not looping, set the looper as armed.
        else if(!isLooping1)
        {
            isArmed2 = true;
        }
    }
    if(button2.TimeHeldMs() > (500.f))
    {
        looper2.Clear();
        isLooping2    = false;
        triggerCount2 = 0;
        isArmed2      = false;
    }

    if(button3.Pressed() && button2.Pressed())
    {
        looper2.Clear();
        isLooping2    = false;
        triggerCount2 = 0;
        isArmed2      = false;

        looper1.Clear();
        isLooping1    = false;
        triggerCount1 = 0;
        isArmed1      = false;
    } // CLEAR LOOP

    if(button3.RisingEdge()) // CLEAR LOOP
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
        // Clear the loop or change looper mode if both button1 and button2 are pressed.
    }
    my_led2.Write(looper2.Recording());
    my_led1.Write(looper1.Recording());

    if(looper2.IsNearBeginning())
    {
        my_led2.Write(true);
    }

    if(looper1.IsNearBeginning())

    {
        my_led1.Write(true);
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
    void AudioCallback(
        AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
    {
        hw.ProcessAllControls();
        button1.Debounce();
        button2.Debounce();
        button3.Debounce();
        //float wetSender = 1.0f - hw.GetAdcValue(verbKnob);


        buttons();
        gateTrigger = hw.gate_in_1.Trig();
        if(gateTrigger)
        {
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
                    looper1.TrigRecord();
                    triggerCount1 = 0;
                    isArmed1      = false;
                }
            }
            else if(isArmed2)
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

        for(size_t i = 0; i < size; i++)
        {
            rez.SetFreq(freqRez);
            rez.SetStructure((cv4 * .5f) + .4f);
            rez.SetDamping((CV5 * .6f) + .2f);
            rez.SetBrightness((bri * .8f) + .2f);
            float inL = IN_R[i] * 0.65f;

            float loop1    = looper1.Process(inL);
            float loop2    = looper2.Process(inL);
            float loop     = loop1 + loop2;
            float softLoop = SoftClip(loop);


            OUT_R[i] = softLoop;
            OUT_L[i] = softLoop;
        }
    }


    int main(void)
    {
        // Initialize and check for errors
        hw.Init();


        looper1.Init(buffer_1, kBuffSize);
        looper2.Init(buffer_2, kBuffSize);
        env1.Init(hw.AudioSampleRate());

        float sample_rate = hw.AudioSampleRate();

        ///verb.Init(sample_rate);
        /// verb.SetFeedback(0.85f);
        /// verb.SetLpFreq(18000.0f);
        //flt.Init(sample_rate);
        lfo1.Init(sample_rate);
        lfo2.Init(sample_rate);
        lfo1.SetFreq(1.f); // Set initial frequency for LFO1
        lfo2.SetAmp(.75f);
        lfo1.SetWaveform(Oscillator::WAVE_TRI);
        lfo2.SetWaveform(Oscillator::WAVE_SIN);

        lfo1.SetAmp(1.);
        blk[0].Init(sample_rate);
        // lfo2.SetAmp(1.);
        //lfo3.Init(sample_rate);
        //lfo2.SetWaveform(Oscillator::WAVE_TRI);
        //  lfo1.SetWaveform(Oscillator::WAVE_TRI);
        reverb.Init(hw.AudioSampleRate());
        my_led.Init(hw.D1, GPIO::Mode::OUTPUT);

        hw.StartDac(EnvelopeCallback);
        // hw.StartDac(EnvelopeCallback);

        button1.Init(hw.D1);
        button2.Init(hw.D10);
        button3.Init(hw.D5);
        //button4.Init(hw.D9);
        looper1.SetMode(Looper::Mode::ONETIME_DUB);
        looper2.SetMode(Looper::Mode::ONETIME_DUB);

        // Start the audio callback
        hw.StartAudio(AudioCallback);

        // loop forever
        while(1) {}
    }
