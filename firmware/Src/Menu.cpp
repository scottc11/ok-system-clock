#include "Menu.h"
#include <cstdio>

Menu::Menu(MenuItem *rootNode)
{
    setRoot(rootNode);
}

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

bool Menu::findMenuPath(uint16_t targetId, SearchResult *outResult) const
{
    MenuItem *emptyAncestors[MENU_STACK_MAX_DEPTH] = {nullptr};
    return findMenuPathRecursive(rootNode, targetId, emptyAncestors, 0, outResult);
}

void Menu::setRoot(MenuItem *newRootNode)
{
    rootNode = newRootNode;
    enter();
}

void Menu::enter()
{
    currentMenu = rootNode;
    selectedIndex = 0;
    menuStackDepth = 0;
    valueEditMode = false;
}

/**
 * @brief Jump to a menu item by its ID. If the ID is 0x00, the root menu is entered.
 * 
 * @param id The ID of the menu item to jump to
 * @return True if the jump was successful, false otherwise
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
    return true;
}

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

const MenuItem *Menu::getActiveMenuItem() const
{
    if (currentMenu == nullptr || currentMenu->optionCount == 0)
    {
        return nullptr;
    }

    const uint8_t boundedIndex = (selectedIndex >= currentMenu->optionCount) ? 0 : selectedIndex;
    return &currentMenu->options[boundedIndex];
}

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

const char *Menu::getActiveItemText()
{
    formatActiveItem(textBuffer, sizeof(textBuffer));
    return textBuffer;
}

bool Menu::isValueEditMode() const
{
    return valueEditMode;
}