#pragma once

#include "main.h"
#include "Menu.h"

enum MenuID : uint16_t
{
    M_ROOT, // 0x00 (root menu must be 0x00)
    M_BACK,
    M_INPUTS,
    M_INPUT_IN,
    M_INPUT_SS,
    M_INPUT_RESET,
    M_INPUT_MIDI,
    M_OUTPUTS,
    M_OUTPUT_1,
    M_OUTPUT_2,
    M_OUTPUT_3,
    M_OUTPUT_MIDI,
};

extern MenuItem menu_root;