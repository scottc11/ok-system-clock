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

const char *const clockSourceLabels[] = {"CLOCK", "FREE"};
const char *const rangeLabels[] = {"SLOW", "MEDIUM", "FAST"};
const char *const divisorLabels[] = {"1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32"};
const char *const metronomeSourceLabels[] = {"MIDI", "INT.", "EXT."};
const char *const trackLabels[] = {"VCO1", "VCO2", "VCO1&2"};
const char *const trackScaleLabels[] = {"IONIAN", "DORIAN", "PHRYG", "LYDIAN", "MIXOLYD", "AEOLIAN", "LOCRIAN"};
const char *const trackRangeLabels[] = {"1 OCT", "2 OCT", "3 OCT", "4 OCT"};


MenuItem inputsOptions[] = {
    makeBackItem(),
    {M_INPUT_IN, "IN", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_SS, "SS", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_RESET, "RESET", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_MIDI, "MIDI", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
};

MenuItem outputsOptions[] = {
    makeBackItem(),
    {M_OUTPUT_1, "OUTPUT 1", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_OUTPUT_2, "OUTPUT 2", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_OUTPUT_3, "OUTPUT 3", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_OUTPUT_MIDI, "MIDI", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
};

MenuItem mainOptions[] = {
    {M_INPUTS, "INPUTS", inputsOptions, compileTimeLength(inputsOptions), 0, true, false, 0, 100, 1, nullptr, 0},
    {M_OUTPUTS, "OUTPUTS", outputsOptions, compileTimeLength(outputsOptions), 0, true, false, 0, 100, 1, nullptr, 0},
};

MenuItem menu_root = {M_ROOT, "Main", mainOptions, compileTimeLength(mainOptions), 0, true, false, 0, 100, 1, nullptr, 0};