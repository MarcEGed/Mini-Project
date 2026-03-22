#include "chatRenderer.h"
#include "display.h"
#include <string.h>

// current design:
//   rows 0-47  : last 3 messages (16px each)
//   row  48-55 : divider + ">" prompt
//   rows 56-63 : current input buffer

void renderChat(chatLog& log, chatInput& input){
    display.clearDisplay();
    display.setTextSize(1);    //6x8 px per char → 21 chars wide
    display.setTextColor(SSD1306_WHITE);

    //draw up to 3 recent messages
    uint8_t lines = min((uint8_t)3, log.size());
    for (uint8_t i = 0; i < lines; i++) {
        Message* m = log.get(lines - 1 - i); //oldest at top
        display.setCursor(0, i * 16);

        //"A: HELLO WORLD" — truncate to fit 21 chars
        char line[22];
        snprintf(line, sizeof(line), "%c:%s", m->senderName, m->text);
        display.print(line);
    }

    //divider
    display.drawFastHLine(0, 48, 128, SSD1306_WHITE);

    //input row: "> BUF_[C]" where C is the spinning char
    display.setCursor(0, 56);
    char inputLine[22];
    const char* cursorLabel = (input.currentChar == '\x7F') ? "\x7F" : nullptr;

    if (input.currentChar == '\x7F') {
        snprintf(inputLine, sizeof(inputLine), ">%s[OK]", input.buffer);
    } else {
        snprintf(inputLine, sizeof(inputLine), ">%s[%c]", input.buffer, input.currentChar);
    }
    display.print(inputLine);

    display.display();
}