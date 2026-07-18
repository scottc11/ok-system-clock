#pragma once

#include "main.h"
#include "DigitalOut.h"

class ClockOutput
{
public:
    ClockOutput(PinName pin) : gpio(pin) {}
    
    DigitalOut gpio;
    uint8_t divisor;

    void setDivisor(uint8_t divisor);
};