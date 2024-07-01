#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "./bezier.h"
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
bezier::Bezier<3> cubicBezier({{0, 0}, {1.91, 1.73}, {-.46, 0.73}, {1, 0}});
bezier::Point     p;
Oscillator        osc;
DaisyPatchSM hw;
void         EnvelopeCallback(uint16_t** output, size_t size)
{
    hw.ProcessAllControls();

;
    float LFOLFO, sampledLFO;
    bool  env_state2 = hw.gate_in_2.State();
    bool  shTrig     = hw.gate_in_2.Trig();
    float cv8        = hw.GetAdcValue(CV_8) + 0.028;
    float cv4        = hw.GetAdcValue(CV_4) + 0.028;
    float x          = hw.GetAdcValue(ADC_12);
    float y          = hw.GetAdcValue(ADC_11);
    float yy        = hw.GetAdcValue(ADC_10);
    float xx         = hw.GetAdcValue(CV_5) + 0.028;


    //  float adc9       = 1.f - hw.GetAdcValue(ADC_10);
    //if(cv8 < 0.f)
    //{
    //    cv8 *= -1.f;
    //}
    //float slew = fmap(adc9, 110.001f, .1f, Mapping::LOG);

   // adcReader.SetFreq(100.f);
   // LFOLFO = adcReader.Process(cv8, 3.f, shTrig);


    for(size_t i = 0; i < size; i++)

    {
     //   cubicBezier.translate((xx + x), yy + y);

        // Get coordinate values for a single axis. Currently only supports 2D.
        double value2,value1;
        value1 = cubicBezier.valueAt(cv4, 0); // 220 (x-coordinate at t = 1)
        value2 = cubicBezier.valueAt(cv4,
                                     1); // 157.1875 (y-coordinate at t = 0.75)
        //lfo1.SetFreq(lfo1Freq);
        //lfo2.SetFreq(lfo2Freq);


        output[0][i] = value1 * 4095.0f;
        output[1][i] = value2 * 4095.0f;
        //output[1][i] = ((lfo2.Process() + 1.f) * .5f)
        //               + ((lfo2.Process() + 1.f) * .5f) * 4095.0f;
    }
}

int main(void)
{
    hw.Init();
    osc.Init(48000.f);
    hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartDac(EnvelopeCallback);
    while(1)
    {
        // Create a cubic bezier with 4 points. Visualized at https://www.desmos.com/calculator/fivneeogmh


    
   
    }
}
