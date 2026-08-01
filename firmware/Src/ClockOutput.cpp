#include "ClockOutput.h"

void ClockOutput::setDivisor(uint16_t divisor)
{
    this->divisor = divisor;
}

void ClockOutput::setAmplitude(uint16_t amplitude)
{
    this->amplitude = amplitude > DAC_MAX ? DAC_MAX : amplitude;
}

void ClockOutput::setPhaseOffset(uint16_t offset)
{
    this->phaseOffset = offset;
}

bool ClockOutput::isTriggered(uint32_t pulseCount) const
{
    if (divisor == 0)
    {
        return false;
    }

    // phaseOffset rotates the trigger within each divisor period. For off-beat,
    // divisor = PPQN and phaseOffset = PPQN/2, so the window sits on the "&".
    const uint32_t phase = (pulseCount + phaseOffset) % divisor;

    if (type == ANALOG)
    {
        // Analog outs use a 50% duty-cycle gate instead of a 1-pulse spike.
        // That makes ofBeat the inversion of x1 (high on the off-beat half),
        // which is obvious on a scope or CV destination — a single-pulse spike
        // at the same rate as x1 is easy to mistake for x1 when self-triggered.
        uint32_t gateLen = divisor / 2u;
        if (gateLen == 0u)
        {
            gateLen = 1u;
        }
        return phase < gateLen;
    }

    // Digital outs stay as short triggers: high only on the single phase-0 pulse.
    return phase == 0u;
}

void ClockOutput::update(uint32_t pulseCount)
{
    const bool triggered = isTriggered(pulseCount);

    if (type == ANALOG)
    {
        if (dac != nullptr)
        {
            dac->write(triggered ? amplitude : 0);
        }
    }
    else
    {
        gpio.write(triggered ? HIGH : LOW);
    }
}
