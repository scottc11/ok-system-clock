#pragma once

#include "main.h"
#include "DigitalOut.h"
#include "AnalogOut.h"
#include "Metronome.h"

class ClockOutput
{
public:
    // Selects how a triggered pulse is emitted.
    enum Type : uint8_t {
        DIGITAL, // drives a DigitalOut HIGH on a trigger, LOW otherwise
        ANALOG,  // drives an AnalogOut (DAC) to `amplitude` on a trigger, 0 otherwise
    };

    // 12-bit DAC full scale; the default amplitude for an analog trigger pulse.
    static constexpr uint16_t DAC_MAX = 4095;

    // The trigger rate is expressed as `divisor`: the number of PPQN pulses
    // between triggers, counted against a free-running pulse counter (see
    // main.cpp) that runs continuously from the last transport reset. A smaller
    // divisor fires more often. The menu (menu_items.cpp) presents divisors as
    // clock multipliers/divisions (e.g. "x2" = twice per beat = PPQN/2, "/2" =
    // once every two beats = PPQN*2). Because divisions can span many beats
    // (up to /16 = 16 * PPQN = 1536 pulses), the divisor is a uint16_t.
    static constexpr uint16_t DEFAULT_DIVISOR = PPQN; // "x1": one trigger per beat

    // Digital (GPIO) trigger output.
    ClockOutput(PinName pin, uint16_t divisor = DEFAULT_DIVISOR)
        : type(DIGITAL), gpio(pin), dac(nullptr), divisor(divisor), amplitude(DAC_MAX / 2), phaseOffset(0) {}

    // Analog (DAC) trigger output. On a trigger the DAC is driven to `amplitude`
    // (12-bit), otherwise 0. The AnalogOut is owned by the caller (must be init()ed).
    ClockOutput(AnalogOut *dacOut, uint16_t divisor = DEFAULT_DIVISOR)
        : type(ANALOG), gpio(NC), dac(dacOut), divisor(divisor), amplitude(DAC_MAX / 2), phaseOffset(0) {}

    Type type;         // which peripheral this output drives
    DigitalOut gpio;   // used when type == DIGITAL
    AnalogOut *dac;    // used when type == ANALOG (not owned)
    uint16_t divisor;  // number of PPQN pulses between triggers
    uint16_t amplitude; // 12-bit DAC level emitted on a trigger (ANALOG only)
    uint16_t phaseOffset; // pulses added before the divisor test; shifts triggers off the downbeat (e.g. divisor/2 = off-beat)

    void setDivisor(uint16_t divisor);

    // Sets the DAC level emitted on a trigger (clamped to DAC_MAX). ANALOG only.
    void setAmplitude(uint16_t amplitude);

    // Sets the trigger phase offset in pulses (0 = on the downbeat).
    void setPhaseOffset(uint16_t offset);

    // True while this output should be asserted for its divisor/phase.
    // pulseCount is the free-running PPQN pulse index since the last transport
    // reset. DIGITAL: true on the single trigger pulse. ANALOG: true for the
    // first half of each divisor period after applying phaseOffset (50% gate).
    bool isTriggered(uint32_t pulseCount) const;

    // Drives the output from the current free-running pulse count.
    // DIGITAL: held HIGH for the single pulse on which the trigger fires.
    // ANALOG:  driven to `amplitude` for half the divisor period (50% duty),
    //          starting at the phase-offset trigger point — so ofBeat is the
    //          inversion of x1.
    void update(uint32_t pulseCount);
};
