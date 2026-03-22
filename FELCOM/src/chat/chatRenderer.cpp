#include "chatRenderer.h"
#include "display.h"
#include <string.h>

// current design:
//   rows 0-47  : last 3 messages (16px each)
//   row  48-55 : divider + ">" prompt
//   rows 56-63 : current input buffer

void renderChat(chatLog& log, chatInput& input){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    uint8_t lines = min((uint8_t)3, log.size()); // only draw what exists

    for (uint8_t i = 0; i < lines; i++) {
        const Message* m = log.get(lines - 1 - i);
        display.setCursor(0, i * 16);
        char line[22];
        snprintf(line, sizeof(line), "%c:%s", m->senderName, m->text);
        display.print(line);
    }

    display.drawFastHLine(0, 48, 128, SSD1306_WHITE);

    display.setCursor(0, 56);
    char inputLine[22];
    if (input.currentChar == '\x7F') {
        snprintf(inputLine, sizeof(inputLine), ">%s[OK]", input.buffer);
    } else {
        snprintf(inputLine, sizeof(inputLine), ">%s[%c]", input.buffer, input.currentChar);
    }
    display.print(inputLine);

    display.display();
}