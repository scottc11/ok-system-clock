#include "menu_items.h"

// ****************************************************************************************************
// **************   MENU ITEM DEFINITIONS   ***********************************************************
// ****************************************************************************************************

// These global arrays are statically initialized at compile/link time, so each MenuItem instance and
// pointer relationship is laid out before main() runs (no runtime allocation). compileTimeLength(...) returns
// a size_t at compile time; static_cast<uint16_t>(...) safely narrows that unsigned size to uint16_t for
// fields like maxValue where we store "last valid index" as count - 1.

// order of struct: ID, text, options, optionCount, value, hasSubmenus, isBack, minValue, maxValue, step, valueLabels, valueLabelCount

/**
 * @brief compile-time array length helper to avoid hardcoding array lengths or using sizeof().
 * @note Parameter type T (&)[N] means “reference to an array of N Ts”
 * @tparam T element type (e.g. MenuItem, const char*)
 * @tparam N number of elements in the array (captured from the parameter type)
 * @return uint8_t number of elements in the array
 */
template <typename T, size_t N>
static uint8_t compileTimeLength(T (&)[N])
{
    return static_cast<uint8_t>(N);
}

static MenuItem makeBackItem()
{
    return {M_BACK, "< Back", nullptr, 0, 0, false, true, 0, 100, 1, nullptr, 0};
}

const char *const divisorLabels[] = {"1/4", "1/8", "1/16"};

// Maps a divisorLabels index onto the matching ClockOutput division.
static uint8_t divisorForIndex(uint16_t index)
{
    switch (index)
    {
        case 0:  return ClockOutput::QUARTER;   // "1/4"
        case 1:  return ClockOutput::EIGHTH;    // "1/8"
        case 2:  return ClockOutput::SIXTEENTH; // "1/16"
        default: return ClockOutput::QUARTER;
    }
}

const char *const metronomeSourceLabels[] = {"EXT.", "INT.", "MIDI", "LINK"};

MenuItem inputsOptions[] = {
    makeBackItem(),
    {M_INPUT_IN, "IN", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_SS, "SS", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_RESET, "RESET", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_MIDI, "MIDI", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
};

MenuItem outputsOptions[] = {
    {M_OUTPUT_1, "OUT 1", nullptr, 0, 0, false, false, 0, compileTimeLength(divisorLabels) - 1, 1, divisorLabels, compileTimeLength(divisorLabels)},
    {M_OUTPUT_2, "OUT 2", nullptr, 0, 0, false, false, 0, compileTimeLength(divisorLabels) - 1, 1, divisorLabels, compileTimeLength(divisorLabels)},
    {M_OUTPUT_3, "OUT 3", nullptr, 0, 0, false, false, 0, compileTimeLength(divisorLabels) - 1, 1, divisorLabels, compileTimeLength(divisorLabels)},
    makeBackItem(),
};

MenuItem mainOptions[] = {
    {M_METRONOME_SOURCE, "SOURCE", nullptr, 0, 0, false, false, 0, static_cast<uint16_t>(compileTimeLength(metronomeSourceLabels) - 1), 1, metronomeSourceLabels, compileTimeLength(metronomeSourceLabels)},
    {M_INPUTS, "INS:", inputsOptions, compileTimeLength(inputsOptions), 0, true, false, 0, 100, 1, nullptr, 0},
    {M_OUTPUTS, "OUTS:", outputsOptions, compileTimeLength(outputsOptions), 0, true, false, 0, 100, 1, nullptr, 0},
};

MenuItem menu_root = {M_ROOT, "Main", mainOptions, compileTimeLength(mainOptions), 0, true, false, 0, 100, 1, nullptr, 0};

void menuHandler(uint8_t direction)
{
    const int8_t delta = direction ? 1 : -1;
    MenuItem *activeMenuItem = menu.getActiveMenuItem();
    if (activeMenuItem == nullptr)
    {
        return;
    }

    const bool valueChanged = menu.handleRotation(delta);
    if (valueChanged)
    {
        MenuItem *changedItem = menu.getActiveMenuItem();
        if (changedItem != nullptr)
        {
            applyMenuSideEffects(*changedItem);
        }
    }

    display.drawString(menu.getActiveItemText());
    return;
}

/**
 * @brief Every time the encoder is rotated, this function is called to apply the side effects of the menu item.
 * 
 * @param item The menu item that was changed.
 */
void applyMenuSideEffects(MenuItem &item)
{
    switch (item.id)
    {
        case M_INPUT_IN:
            break;
        case M_METRONOME_SOURCE:
            metronome.setMode(static_cast<Metronome::Mode>(item.value));
            break;
        case M_OUTPUT_1:
            output1.setDivisor(divisorForIndex(item.value));
            break;
        case M_OUTPUT_2:
            output2.setDivisor(divisorForIndex(item.value));
            break;
        case M_OUTPUT_3:
            break;
    }
}

/**
 * @brief Sync all menu item values with runtime values
 *
 * @param menuNode root menu node
 */
void syncMenuValuesRecursive(MenuItem &menuNode)
{
    if (menuNode.optionCount == 0 || menuNode.options == nullptr)
    {
        return;
    }

    for (uint8_t i = 0; i < menuNode.optionCount; ++i)
    {
        MenuItem &item = menuNode.options[i];

        if (item.hasSubmenus)
        {
            syncMenuValuesRecursive(item);
            continue;
        }

        switch (item.id)
        {
        case M_METRONOME_SOURCE:
            item.value = static_cast<uint16_t>(metronome.mode);
            break;
        default:
            break;
        }
    }
}