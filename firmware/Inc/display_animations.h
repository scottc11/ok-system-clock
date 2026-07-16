#pragma once

#include "main.h"

struct Animation
{
    const uint8_t *frames;     // array of frame buffers, each frame is 22x5 bytes
    uint8_t frameCount;        // number of frames in the animation
    uint16_t frameDuration_ms; // duration of each frame in milliseconds
    bool loop;                 // whether the animation should loop
};

struct Animator
{
    const Animation *animation = nullptr;
    uint8_t frame = 0;
    uint16_t elapsed = 0; // time elapsed since last frame
    bool active = false;
};

extern const Animation ANIM_WIPE_LEFT_RIGHT;
extern const Animation ANIM_BLINK_3;
