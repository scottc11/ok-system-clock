#include "ClockOutput.h"

void ClockOutput::setDivisor(uint8_t divisor)
{
    this->divisor = divisor;
}

bool ClockOutput::isTriggered(uint8_t pulse) const
{
    return divisor != 0 && (pulse % divisor) == 0;
}

void ClockOutput::update(uint8_t pulse)
{
    gpio.write(isTriggered(pulse) ? HIGH : LOW);
}
