#pragma once

#include "main.h"
#include "DigitalOut.h"
#include "Metronome.h"

class ClockOutput
{
public:
    // Clock divisions expressed as the number of PPQN pulses between triggers.
    // A smaller value fires more often (a faster note value).
    enum Division : uint8_t {
        QUARTER   = PPQN,      // 1/4  note -> one trigger per beat
        EIGHTH    = PPQN_8th,  // 1/8  note -> two triggers per beat
        SIXTEENTH = PPQN_16th, // 1/16 note -> four triggers per beat
    };

    ClockOutput(PinName pin, uint8_t divisor = QUARTER) : gpio(pin), divisor(divisor) {}

    DigitalOut gpio;
    uint8_t divisor; // number of PPQN pulses between triggers

    void setDivisor(uint8_t divisor);

    // True on the single pulse where this output fires for its divisor.
    bool isTriggered(uint8_t pulse) const;

    // Drives the output from the current metronome pulse (0..PPQN-1).
    // Held HIGH for the single pulse on which the trigger fires, LOW otherwise.
    void update(uint8_t pulse);
};
