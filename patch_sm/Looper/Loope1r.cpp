#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "pooper.h"
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
static Pooper         loop;
static const uint32_t kBufferLengthSec     = 60 * 5;
static const uint32_t kSampleRate          = 48000;
static const size_t   kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buffer[kBufferLenghtSamples];
DaisyPatchSM               hw;


float slicerStart[16] = {0.0625f,
                         .125f,
                         .1875f,
                         .25f,
                         0.3125f,
                         0.375f,
                         .4375,
                         .5f,
                         .5625f,
                         .625f,
                         .6875f,
                         .75f,
                         .8125f,
                         .9375f};

float slicerLength[5] = {.0625, .125f, .25f, .5f, 1.f};
float cv4, cv5, cv8, loopLengthKnob, loopStartKnob;
bool  trig1, trig2;
void  UpdateParameters()
{
    cv4 = hw.GetAdcValue(CV_4) + 0.018f;
    cv5 = hw.GetAdcValue(CV_5) + 0.0118f;
    cv8 = hw.GetAdcValue(CV_8) + 0.018f;

    trig1 = hw.gate_in_1.Trig();
    trig2 = hw.gate_in_2.Trig();

    loopStartKnob  = 1.f - hw.GetAdcValue(ADC_12);
    loopLengthKnob = 1.f - hw.GetAdcValue(ADC_11);
    //float envknob       = 1.f - hw.GetAdcValue(ADC_10);

    // Check for the rising edge on button1 to either trigger recording or arm the looper.
}
bool record_on     = 0;
bool isArmed1      = 0;
bool isLooping1    = 0;
int  triggerCount1 = 0;
bool gate          = 0;
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    int loop_start = static_cast<int>(
        loopStartKnob * 15.f); // Ensure the index is within 0 to 15
    int loop_length = static_cast<int>(
        loopLengthKnob * 4.f); // Ensure the index is within 0 to 4

    // Ensure loop_start and loop_length are within valid range
    loop_start  = std::max(0, std::min(loop_start, 15));
    loop_length = std::max(0, std::min(loop_length, 4));
    //{
    isArmed1 = true;
    // }

    float LoopLength = slicerStart[loop_start] + slicerLength[loop_length];
    if(LoopLength > 1.0f)
        LoopLength = 1.f;

    loop.SetLoop(slicerStart[loop_start], LoopLength);
    // Toggle record
    loop.SetRecording(record_on);


    if(trig1)
    {
        if(!gate)
        {
            if(!record_on)
            {
                record_on = 0;

                isLooping1    = true;
                triggerCount1 = 1;
            }
            else if(++triggerCount1 > 8)
            {
                record_on     = 0;
                triggerCount1 = 0;
                isArmed1      = false;
                gate          = 1;
            }
        }
    }
    for(size_t i = 0; i < size; i++)
    {
        float looper_out = loop.Process(IN_R[i] * 0.75f);
        OUT_R[i]         = SoftClip(looper_out);
    }
}
int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    loop.Init(buffer, kBufferLenghtSamples);
    while(1) {}
}
