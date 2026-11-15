#include "globals.h"

void dispatch_event_isr(Event event)
{
    uint32_t event_id = static_cast<uint32_t>(event);

    // USB MIDI clock calls metronome.tick() from the usbd task, and that path posts
    // pulse and step events here. The serial and timer paths post from an ISR.
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(queue_main, &event_id, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        xQueueSend(queue_main, &event_id, 0);
    }
}