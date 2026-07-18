#include "Menu.h"
#include <cstdio>

/**
 * @brief Construct a Menu, optionally binding it to a root menu tree.
 *
 * @param rootNode Pointer to the root MenuItem (may be nullptr and set later via setRoot).
 */
Menu::Menu(MenuItem *rootNode)
{
    setRoot(rootNode);
}

/**
 * @brief Depth-first search for a menu item by ID, recording the path to it.
 *
 * Walks @p menuNode's children (and their sub-trees) looking for an item whose
 * id matches @p targetId. When found, @p outResult is populated with the parent
 * menu, the child's index, and the ancestor stack needed to navigate back out.
 * Recursion is bounded by MENU_STACK_MAX_DEPTH to prevent stack overflow.
 *
 * @param menuNode Menu whose children are searched (its own id is not tested).
 * @param targetId ID of the menu item to locate.
 * @param ancestorStack Stack of ancestor menus leading to @p menuNode.
 * @param ancestorDepth Number of valid entries in @p ancestorStack.
 * @param outResult Out-param populated with the located item's navigation state.
 * @return True if the item was found, false otherwise.
 */
bool Menu::findMenuPathRecursive(
    MenuItem *menuNode,
    uint16_t targetId,
    MenuItem *const *ancestorStack,
    uint8_t ancestorDepth,
    SearchResult *outResult) const
{
    if (menuNode == nullptr || outResult == nullptr)
    {
        return false;
    }

    for (uint8_t i = 0; i < menuNode->optionCount; ++i)
    {
        MenuItem *item = &menuNode->options[i];
        if (item->id == targetId)
        {
            outResult->currentMenu = menuNode;
            outResult->selectedIndex = i;
            outResult->stackDepth = ancestorDepth;
            for (uint8_t s = 0; s < ancestorDepth; ++s)
            {
                outResult->stack[s] = ancestorStack[s];
            }
            return true;
        }

        if (item->hasSubmenus && item->options != nullptr && item->optionCount > 0 && ancestorDepth < MENU_STACK_MAX_DEPTH)
        {
            MenuItem *nextAncestors[MENU_STACK_MAX_DEPTH] = {nullptr};
            for (uint8_t s = 0; s < ancestorDepth; ++s)
            {
                nextAncestors[s] = ancestorStack[s];
            }
            nextAncestors[ancestorDepth] = menuNode;

            if (findMenuPathRecursive(item, targetId, nextAncestors, static_cast<uint8_t>(ancestorDepth + 1U), outResult))
            {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Search the whole menu tree for an item by ID, starting from the root.
 *
 * Thin wrapper over findMenuPathRecursive that seeds the search with an empty
 * ancestor stack rooted at rootNode.
 *
 * @param targetId ID of the menu item to locate.
 * @param outResult Out-param populated with the located item's navigation state.
 * @return True if the item was found, false otherwise.
 */
bool Menu::findMenuPath(uint16_t targetId, SearchResult *outResult) const
{
    MenuItem *emptyAncestors[MENU_STACK_MAX_DEPTH] = {nullptr};
    return findMenuPathRecursive(rootNode, targetId, emptyAncestors, 0, outResult);
}

/**
 * @brief Bind the menu to a new root tree and reset navigation to that root.
 *
 * @param newRootNode Pointer to the new root MenuItem.
 */
void Menu::setRoot(MenuItem *newRootNode)
{
    rootNode = newRootNode;
    enter();
}

/**
 * @brief Reset navigation state to the root menu.
 *
 * Clears the selection, navigation stack, and value-edit mode so the menu
 * displays the top level from a known state.
 */
void Menu::enter()
{
    currentMenu = rootNode;
    selectedIndex = 0;
    menuStackDepth = 0;
    valueEditMode = false;
}

/**
 * @brief Navigate directly to a menu item by its ID.
 *
 * Searches the tree for @p id and, if found, restores the full navigation
 * context (current menu, selection index, and back-navigation stack) so the
 * item appears selected as if reached manually. Value-edit mode is enabled when
 * the target is an editable leaf. An @p id of 0x00 is treated as the root and
 * resets navigation via enter().
 *
 * @param id The ID of the menu item to jump to (0x00 selects the root).
 * @return True if the jump succeeded, false if there is no root or the ID was not found.
 */
bool Menu::jumpToMenuItem(uint16_t id)
{
    if (rootNode == nullptr)
    {
        return false;
    }

    if (id == 0x00)
    {
        enter();
        return true;
    }

    SearchResult result{};
    if (!findMenuPath(id, &result))
    {
        return false;
    }

    currentMenu = result.currentMenu;
    selectedIndex = result.selectedIndex;
    menuStackDepth = result.stackDepth;

    for (uint8_t i = 0; i < MENU_STACK_MAX_DEPTH; ++i)
    {
        menuStack[i] = (i < menuStackDepth) ? result.stack[i] : nullptr;
    }

    MenuItem *activeItem = getActiveMenuItem();
    valueEditMode = (activeItem != nullptr) && !activeItem->hasSubmenus && !activeItem->isBack;

    if (jumpCallback)
    {
        jumpCallback(activeItem);
    }

    return true;
}

/**
 * @brief Attach a callback fired when jumpToMenuItem() locates an item.
 *
 * The callback runs whenever jumpToMenuItem() successfully finds the requested
 * item, receiving a pointer to the located MenuItem.
 *
 * @param callback The callback to attach.
 */
void Menu::attachJumpCallback(Callback<void(MenuItem *item)> callback)
{
    jumpCallback = callback;
}

/**
 * @brief Get the currently selected item in the active menu.
 *
 * If the stored selection index is out of range it is clamped back to 0, so
 * this overload may modify selectedIndex as a side effect.
 *
 * @return Pointer to the selected MenuItem, or nullptr if the menu is empty.
 */
MenuItem *Menu::getActiveMenuItem()
{
    if (currentMenu == nullptr || currentMenu->optionCount == 0)
    {
        return nullptr;
    }

    if (selectedIndex >= currentMenu->optionCount)
    {
        selectedIndex = 0;
    }

    return &currentMenu->options[selectedIndex];
}

/**
 * @brief Get the currently selected item in the active menu (read-only).
 *
 * Const overload: an out-of-range selection index is treated as 0 for this
 * call without mutating any state.
 *
 * @return Pointer to the selected MenuItem, or nullptr if the menu is empty.
 */
const MenuItem *Menu::getActiveMenuItem() const
{
    if (currentMenu == nullptr || currentMenu->optionCount == 0)
    {
        return nullptr;
    }

    const uint8_t boundedIndex = (selectedIndex >= currentMenu->optionCount) ? 0 : selectedIndex;
    return &currentMenu->options[boundedIndex];
}

/**
 * @brief Handle a rotary encoder step, either moving the selection or editing a value.
 *
 * When not in value-edit mode, the selection cursor wraps through the current
 * menu's items. In value-edit mode the active item's value is adjusted: indexed
 * items (with valueLabels) wrap through their labels, while numeric items step
 * by the item's step amount and clamp to [minValue, maxValue].
 *
 * @param delta Direction of rotation: positive increments, non-positive decrements.
 * @return True if a value was changed in value-edit mode, false otherwise
 *         (including plain selection movement).
 */
bool Menu::handleRotation(int8_t delta)
{
    MenuItem *activeItem = getActiveMenuItem();
    if (activeItem == nullptr)
    {
        return false;
    }

    if (!valueEditMode)
    {
        if (delta > 0)
        {
            selectedIndex = static_cast<uint8_t>((selectedIndex + 1U) % currentMenu->optionCount);
        }
        else if (selectedIndex == 0)
        {
            selectedIndex = static_cast<uint8_t>(currentMenu->optionCount - 1U);
        }
        else
        {
            --selectedIndex;
        }
        return false;
    }

    const uint16_t oldValue = activeItem->value;
    if (activeItem->valueLabels != nullptr && activeItem->valueLabelCount > 0)
    {
        if (delta > 0)
        {
            activeItem->value = (activeItem->value + 1u >= activeItem->valueLabelCount)
                ? 0u
                : static_cast<uint16_t>(activeItem->value + 1u);
        }
        else
        {
            activeItem->value = (activeItem->value == 0u)
                ? static_cast<uint16_t>(activeItem->valueLabelCount - 1u)
                : static_cast<uint16_t>(activeItem->value - 1u);
        }
    }
    else
    {
        const uint16_t step = (activeItem->step == 0u) ? 1u : activeItem->step;
        if (delta > 0)
        {
            activeItem->value = (activeItem->value > activeItem->maxValue - step)
                ? activeItem->maxValue
                : static_cast<uint16_t>(activeItem->value + step);
        }
        else
        {
            activeItem->value = (activeItem->value < activeItem->minValue + step)
                ? activeItem->minValue
                : static_cast<uint16_t>(activeItem->value - step);
        }
    }

    return activeItem->value != oldValue;
}

/**
 * @brief Handle a select/press action on the active item.
 *
 * Behavior depends on context: in value-edit mode it commits the value and
 * exits edit mode; a "back" item pops one level off the navigation stack; an
 * item with submenus descends into it; and a plain leaf item enters value-edit
 * mode.
 */
void Menu::handleSelect()
{
    MenuItem *activeItem = getActiveMenuItem();
    if (activeItem == nullptr)
    {
        return;
    }

    if (valueEditMode)
    {
        valueEditMode = false;
        return;
    }

    if (activeItem->isBack)
    {
        if (menuStackDepth > 0)
        {
            --menuStackDepth;
            currentMenu = menuStack[menuStackDepth];
            selectedIndex = 0;
        }
        return;
    }

    if (activeItem->hasSubmenus)
    {
        if (menuStackDepth < MENU_STACK_MAX_DEPTH)
        {
            menuStack[menuStackDepth] = currentMenu;
            ++menuStackDepth;
            currentMenu = activeItem;
            selectedIndex = 0;
        }
        return;
    }

    valueEditMode = true;
}

/**
 * @brief Render the active item's display label into a caller-provided buffer.
 *
 * Produces the text to show for the current selection: the item's label for
 * submenus and non-editing leaves, "< Back" for back items, the matching
 * valueLabels entry (or the raw numeric value) while editing. Output is always
 * null-terminated and truncated to @p bufSize. Use this overload when the
 * caller owns the destination memory.
 *
 * @param buf Destination buffer (must be non-null).
 * @param bufSize Size of @p buf in bytes (must be greater than 0).
 */
void Menu::formatActiveItem(char *buf, size_t bufSize) const
{
    if (buf == nullptr || bufSize == 0)
    {
        return;
    }

    buf[0] = '\0';

    const MenuItem *activeItem = getActiveMenuItem();
    if (activeItem == nullptr)
    {
        return;
    }

    if (activeItem->hasSubmenus)
    {
        std::snprintf(buf, bufSize, "%s", activeItem->text);
    }
    else if (activeItem->isBack)
    {
        std::snprintf(buf, bufSize, "< Back");
    }
    else if (activeItem->valueLabels != nullptr && activeItem->valueLabelCount > 0)
    {
        if (valueEditMode)
        {
            const uint16_t valueIndex = (activeItem->value >= activeItem->valueLabelCount)
                ? static_cast<uint16_t>(activeItem->valueLabelCount - 1)
                : activeItem->value;
            std::snprintf(buf, bufSize, "%s", activeItem->valueLabels[valueIndex]);
        }
        else
        {
            std::snprintf(buf, bufSize, "%s", activeItem->text);
        }
    }
    else
    {
        if (valueEditMode)
        {
            std::snprintf(buf, bufSize, "%u", static_cast<unsigned int>(activeItem->value));
        }
        else
        {
            std::snprintf(buf, bufSize, "%s", activeItem->text);
        }
    }
}

/**
 * @brief Render the active item's label into the internal buffer and return it.
 *
 * Convenience wrapper over formatActiveItem that uses this Menu's own buffer.
 * The returned pointer is owned by the Menu and stays valid only until the next
 * call to this method; copy the text out if it must persist.
 *
 * @return Pointer to the null-terminated label text (never nullptr).
 */
const char *Menu::getActiveItemText()
{
    formatActiveItem(textBuffer, sizeof(textBuffer));
    return textBuffer;
}

/**
 * @brief Report whether the menu is currently editing an item's value.
 *
 * @return True if in value-edit mode, false if navigating the menu.
 */
bool Menu::isValueEditMode() const
{
    return valueEditMode;
}