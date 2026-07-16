#pragma once

#include "cmsis_os.h"
#include "common.h"

#define PPQN 24

#define I2C1_SDA PB_9
#define I2C1_SCL PB_8
#define DISPLAY_SHUTDOWN PC_13

#define TRIG_OUT_1 PC_15
#define TRIG_OUT_2 PC_14

#define LED_START_STOP PB_0
#define LED_RESET PB_4

#define BTN_START_STOP PB_1
#define BTN_RESET PB_7

#define EXT_CLOCK_IN PA_2
#define EXT_START_STOP_IN PA_1
#define EXT_RESET_IN PA_3