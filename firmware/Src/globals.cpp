#include "globals.h"

void dispatch_event_isr(Event event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t event_id = static_cast<uint32_t>(event);
    xQueueSendFromISR(queue_main, &event_id, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}