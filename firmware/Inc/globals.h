#pragma once

#include "main.h"
#include "task_I2C_manager.h"
#include "Display.h"
#include "Menu.h"
#include "Metronome.h"
#include "ClockOutput.h"

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
extern Menu menu;
extern Metronome metronome;
extern ClockOutput output1;
extern ClockOutput output2;

void dispatch_event_isr(Event event);