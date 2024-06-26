#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** Hardware object for the patch_sm */
DaisyPatchSM hw;

/** Switch object */
Switch toggle;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init();

    /* Initialize the switch
	 - We'll read the switch on pin B8
	*/
    toggle.Init(hw.B8);

    /** Loop forever */
    while(1)
    {
        /** Debounce the switch */
        toggle.Debounce();

        /** Get the current toggle state */
        bool state = toggle.Pressed();

        /** Set the onboard led to the current state */
        hw.SetLed(state);
    }
}
