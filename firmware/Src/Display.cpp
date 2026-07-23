#include "Display.h"
#include <cstdio>

void Display::init()
{
    // enable display
    shutdown_pin.write(1);

    // Initialise local framebuffers and TX buffers
    for (uint8_t m = 0; m < 2; ++m)
    {
        for (uint8_t c = 0; c < 11; ++c)
        {
            matrix_buffer[m][c] = 0;
        }
    }

    // Set starting register addresses for bulk writes
    tx_buffer_matrix1[0] = 0x01; // Matrix 1 Data Registers start at 0x01
    tx_buffer_matrix2[0] = 0x0E; // Matrix 2 Data Registers start at 0x0E

    driver.reset();
    driver.setConfigRegister(false, IS31FL3730::DisplayMode::MatrixOneAndTwo, false, IS31FL3730::MatrixMode::_5x11);
    driver.setLightingEffectRegister(0x8, 0x00);
    driver.setPWMRegister(10);
}

void Display::setPWM(uint8_t pwm)
{
    driver.setPWMRegister(pwm);
}

void Display::clear()
{
    for (uint8_t matrix = 0; matrix < 2; ++matrix)
    {
        for (uint8_t column = 0; column < 11; ++column)
        {
            matrix_buffer[matrix][column] = 0;
        }
    }
}

void Display::startAnimation(const Animation *animation)
{
    animator.animation = animation;
    animator.frame = 0;
    animator.elapsed = 0;
    animator.active = true;
}

void Display::stopAnimation()
{
    animator.active = false;
    this->clear();
}

/**
 * @brief Update the animation
 * 
 * @param dt_ms time since last update in milliseconds
 */
void Display::updateAnimation(uint16_t dt_ms)
{
    if (!animator.active || !animator.animation)
        return;

    animator.elapsed += dt_ms;
    if (animator.elapsed < animator.animation->frameDuration_ms) // if the elapsed time is less than the frame duration, do not advance the animation
        return;

    animator.elapsed = 0;
    const uint8_t *frames = animator.animation->frames;

    // Copy 22-column frame into the interleaved matrix buffers.
    const uint8_t *frame_ptr = &frames[animator.frame * 22];
    for (uint8_t x = 0; x < 22; ++x)
    {
        uint8_t matrix = static_cast<uint8_t>(x & 1u);
        uint8_t col = static_cast<uint8_t>(x >> 1);
        matrix_buffer[matrix][col] = frame_ptr[x];
    }

    animator.frame++;
    if (animator.frame >= animator.animation->frameCount) {
        if (animator.animation->loop) {
            animator.frame = 0;
        } else {
            this->stopAnimation();
        }
    }
}

/**
 * @brief Draw a string on the display, left-aligned
 *
 * Characters are drawn back to back, one 3-column cluster each, with no blank
 * column between them. Up to CHAR_COUNT characters are shown.
 *
 * @param str string to draw
 */
void Display::drawString(const char *str)
{
    if (str == nullptr) return;
    clear();
    uint8_t column = 0;
    for (uint8_t i = 0; str[i] != '\0'; i++)
    {
        if (column >= DISPLAY_COLUMNS) {
            break;
        }
        drawChar(str[i], column);
        column = static_cast<uint8_t>(column + COLUMNS_PER_CHAR);
    }
}

/**
 * @brief Draw a null-terminated buffer right-aligned to the last cluster
 *
 * Each character occupies one 3-column cluster with no gap between characters.
 * If the text is wider than the display it starts at column 0 so the leftmost
 * clusters are used first. The caller must clear the framebuffer beforehand.
 *
 * @param buf null-terminated text to draw
 */
void Display::drawStringRightAligned(const char *buf)
{
    if (buf == nullptr) return;

    // Compute string length.
    uint8_t len = 0;
    while (buf[len] != '\0') {
        ++len;
    }
    if (len == 0) {
        return;
    }

    // Each glyph is COLUMNS_PER_CHAR columns wide with no gap between characters.
    uint8_t width = static_cast<uint8_t>(COLUMNS_PER_CHAR * len);

    // If too wide, start at 0 so the leftmost clusters are still used.
    uint8_t startColumn = 0;
    if (width < DISPLAY_COLUMNS) {
        startColumn = static_cast<uint8_t>(DISPLAY_COLUMNS - width);
    }

    uint8_t column = startColumn;
    for (uint8_t i = 0; i < len; ++i)
    {
        if (column >= DISPLAY_COLUMNS) {
            break;
        }
        drawChar(buf[i], column);
        column = static_cast<uint8_t>(column + COLUMNS_PER_CHAR);
    }
}

/**
 * @brief Draw an integer on the display without an explicit sign
 *
 * @param value integer to draw
 */
void Display::drawInteger(int value)
{
    clear(); // clear buffer

    char buf[12] = {0};
    std::snprintf(buf, sizeof(buf), "%d", value);

    drawStringRightAligned(buf);
}

/**
 * @brief Draw a signed integer on the display (always shows a sign)
 *
 * @param value integer to draw (signed)
 */
void Display::drawIntegerSigned(int value)
{
    clear(); // clear buffer

    char buf[12] = {0};
    std::snprintf(buf, sizeof(buf), "%+d", value);

    drawStringRightAligned(buf);
}

/**
 * @brief Draw a float on the display without an explicit sign
 *
 * @param value float to draw
 * @param decimals number of decimal places (default is 1)
 */
void Display::drawFloat(float value, uint8_t decimals)
{
    clear(); // clear buffer

    char buf[16] = {0};

    // Build format string like "%.1f"
    char fmt[8] = {0};
    std::snprintf(fmt, sizeof(fmt), "%%.%uf", static_cast<unsigned>(decimals));
    std::snprintf(buf, sizeof(buf), fmt, static_cast<double>(value));

    drawStringRightAligned(buf);
}

/**
 * @brief Draw a character on the display at a given column
 * 
 * @param c ASCII character to draw
 * @param column column to draw the character at (0-based), DISPLAY_COLUMNS columns are available
 */
void Display::drawChar(char c, uint8_t column)
{
    const uint8_t* glyph = glyphForChar(c);

    // Each glyph has COLUMNS_PER_CHAR columns; we place them at
    // logical_x = column, column+1, column+2.
    for (uint8_t i = 0; i < COLUMNS_PER_CHAR; ++i)
    {
        uint8_t x = static_cast<uint8_t>(column + i);
        if (x >= DISPLAY_COLUMNS) {
            break; // past the physical display width
        }

        uint8_t matrix = (x % 2u == 0u) ? 0u : 1u;
        uint8_t col_reg = static_cast<uint8_t>(x / 2u);
        if (col_reg >= 11u) {
            continue;
        }

        // Write directly into the shadow framebuffer; actual I2C transfer happens in update().
        matrix_buffer[matrix][col_reg] = glyph[i];
    }
}

void Display::update()
{
    // Prepare TX buffers from the shadow framebuffers
    for (uint8_t c = 0; c < 11; ++c)
    {
        tx_buffer_matrix1[c + 1] = matrix_buffer[0][c];
        tx_buffer_matrix2[c + 1] = matrix_buffer[1][c];
    }

    // 1) send Matrix 1 data (0x01..0x0B)
    I2CRequest m1_req{ RequestType::Transmit, driver.i2c, driver.address, tx_buffer_matrix1, static_cast<uint16_t>(sizeof(tx_buffer_matrix1)), nullptr, pdFAIL};
    i2c_submit_async(m1_req);

    // 2) send Matrix 2 data (0x0E..0x18)
    I2CRequest m2_req{ RequestType::Transmit, driver.i2c, driver.address, tx_buffer_matrix2, static_cast<uint16_t>(sizeof(tx_buffer_matrix2)), nullptr, pdFAIL };
    i2c_submit_async(m2_req);

    // 3) send update command to latch both matrices
    static uint8_t update_buffer[2] = {0x0C, 0x00};
    I2CRequest update_req{ RequestType::Transmit, driver.i2c, driver.address, update_buffer, 2, nullptr, pdFAIL };
    i2c_submit_async(update_req);
}

const uint8_t* Display::glyphForChar(char c) const
{
    // Clamp to printable ASCII range 32 (' ')..126 ('~').
    if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
        c = '?';
    }
    return GLYPHS[static_cast<unsigned char>(c) - 32];
}