#include "ClockOutput.h"

void ClockOutput::setDivisor(uint16_t divisor)
{
    this->divisor = divisor;
}

/**
 * @brief Sets the DAC level emitted on a trigger (clamped to DAC_MAX). ANALOG only.
 * @param amplitude the DAC level to emit on a trigger.
 */
void ClockOutput::setAmplitude(uint16_t amplitude)
{
    this->amplitude = amplitude > DAC_MAX ? DAC_MAX : amplitude;
}

/**
 * @brief Sets the trigger phase offset in pulses (0 = on the downbeat).
 * @param offset the phase offset in pulses.
 */
void ClockOutput::setPhaseOffset(uint16_t offset)
{
    this->phaseOffset = offset;
}

/**
 * @brief True while this output should be HIGH
 * Handles all states of the output, including the reset state and DIN SYNC.
 * @param pulseCount the free-running PPQN pulse index since the last transport reset.
 */
bool ClockOutput::isTriggered(uint32_t pulseCount) const
{
    if (useForReset) {
        return resetState;
    }

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

/**
 * @brief Drives the output from the current free-running pulse count.
 * DIGITAL: held HIGH for the single pulse on which the trigger fires.
 * ANALOG:  driven to `amplitude` for half the divisor period (50% duty),
 *          starting at the phase-offset trigger point — so ofBeat is the
 *          inversion of x1.
 * 
 * @param pulseCount 
 */
void ClockOutput::update(uint32_t pulseCount)
{
    const bool triggered = isTriggered(pulseCount);

    set(triggered);
}

/**
 * @brief Sets the output to the given state.
 */
void ClockOutput::set(bool state) {
    if (type == ANALOG) {
        if (dac != nullptr) {
            dac->write(state ? amplitude : 0);
        }
    } else {
        gpio.write(state ? HIGH : LOW);
    }
}

/**
 * @brief Handles the reset state for this output.
 * @param state the new reset state.
 */
void ClockOutput::handleReset(bool state) {
    if (useForReset) {
        resetState = state;
        set(state);
    }
}

/**
 * @brief Configures this output to be used for the reset signal.
 * 
 * @param state 
 */
void ClockOutput::configureAsReset(bool state) {
    useForReset = state;
}