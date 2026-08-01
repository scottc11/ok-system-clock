#pragma once

#include "main.h"
#include "InterruptIn.h"
#include "DigitalIn.h"

class SystemClock
{
public:
    enum class Mode : uint8_t
    {
        EXTERNAL,  // follows an external clock signal (square wave) via input capture
        INTERNAL,  // internal clock (overflowTimer overflow)
        MIDI,      // MIDI clock
        LINK,      // ableton link
        TRANSPORT, // transport pins drive the clock
        MANUAL     // app manually calls tick()
    };

    SystemClock(PinName pulse_pin = NC, PinName reset_pin = NC, PinName start_stop_pin = NC) : tp_ppqn96(pulse_pin), tp_reset(reset_pin), tp_startStop(start_stop_pin)
    {
        mode = Mode::INTERNAL;
        master = false;
        bpm = 120;
        pulse = 0;
        step = 0;
        stepsPerBar = 4;
        ticksPerPulse = 11129 / PPQN;
    }

    Mode mode;
    float bpm;
    bool master;  // if true, instance drives transport pins, if false, instance reacts to transport pins
    bool running;
    
    uint8_t pulse;
    uint8_t step;           // current step. Will never exceed value of stepsPerBar
    uint8_t stepsPerBar;    // value represents the number of quarter notes per bar (ie. 3/4, 4/4, 5/4, 6/4, 7/4)
    
    uint16_t ticksPerPulse; // how many timer ticks per pulse (PPQN)

    InterruptIn tp_ppqn96;
    InterruptIn tp_reset;
    DigitalIn tp_startStop;

    TIM_HandleTypeDef *overflowTimer; // ex. tim4
    TIM_HandleTypeDef *captureTimer; // ex. tim2
    int captureChannel;              // ex. TIM_CHANNEL_3

    Callback<void()> tickCallback;              // this callback gets executed at a frequency equal to tim1_freq
    Callback<void()> barResetCallback;          // executes when clock.step exceeds the set time signature (ie. one bar)
    Callback<void(uint8_t pulse)> ppqnCallback; // executes every tick
    Callback<void(uint16_t step)> stepCallback; // executes every step
    Callback<void(uint8_t pulse)> resetCallback;

    void init();
    void setMode(Mode mode);
    void tick();
    void stepHandler();
    void reset();
};

/*
Next is to setup the system clock class to be either a master or a slave,
and you will need to setup pins for all the clock bus pins (1ppqn, 96ppqn, reset, start/stop)
if a slave, you respond to these pins
if a master, you set these pins

*/