#include "AboutUI.h"

#include <config.h>
#include <display.h>

void aboutUIInitDisplay() {
    display.clearDisplay();

    // Box around the screen
    display.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, SSD1306_WHITE);
    // Divider line for top bar
    display.drawFastHLine(2, 17, SCREEN_WIDTH - 4,
                          SSD1306_WHITE);  // matching MenuUI TOP_BAR_HEIGHT + 2

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

    char ver[12];
    snprintf(ver, sizeof(ver), "%.9s", FELCOM_VERSION);
    int16_t verWidth = strlen(ver) * 6;

    // Items
    uint8_t startY = 22;
    uint8_t lineSpacing = 12;

    display.setCursor(6, startY);
    display.print("VERSION:");
    display.setCursor(
        SCREEN_WIDTH - verWidth - 11,
        startY);  // -11 to account for slightly smaller width or standard right
                  // padding + scroll handle missing space
    display.print(ver);

    char idStr[16];
    snprintf(idStr, sizeof(idStr), "0X%02X", SENDER_ID);
    display.setCursor(6, startY + lineSpacing);
    display.print("NODE ID:");
    int16_t idWidth = strlen(idStr) * 6;
    display.setCursor(SCREEN_WIDTH - idWidth - 11, startY + lineSpacing);
    display.print(idStr);

    char nameStr[16];
    snprintf(nameStr, sizeof(nameStr), "%c", (char)NODE_NAME);
    display.setCursor(6, startY + 2 * lineSpacing);
    display.print("NAME:");
    int16_t nameWidth = strlen(nameStr) * 6;
    display.setCursor(SCREEN_WIDTH - nameWidth - 11, startY + 2 * lineSpacing);
    display.print(nameStr);

    display.display();
}

void aboutUIUpdate() {
    // static, no update needed unless we add animations or real-time info
}