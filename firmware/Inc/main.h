#pragma once

#include "cmsis_os.h"
#include "common.h"

#define PPQN 24

#define I2C1_SDA PB_9
#define I2C1_SCL PB_8
#define DISPLAY_SHUTDOWN PC_13

#define TRIG_OUT_1 PC_15
#define TRIG_OUT_2 PC_14

#define DAC_OUT_3 PA_5

#define LED_START_STOP PB_4
#define LED_RESET PB_0

#define BTN_START_STOP PB_7
#define BTN_RESET PB_1

#define EXT_CLOCK_IN PA_2
#define EXT_START_STOP_IN PA_1
#define EXT_RESET_IN PA_3

#define ROTARY_ENCODER_A PA_6
#define ROTARY_ENCODER_B PA_7
#define ROTARY_ENCODER_BUTTON PA_8

#define TRANSPORT_PPQN_1 PC_12
#define TRANSPORT_PPQN_24 PC_11
#define TRANSPORT_RESET PA_15       // why?
#define TRANSPORT_START_STOP PC_10

#define CAN_RX PB_5
#define CAN_TX PB_6