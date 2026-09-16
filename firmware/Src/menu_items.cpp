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

// Clock-output rate table, expressed as multipliers ("xN": N triggers per beat)
// and divisions ("/N": one trigger every N beats). The ".3"/".6" suffixes denote
// triplet subdivisions (1/3 and 2/3 of a beat). "off-bt" is the same rate as "x1"
// but phase-shifted by half a beat (the "&"). Ordered slowest -> fastest so the
// encoder sweeps monotonically. Only rates whose divisor is a whole number of
// PPQN pulses are listed; rateLabels, rateDivisors, and ratePhaseOffsets are kept
// in lockstep (index i in one maps to index i in the others).
const char *const rateLabels[] = {
    "/16", "/12", "/8", "/6", "/4", "/3", "/2.6", "/2.3", "/2", "/1.6", "/1.3",
    "ofBeat",
    "x1", "x1.3", "x1.5", "x2", "x2.6", "x3", "x4", "x6", "x8", "x12", "x16",
};

// Divisor (PPQN pulses between triggers) for each rateLabels entry.
//   division "/N" -> PPQN * N ;  multiplier "xN" -> PPQN / N (N in thirds where noted)
const uint16_t rateDivisors[] = {
    PPQN * 16,        // /16
    PPQN * 12,        // /12
    PPQN * 8,         // /8
    PPQN * 6,         // /6
    PPQN * 4,         // /4
    PPQN * 3,         // /3
    (PPQN * 8) / 3,   // /2.6  (every 2 2/3 beats)
    (PPQN * 7) / 3,   // /2.3  (every 2 1/3 beats)
    PPQN * 2,         // /2
    (PPQN * 5) / 3,   // /1.6  (every 1 2/3 beats)
    (PPQN * 4) / 3,   // /1.3  (every 1 1/3 beats)
    PPQN,             // ofBeat (same rate as x1, half-beat phase offset)
    PPQN,             // x1
    (PPQN * 3) / 4,   // x1.3  (1 1/3 per beat)
    (PPQN * 2) / 3,   // x1.5  (1 1/2 per beat -> quarter-note triplet)
    PPQN / 2,         // x2
    (PPQN * 3) / 8,   // x2.6  (2 2/3 per beat)
    PPQN / 3,         // x3
    PPQN / 4,         // x4
    PPQN / 6,         // x6
    PPQN / 8,         // x8
    PPQN / 12,        // x12
    PPQN / 16,        // x16
};

// Phase offset (PPQN pulses) for each rateLabels entry. Non-zero only for "off-bt",
// which shifts an x1-rate trigger halfway between beats.
const uint16_t ratePhaseOffsets[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    PPQN / 2,   // ofBeat
    0,          // x1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Index into the rate table that maps to the default divisor ("x1").
static constexpr uint16_t DIVISOR_DEFAULT_INDEX = 12;

// Maps a rate table index onto the matching divisor (PPQN pulses between triggers).
static uint16_t divisorForIndex(uint16_t index)
{
    if (index >= compileTimeLength(rateDivisors))
    {
        return ClockOutput::DEFAULT_DIVISOR;
    }
    return rateDivisors[index];
}

// Maps a rate table index onto the matching phase offset.
static uint16_t phaseOffsetForIndex(uint16_t index)
{
    if (index >= compileTimeLength(ratePhaseOffsets))
    {
        return 0;
    }
    return ratePhaseOffsets[index];
}

// Applies both divisor and phase from a rate table index to a ClockOutput.
static void applyRateIndex(ClockOutput &output, uint16_t index)
{
    output.setDivisor(divisorForIndex(index));
    output.setPhaseOffset(phaseOffsetForIndex(index));
}

// Inverse of applyRateIndex: maps a ClockOutput's divisor + phaseOffset back to
// its rate table index so the menu can be synced from the live output state.
static uint16_t indexForRate(uint16_t divisor, uint16_t phaseOffset)
{
    for (uint16_t i = 0; i < compileTimeLength(rateDivisors); ++i)
    {
        if (rateDivisors[i] == divisor && ratePhaseOffsets[i] == phaseOffset)
        {
            return i;
        }
    }
    return DIVISOR_DEFAULT_INDEX;
}

// DAC amplitude is selected in 0.5 V steps from 0.5 V up to 10 V, where DAC full
// scale (ClockOutput::DAC_MAX) corresponds to AMP_MAX_TENTHS (10.0 V). Each label
// index i maps to (i + 1) * 0.5 V, so index 0 == 0.5 V and the last == 10 V.
static constexpr uint16_t AMP_STEP_TENTHS = 5;   // 0.5 V per step
static constexpr uint16_t AMP_MAX_TENTHS = 100;  // 10.0 V at DAC full scale

const char *const ampVoltLabels[] = {
    "0.5V", "1.0V", "1.5V", "2.0V", "2.5V", "3.0V", "3.5V", "4.0V", "4.5V", "5.0V",
    "5.5V", "6.0V", "6.5V", "7.0V", "7.5V", "8.0V", "8.5V", "9.0V", "9.5V", "10.0V",
};

// Volts (in tenths) for a given amplitude label index.
static uint16_t voltTenthsForIndex(uint16_t index)
{
    return static_cast<uint16_t>((index + 1u) * AMP_STEP_TENTHS);
}

// Converts an amplitude label index to a 12-bit DAC value.
static uint16_t dacFromVoltIndex(uint16_t index)
{
    uint16_t tenths = voltTenthsForIndex(index);
    if (tenths > AMP_MAX_TENTHS)
    {
        tenths = AMP_MAX_TENTHS;
    }
    return static_cast<uint16_t>((static_cast<uint32_t>(tenths) * ClockOutput::DAC_MAX) / AMP_MAX_TENTHS);
}

// Converts a 12-bit DAC value back to the nearest amplitude label index.
static uint16_t voltIndexFromDac(uint16_t dacValue)
{
    if (dacValue > ClockOutput::DAC_MAX)
    {
        dacValue = ClockOutput::DAC_MAX;
    }
    // Round DAC value to the nearest 0.1 V, then snap to a 0.5 V label index.
    uint32_t tenths = (static_cast<uint32_t>(dacValue) * AMP_MAX_TENTHS + ClockOutput::DAC_MAX / 2) / ClockOutput::DAC_MAX;
    if (tenths < AMP_STEP_TENTHS)
    {
        tenths = AMP_STEP_TENTHS; // clamp up to the 0.5 V minimum
    }
    uint16_t index = static_cast<uint16_t>(tenths / AMP_STEP_TENTHS - 1u);
    const uint16_t maxIndex = static_cast<uint16_t>(compileTimeLength(ampVoltLabels) - 1);
    return index > maxIndex ? maxIndex : index;
}

const char *const metronomeSourceLabels[] = {"EXT.", "INT.", "MIDI", "LINK"};

MenuItem inputsOptions[] = {
    makeBackItem(),
    {M_INPUT_IN, "IN", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_SS, "SS", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_RESET, "RESET", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
    {M_INPUT_MIDI, "MIDI", nullptr, 0, 0, false, false, 0, 100, 1, nullptr, 0},
};

// OUT 3 drives the DAC, so it gets its own submenu with a trigger RATE (division)
// and an output AMP (amplitude) rather than being a single division picker.
MenuItem output3Options[] = {
    {M_OUTPUT_3_RATE, "RATE", nullptr, 0, DIVISOR_DEFAULT_INDEX, false, false, 0, compileTimeLength(rateLabels) - 1, 1, rateLabels, compileTimeLength(rateLabels)},
    {M_OUTPUT_3_AMP, "AMP", nullptr, 0, static_cast<uint16_t>(compileTimeLength(ampVoltLabels) - 1), false, false, 0, compileTimeLength(ampVoltLabels) - 1, 1, ampVoltLabels, compileTimeLength(ampVoltLabels)},
    makeBackItem(),
};

MenuItem outputsOptions[] = {
    {M_OUTPUT_1, "OUT 1", nullptr, 0, DIVISOR_DEFAULT_INDEX, false, false, 0, compileTimeLength(rateLabels) - 1, 1, rateLabels, compileTimeLength(rateLabels)},
    {M_OUTPUT_2, "OUT 2", nullptr, 0, DIVISOR_DEFAULT_INDEX, false, false, 0, compileTimeLength(rateLabels) - 1, 1, rateLabels, compileTimeLength(rateLabels)},
    {M_OUTPUT_3, "OUT 3", output3Options, compileTimeLength(output3Options), 0, true, false, 0, 100, 1, nullptr, 0},
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
            eeprom.writeByte(EEPROM_ADDR_CLOCK_MODE, static_cast<uint8_t>(item.value));
            break;
        case M_OUTPUT_1:
            applyRateIndex(output1, item.value);
            break;
        case M_OUTPUT_2:
            applyRateIndex(output2, item.value);
            break;
        case M_OUTPUT_3_RATE:
            applyRateIndex(output3, item.value);
            break;
        case M_OUTPUT_3_AMP:
            output3.setAmplitude(dacFromVoltIndex(item.value));
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
        case M_OUTPUT_1:
            item.value = indexForRate(output1.divisor, output1.phaseOffset);
            break;
        case M_OUTPUT_2:
            item.value = indexForRate(output2.divisor, output2.phaseOffset);
            break;
        case M_OUTPUT_3_RATE:
            item.value = indexForRate(output3.divisor, output3.phaseOffset);
            break;
        case M_OUTPUT_3_AMP:
            item.value = voltIndexFromDac(output3.amplitude);
            break;
        default:
            break;
        }
    }
}