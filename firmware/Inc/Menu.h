#pragma once

#include <stddef.h>
#include <stdint.h>

static constexpr uint8_t MENU_STACK_MAX_DEPTH = 16;
static constexpr size_t MENU_TEXT_BUFFER_SIZE = 20;

struct MenuItem
{
    uint16_t id;
    const char *text;               // Label shown for this menu entry
    MenuItem *options;              // Child menu entries (nullptr for leaf items)
    uint8_t optionCount;            // Number of valid child entries in options
    uint16_t value;                 // Stored value/index for this item
    bool hasSubmenus;               // True when this item opens a submenu
    bool isBack;                    // True when this item navigates back one level
    uint16_t minValue;              // Minimum allowed numeric value
    uint16_t maxValue;              // Maximum allowed numeric value
    uint16_t step;                  // Increment/decrement amount for numeric edits
    const char *const *valueLabels; // Optional lookup table for indexed text values
    uint8_t valueLabelCount;        // Number of entries in valueLabels
};

class Menu
{
public:
    explicit Menu(MenuItem *rootNode = nullptr);

    MenuItem *rootNode = nullptr;
    MenuItem *currentMenu = nullptr;
    uint8_t selectedIndex = 0;
    MenuItem *menuStack[MENU_STACK_MAX_DEPTH] = {nullptr};
    uint8_t menuStackDepth = 0;
    bool valueEditMode = false;

    void setRoot(MenuItem *rootNode);
    void enter();
    bool jumpToMenuItem(uint16_t id);

    MenuItem *getActiveMenuItem();
    const MenuItem *getActiveMenuItem() const;

    bool handleRotation(int8_t delta);
    void handleSelect();

    // Formats the active item's label into the internal buffer and returns a
    // pointer to it. The buffer is owned by this Menu instance and remains
    // valid until the next call. The external display code reads from it.
    const char *getActiveItemText();

    // Formats the active item's label into a caller-provided buffer. Use this
    // when the display layer owns the memory (the safest, most flexible form).
    void formatActiveItem(char *buf, size_t bufSize) const;
    bool isValueEditMode() const;

private:
    char textBuffer[MENU_TEXT_BUFFER_SIZE] = {0};

    struct SearchResult
    {
        MenuItem *currentMenu;
        uint8_t selectedIndex;
        MenuItem *stack[MENU_STACK_MAX_DEPTH];
        uint8_t stackDepth;
    };

    bool findMenuPath(uint16_t targetId, SearchResult *outResult) const;
    bool findMenuPathRecursive(
        MenuItem *menuNode,
        uint16_t targetId,
        MenuItem *const *ancestorStack,
        uint8_t ancestorDepth,
        SearchResult *outResult) const;
};
