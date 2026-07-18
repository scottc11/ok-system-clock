#pragma once

#include "main.h"
#include "task_I2C_manager.h"
#include "Display.h"
#include "menu_items.h"

enum Event {
    ENCODER_ROTATE,
    ENCODER_PRESS,
    ENCODER_RELEASE,
    UPDATE_DISPLAY,
    PPQN_UPDATE,
    METRONOME_PULSE,
};

extern QueueHandle_t queue_main;
extern Display display;
extern bool encoderPressed;

void dispatch_event_isr(Event event);