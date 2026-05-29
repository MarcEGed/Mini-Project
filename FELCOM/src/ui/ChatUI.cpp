#include "ChatUI.h"

#include <display.h>
#include <stdio.h>
#include <transceiver.h>

// current design:
//   rows 0-7   : status bar (channel + timer)
//   rows 10-41 : last 2 messages (16px each)
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

static void renderStatusBar(const transceiver* xcvr) {
    display.fillRect(0, 0, 128, 8, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    uint8_t channel = 0;
    uint8_t peers = 0;
    if (xcvr && xcvr->radio) {
        channel = xcvr->radio->getChannel();
    }
    if (xcvr) {
        peers = xcvr->syncedPeerCount();
    }

    char bar[22];
    if (xcvr && xcvr->fhss_timer) {
        uint64_t ticks = timerRead(xcvr->fhss_timer);
        uint32_t ticks_short = static_cast<uint32_t>(ticks % 100000000ULL);
        snprintf(bar, sizeof(bar), "C%u N%u T%lu", channel, peers,
                 static_cast<unsigned long>(ticks_short));
    } else {
        snprintf(bar, sizeof(bar), "C%u N%u T-", channel, peers);
    }

    display.print(bar);
    display.drawFastHLine(0, 8, 128, SSD1306_WHITE);
}

static void renderChatStatusBar(const transceiver* xcvr) {
    renderStatusBar(xcvr);
    display.display();
}

void renderChatMain(chatLog& log, chatInput& input,
                    const transceiver* xcvr) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    renderStatusBar(xcvr);

    const uint8_t lines = min((uint8_t)2, log.size());
    const uint8_t logTop = 10;
    const uint8_t lineHeight = 16;

    for (uint8_t i = 0; i < lines; i++) {
        const ChatMessage* m = log.get(lines - 1 - i);
        display.setCursor(0, logTop + (i * lineHeight));
        char line[22];
        snprintf(line, sizeof(line), "%c:%s", m->senderId, m->text);
        display.print(line);
    }

    display.drawFastHLine(0, 48, 128, SSD1306_WHITE);
    printInputLine(input);
    display.display();
}

void renderChatInputLine(chatInput& input, const transceiver* xcvr) {
    renderStatusBar(xcvr);
    display.fillRect(0, 56, 128, 8, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    printInputLine(input);
    display.display();
}

void renderChat(chatLog& log, chatInput& input, const transceiver* xcvr) {
    renderChatMain(log, input, xcvr);
}

void chatUIInitDisplay(ChatHandler* chat, const transceiver* xcvr) {
    if (chat == nullptr) {
        return;
    }
    renderChatMain(chat->log, chat->input, xcvr);
}

void chatUIUpdate(ChatHandler* chat, ChatUpdateKind kind,
                  const transceiver* xcvr) {
    if (chat == nullptr) {
        return;
    }

    if (kind == ChatInputLine) {
        renderChatInputLine(chat->input, xcvr);
        return;
    }

    if (kind == ChatStatusBar) {
        renderChatStatusBar(xcvr);
        return;
    }

    renderChatMain(chat->log, chat->input, xcvr);
}
