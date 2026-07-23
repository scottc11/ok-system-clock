#pragma once

#include "main.h"

class SystemClock
{
public:
    SystemClock(PinName pulse_pin) 
    {

    }
    
    float bpm;
    uint8_t pulse;
    uint8_t step;
    TIM_HandleTypeDef *overflowTimer; // ex. tim4
    TIM_HandleTypeDef *captureTimer; // ex. tim2

    void init();
};