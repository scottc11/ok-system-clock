#include "SystemClock.h"

void SystemClock::init()
{
    bpm = 120;
    pulse = 0;
    step = 0;
    tp_ppqn96.rise(callback(this, &SystemClock::tick));
    tp_reset.rise(callback(this, &SystemClock::reset));
}

void SystemClock::setMode(Mode mode)
{
    this->mode = mode;
    tp_ppqn96.disable();
    tp_reset.disable();

    switch (mode) {
        case Mode::EXTERNAL:
            break;
        case Mode::INTERNAL:
            break;
        case Mode::MIDI:
            break;
        case Mode::LINK:
            break;
        case Mode::TRANSPORT:
            HAL_TIM_IC_Stop_IT(captureTimer, captureChannel);
            HAL_TIM_Base_Stop_IT(overflowTimer);
            tp_ppqn96.enable();
            tp_reset.enable();
            break;
    }
}

/**
 * @brief Handle the clock tick (pulse increment and step handling)
 *
 * NOTE: Function usually called in interrupt context.
 */
void SystemClock::tick()
{

    if (running == false)
        return;

    if (ppqnCallback)
        ppqnCallback(pulse); // when clock inits, this ensures the 0ith pulse will get handled

    // by checking this first, you have the chance to reset any sequences prior to executing their 0ith pulse
    if (pulse == 0)
    {
        if (stepCallback)
            stepCallback(step);
    }

    if (pulse < PPQN - 1)
    {
        pulse++;
    }
    else
    {
        pulse = 0;
        stepHandler();

        if (mode == Mode::EXTERNAL)
        {
            __HAL_TIM_DISABLE(overflowTimer); // halt overflow timer until a new input capture event occurs (ei. stop the clock)
        }
    }
}

void SystemClock::stepHandler()
{
    if (step < stepsPerBar - 1)
    {
        step++;
    }
    else
    {
        step = 0;
        if (barResetCallback)
            barResetCallback();
    }
}

void SystemClock::reset()
{
    this->pulse = 0;
    this->step = 0;
    if (resetCallback)
        resetCallback(pulse);
}