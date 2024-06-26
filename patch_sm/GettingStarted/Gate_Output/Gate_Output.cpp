#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** Hardware object for the patch_sm */
DaisyPatchSM hw;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init();

    /** Loop forever */
    while(1)
    {
        /** Set the gate high */
        dsy_gpio_write(&hw.gate_out_1, true);

        /** Wait 250 ms */
        hw.Delay(250);

        /** Set the gate low */
        dsy_gpio_write(&hw.gate_out_1, false);

        /** Wait 250 ms */
        hw.Delay(250);
    }
}
