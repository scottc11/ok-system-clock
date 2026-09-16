#pragma once

#include "main.h"
#include "task_I2C_manager.h"
#include "task_CAN_manager.h"
#include "Display.h"
#include "Menu.h"
#include "Metronome.h"
#include "ClockOutput.h"
#include "MIDI.h"
#include "M24256.h"

enum Event
{
    ENCODER_ROTATE,
    ENCODER_PRESS,
    ENCODER_RELEASE,
    UPDATE_DISPLAY,
    PPQN_UPDATE,
    METRONOME_PULSE,
    METRONOME_STEP,
    MIDI_RECEIVED,
};

extern QueueHandle_t queue_main;
extern Display display;
extern M24256 eeprom;

extern bool encoderPressed;
extern Menu menu;
extern Metronome metronome;
extern MIDI midi;
extern ClockOutput output1;
extern ClockOutput output2;
extern ClockOutput output3;


void dispatch_event_isr(Event event);