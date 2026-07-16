#pragma once

#include "main.h"
#include "IS31FL3730.h"
#include "DigitalOut.h"
#include "task_I2C_manager.h"
#include "display_animations.h"
#include "display_glyphs.h"

class Display
{
    public:
        // Physical layout: 6 clusters of 5x3 LEDs placed side by side. The gaps
        // between clusters contain no LEDs, so they consume no driver columns.
        // Each character occupies exactly one 3-column cluster and characters are
        // drawn back to back with no blank column between them.
        static constexpr uint8_t COLUMNS_PER_CHAR = 3;
        static constexpr uint8_t CHAR_COUNT = 6;
        static constexpr uint8_t DISPLAY_COLUMNS = COLUMNS_PER_CHAR * CHAR_COUNT; // 18

        Display(I2C *_i2c, PinName _shutdown) : driver(_i2c, IS31FL3730_ADDR_GND), shutdown_pin(_shutdown)
        {
            i2c = _i2c;
        }
    
        I2C *i2c;
        IS31FL3730 driver;
        DigitalOut shutdown_pin;

        Animator animator;

        // Shadow framebuffers for each matrix (Matrix 1 & Matrix 2), 11 columns each in 5x11 mode.
        // Each entry holds the row bits for that column (R1..R5 in the 5 LSBs).
        uint8_t matrix_buffer[2][11] = {{0}};

        // TX buffers used for bulk I2C writes:
        // [0] = start register address (0x01 for Matrix 1, 0x0E for Matrix 2)
        // [1..11] = column data copied from matrix_buffer.
        uint8_t tx_buffer_matrix1[12] = {0};
        uint8_t tx_buffer_matrix2[12] = {0};
        
        void init();
        void update();
        void setPWM(uint8_t pwm);
        void drawChar(char c, uint8_t column);
        void drawString(const char *str);
        void drawInteger(int value);
        void drawIntegerSigned(int value);
        void drawFloat(float value, uint8_t decimals = 1);

        void clear();

        void startAnimation(const Animation *animation);
        void stopAnimation();
        void updateAnimation(uint16_t dt_ms);

        // Return pointer to 3-column glyph data for a given ASCII character (32–126).
        const uint8_t* glyphForChar(char c) const;

    private:
        // Draw a null-terminated string right-aligned to the last cluster.
        // Assumes the caller has already cleared the framebuffer.
        void drawStringRightAligned(const char *buf);
};