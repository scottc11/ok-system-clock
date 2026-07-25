#pragma once

#include "main.h"
#include "InterruptIn.h"
#include "DigitalIn.h"

class SystemClock
{
public:
    SystemClock(PinName pulse_pin) 
    {

    }
    
    float bpm;
    uint8_t pulse;
    uint8_t step;

    // InterruptIn ppqn1;
    // InterruptIn ppqn96;
    // DigitalIn reset;
    // DigitalIn startStop;

    TIM_HandleTypeDef *overflowTimer; // ex. tim4
    TIM_HandleTypeDef *captureTimer; // ex. tim2

    void init();
};

/*
Next is to setup the system clock class to be either a master or a slave,
and you will need to setup pins for all the clock bus pins (1ppqn, 96ppqn, reset, start/stop)
if a slave, you respond to these pins
if a master, you set these pins

*/