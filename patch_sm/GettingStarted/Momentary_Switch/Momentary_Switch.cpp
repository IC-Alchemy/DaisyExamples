#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** Hardware object for the patch_sm */
DaisyPatchSM hw;

/** Switch object */
Switch button;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init();

    /* Initialize the switch
	 - We'll read the switch on pin B7
	*/
    button.Init(hw.B7);

    /** loop forever */
    while(1)
    {
        /** Debounce the switch */
        button.Debounce();

        /** Get the current button state */
        bool state = button.Pressed();

        /** Set the onboard led to the current state */
        hw.SetLed(state);
    }
}
