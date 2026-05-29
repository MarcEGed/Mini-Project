#include "MenuUI.h"

#include <config.h>
#include <display.h>

constexpr uint8_t MENU_SELECTION_COUNT = 5;

MenuModeSelection g_selection = MenuChat;

#define TOP_BAR_HEIGHT 15

void renderMenu() {
    display.clearDisplay();

    // Box around the screen
    display.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, SSD1306_WHITE);
    // Divider line for top bar
    display.drawFastHLine(2, TOP_BAR_HEIGHT + 2, SCREEN_WIDTH - 4,
                          SSD1306_WHITE);

    // Top Bar Text
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Center FELCOM-OS
    const char* title = "FELCOM-OS";
    int16_t titleWidth = strlen(title) * 6;
    display.setCursor((SCREEN_WIDTH - titleWidth) / 2, 6);
    display.print(title);
    display.setCursor((SCREEN_WIDTH - titleWidth) / 2 + 1,
                      6);  // slight bold effect
    display.print(title);

    // Menu Items
    uint8_t startY = TOP_BAR_HEIGHT + 8;
    const char* items[] = {"CHAT", "PONG", "RF TEST", "ABOUT", "SYNC"};

    uint8_t maxVisible = 3;
    uint8_t startIndex = 0;
    if (g_selection >= maxVisible) {
        startIndex = g_selection - maxVisible + 1;
    }

    for (uint8_t i = 0;
         i < maxVisible && (startIndex + i) < MENU_SELECTION_COUNT; i++) {
        uint8_t actualIndex = startIndex + i;
        uint8_t itemY =
            startY + (i * 13);  // Slightly increased spacing to fit better

        if (actualIndex == g_selection) {
            display.fillRect(4, itemY - 2, SCREEN_WIDTH - 12, 11,
                             SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            display.setCursor(6, itemY);
            display.print("> ");
            display.print(items[actualIndex]);
        } else {
            display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            display.setCursor(6, itemY);
            display.print("  ");
            display.print(items[actualIndex]);
        }
    }

    // Draw a small scrollbar handle on the right
    uint8_t scrollHeight = (maxVisible * 16) / MENU_SELECTION_COUNT;
    if (scrollHeight < 5) scrollHeight = 5;
    uint8_t scrollArea = SCREEN_HEIGHT - TOP_BAR_HEIGHT - 8 - scrollHeight;
    uint8_t scrollY =
        TOP_BAR_HEIGHT + 4 +
        (startIndex * scrollArea) / (MENU_SELECTION_COUNT - maxVisible);

    display.fillRect(SCREEN_WIDTH - 6, scrollY, 3, scrollHeight, SSD1306_WHITE);

    display.display();
}

void menuUIInitDisplay() {
    g_selection = MenuChat;
    renderMenu();
}

void menuUIUpdate() { renderMenu(); }

bool menuUIUpdateSelection(int8_t dirY) {
    // return true if we should update the display (selection changed)
    if (dirY == 0) {
        return false;
    }

    uint8_t index = static_cast<uint8_t>(g_selection);
    if (dirY < 0) {
        index = (index + 1) % MENU_SELECTION_COUNT;
    } else {
        index = (index + MENU_SELECTION_COUNT - 1) % MENU_SELECTION_COUNT;
    }
    g_selection = static_cast<MenuModeSelection>(index);
    return true;  // We changed selection, UI needs redrawing
}

MenuModeSelection menuUIGetSelection() { return g_selection; }