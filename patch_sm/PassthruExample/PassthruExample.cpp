#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;


SampleHold samp1,samp2;


void EnvelopeCallback(uint16_t** output, size_t size)


{
    hw.ProcessAllControls();


    bool  trig1 = hw.gate_in_1.Trig();
    bool  trig2 = hw.gate_in_2.Trig();
    float slewKnobber1
        = fmap(hw.GetAdcValue(CV_8), .000001f, 12.11f, Mapping::LINEAR);
    float cvraw1 = hw.GetAdcValue(CV_6);

    float cvraw2 = hw.GetAdcValue(CV_7);


    for(size_t i = 0; i < size; i++)
    {
        float current_out1
            = samp1.Process(trig1, cvraw2, SampleHold::Mode::MODE_SAMPLE_HOLD);
        float current_out2 = samp2.Process(trig2, cvraw1,SampleHold::Mode::MODE_TRACK_HOLD);

        output[0][i] = (current_out1 * 4095.0f);

        output[1][i] = (current_out2 * 4095.0f);
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartDac(EnvelopeCallback);

    while(1) {}
}
