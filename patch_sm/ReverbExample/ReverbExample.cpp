#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM hw;
ReverbSc     reverb;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAnalogControls();

    /** Update Params with the four knobs */
    float time_knob = hw.GetAdcValue(CV_1);
    float time      = fmap(time_knob, 0.3f, 0.99f);

    float damp_knob = hw.GetAdcValue(CV_2);
    float damp      = fmap(damp_knob, 1000.f, 19000.f, Mapping::LOG);

    float in_level = hw.GetAdcValue(CV_3);

    float send_level = hw.GetAdcValue(CV_4);

    reverb.SetFeedback(time);
    reverb.SetLpFreq(damp);

    for(size_t i = 0; i < size; i++)
    {
        float dryl  = IN_L[i] * in_level;
        float dryr  = IN_R[i] * in_level;
        float sendl = IN_L[i] * send_level;
        float sendr = IN_R[i] * send_level;
        float wetl, wetr;
        reverb.Process(sendl, sendr, &wetl, &wetr);
        OUT_L[i] = dryl + wetl;
        OUT_R[i] = dryr + wetr;
    }
}

int main(void)
{
    hw.Init();
    reverb.Init(hw.AudioSampleRate());
    hw.StartAudio(AudioCallback);
    while(1) {}
}
