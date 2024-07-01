/** Example of DAC Output using basic Polling implementation
 *  
 *  This will use A8 to demonstrate DAC output
 *  This will generate a ramp wave from 0-3v3
 * 
 *  A7 will output a ramp, and output A8 will output an inverted ramp
 */
#include "daisy_seed.h"

/** This prevents us from having to type "daisy::" in front of a lot of things. */
using namespace daisy;

/** Global Hardware access */
DaisySeed hw;
float     semitones_per_octave = 12.0f;
float     volts_per_semitone   = 1.0 / semitones_per_octave;
int       poo                  = 0;
uint32_t  volts_to_DAC(int      volts,
                       uint32_t resolution  = 4096,
                       float    min_voltage = 0.0f,
                       float    max_voltage = 3.3f)
{
    float    voltage_range    = max_voltage - min_voltage;
    float    normalized_value = (volts - min_voltage) / voltage_range;
    uint32_t dacUnit          = ceil(normalized_value * (resolution - 1));
    return dacUnit;
}


float midi_note_to_volts(uint32_t midi_note)
{
    return (midi_note)*volts_per_semitone;
}

int main(void)
{
    /** Initialize our hardware */
    hw.Init();

    /** Configure and Initialize the DAC */
    DacHandle::Config dac_cfg;
    dac_cfg.bitdepth   = DacHandle::BitDepth::BITS_12;
    dac_cfg.buff_state = DacHandle::BufferState::DISABLED;
    dac_cfg.mode       = DacHandle::Mode::POLLING;
    dac_cfg.chn        = DacHandle::Channel::BOTH;
    hw.dac.Init(dac_cfg);

    /** Variable for output */
    int output_val_1 = 0;
    int output_val_2 = 0;

    /** Infinite Loop */
    while(1)
    {
        /** Every 1millisecond we'll increment our 12-bit value
         *  The value will go from 0-4095
         *  The full ramp waveform will have a period of about 4 seconds
         *  The secondary waveform will be an inverted ramp (saw) 
         *  at the same frequency
         */
        System::Delay(1500);


        /** This sets our second output to the opposite of the first */
        ///  output_val_2 = 4095 - output_val_1;

        int dacU1 = volts_to_DAC(poo, 4096, 0.0f, 3.3f);
        /** And write our 12-bit value to the output */
        hw.dac.WriteValue(DacHandle::Channel::ONE, dacU1);
        hw.dac.WriteValue(DacHandle::Channel::TWO, dacU1);
        poo++;

        if(poo > 7)
        {
            poo = 0;
        }
    }
}