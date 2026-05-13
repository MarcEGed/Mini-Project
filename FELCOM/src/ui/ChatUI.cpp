#include "ChatUI.h"

#include <display.h>
#include <stdio.h>

// current design:
//   rows 0-47  : last 3 messages (16px each)
//   row  48-55 : divider + ">" prompt
//   rows 56-63 : current input buffer

static void printInputLine(chatInput& input) {
    display.setCursor(0, 56);
    char inputLine[22];
    const char current = input.selectedChar();
    if (current == '\x7F') {
        snprintf(inputLine, sizeof(inputLine), ">%s[OK]", input.buffer);
    } else {
        snprintf(inputLine, sizeof(inputLine), ">%s[%c]", input.buffer,
                 current);
    }
    display.print(inputLine);
}

void renderChatMain(chatLog& log, chatInput& input) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    uint8_t lines = min((uint8_t)3, log.size());

    for (uint8_t i = 0; i < lines; i++) {
        const Message* m = log.get(lines - 1 - i);
        display.setCursor(0, i * 16);
        char line[22];
        snprintf(line, sizeof(line), "%c:%s", m->senderId, m->text);
        display.print(line);
    }

    display.drawFastHLine(0, 48, 128, SSD1306_WHITE);
    printInputLine(input);
    display.display();
}

void renderChatInputLine(chatInput& input) {
    display.fillRect(0, 56, 128, 8, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    printInputLine(input);
    display.display();
}

void renderChat(chatLog& log, chatInput& input) { renderChatMain(log, input); }

void chatUIInitDisplay(ChatHandler* chat) {
    if (chat == nullptr) {
        return;
    }
    renderChatMain(chat->log, chat->input);
}

void chatUIUpdate(ChatHandler* chat, ChatUpdateKind kind) {
    if (chat == nullptr) {
        return;
    }

    if (kind == ChatInputLine) {
        renderChatInputLine(chat->input);
        return;
    }

    renderChatMain(chat->log, chat->input);
}
