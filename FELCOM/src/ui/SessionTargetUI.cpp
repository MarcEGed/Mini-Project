#include "SessionTargetUI.h"

#include <config.h>
#include <display.h>
#include <transceiver.h>

namespace {

static uint8_t gSelectionIndex = 0;
static constexpr uint8_t kMaxVisibleNodes = 3;

static uint8_t optionCount(const transceiver& xcvr) {
    return static_cast<uint8_t>(1 + xcvr.nbSyncedNodes);
}

static void syncSelectionToCurrentTarget(const transceiver& xcvr,
                                         uint8_t currentDstNode) {
    gSelectionIndex = 0;

    if (currentDstNode == 0xFF) {
        return;
    }

    for (uint8_t i = 0; i < xcvr.nbSyncedNodes; ++i) {
        if (xcvr.syncedNodes[i] == currentDstNode) {
            gSelectionIndex = static_cast<uint8_t>(i + 1);
            return;
        }
    }
}

static uint8_t selectedNodeId(const transceiver& xcvr) {
    if (gSelectionIndex == 0) {
        return 0xFF;
    }

    uint8_t nodeIndex = static_cast<uint8_t>(gSelectionIndex - 1);
    if (nodeIndex < xcvr.nbSyncedNodes) {
        return xcvr.syncedNodes[nodeIndex];
    }

    return 0xFF;
}

static void render(const transceiver& xcvr) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(18, 0);
    display.print("SELECT TARGET");
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

    const uint8_t totalOptions = optionCount(xcvr);
    uint8_t startIndex = 0;
    if (gSelectionIndex >= kMaxVisibleNodes) {
        startIndex = static_cast<uint8_t>(gSelectionIndex - kMaxVisibleNodes + 1);
    }

    for (uint8_t row = 0; row < kMaxVisibleNodes; ++row) {
        uint8_t optionIndex = static_cast<uint8_t>(startIndex + row);
        if (optionIndex >= totalOptions) {
            break;
        }

        uint8_t y = static_cast<uint8_t>(16 + row * 14);
        bool selected = (optionIndex == gSelectionIndex);
        if (selected) {
            display.fillRect(2, y - 1, SCREEN_WIDTH - 4, 11, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        }

        display.setCursor(6, y);
        display.print(selected ? "> " : "  ");
        if (optionIndex == 0) {
            display.print("Broadcast");
        } else {
            uint8_t nodeId = xcvr.syncedNodes[optionIndex - 1];
            display.print("Node ");
            display.print(nodeId);
        }
    }

    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.drawFastHLine(0, 54, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(2, 56);
    display.print("SEL confirm");

    display.display();
}

}  // namespace

bool sessionTargetUITickInput(transceiver& xcvr, int8_t dirY, bool btnDown,
                              uint8_t currentDstNode,
                              uint8_t* outSelectedDstNode,
                              bool* outConfirmed) {
    (void)currentDstNode;

    bool changed = false;
    const uint8_t totalOptions = optionCount(xcvr);

    if (outConfirmed != nullptr) {
        *outConfirmed = false;
    }

    if (dirY != 0 && totalOptions > 0) {
        if (dirY < 0) {
            gSelectionIndex = static_cast<uint8_t>((gSelectionIndex + 1) % totalOptions);
        } else {
            gSelectionIndex = static_cast<uint8_t>((gSelectionIndex + totalOptions - 1) % totalOptions);
        }
        changed = true;
    }

    if (btnDown) {
        if (outSelectedDstNode != nullptr) {
            *outSelectedDstNode = selectedNodeId(xcvr);
        }
        if (outConfirmed != nullptr) {
            *outConfirmed = true;
        }
        changed = true;
    }

    return changed;
}

void sessionTargetUIInitDisplay(transceiver& xcvr, uint8_t currentDstNode) {
    syncSelectionToCurrentTarget(xcvr, currentDstNode);
    render(xcvr);
}

void sessionTargetUIUpdate(transceiver& xcvr) { render(xcvr); }
