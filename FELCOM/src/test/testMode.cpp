#include "testMode.h"

#define MAX_READS_PER_LOOP 6

struct TestPacket {
    uint32_t seq;
    uint8_t payload[28];
};

static_assert(sizeof(TestPacket) == RF_TEST_PACKET_SIZE,
              "TestPacket size must match RF_TEST_PACKET_SIZE");

static uint32_t sent     = 0;
static uint32_t echoed   = 0;
static uint32_t lost     = 0;
static uint32_t corrupt  = 0;

static uint32_t nextSeq      = 0;
static uint32_t pendingSeq   = 0;
static bool     awaitingEcho = false;
static uint32_t pendingMs    = 0;
static uint32_t lastSendMs   = 0;

// ── display ──────────────────────────────────────────────────────────────────

static void renderTx() {
    float ber     = echoed > 0 ? 100.0f * corrupt / echoed : 0.0f;
    float lossP   = sent   > 0 ? 100.0f * lost    / sent   : 0.0f;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // big BER number
    display.setTextSize(3);
    char berStr[8];
    snprintf(berStr, sizeof(berStr), "%.0f%%", ber);
    display.setCursor(0, 0);
    display.print(berStr);

    display.setTextSize(1);
    display.setCursor(80, 4);
    display.print("BER");

    display.drawFastHLine(0, 26, 128, SSD1306_WHITE);

    // counters
    char line[22];
    snprintf(line, sizeof(line), "Sent:  %lu", (unsigned long)sent);
    display.setCursor(0, 30); display.print(line);

    snprintf(line, sizeof(line), "Echo:  %lu", (unsigned long)echoed);
    display.setCursor(0, 40); display.print(line);

    snprintf(line, sizeof(line), "Lost:  %lu (%.0f%%)", (unsigned long)lost, lossP);
    display.setCursor(0, 50); display.print(line);

    display.display();
}

static void renderRx(uint32_t received) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("RX MODE");

    display.drawFastHLine(0, 20, 128, SSD1306_WHITE);

    char line[22];
    snprintf(line, sizeof(line), "Rcvd:   %lu", (unsigned long)received);
    display.setCursor(0, 26); display.print(line);

    snprintf(line, sizeof(line), "Bad:    %lu", (unsigned long)corrupt);
    display.setCursor(0, 38); display.print(line);

    float ber = received > 0 ? 100.0f * corrupt / received : 0.0f;
    snprintf(line, sizeof(line), "BER:    %.1f%%", ber);
    display.setCursor(0, 50); display.print(line);

    display.display();
}

// ── TX ───────────────────────────────────────────────────────────────────────

static void processTx(transceiver& xcvr, uint32_t now) {
    // timeout outstanding echo → lost
    if (awaitingEcho && now - pendingMs > RF_TEST_DELAY_MS + 400) {
        lost++;
        awaitingEcho = false;
    }

    // send next ping
    if (!awaitingEcho && now - lastSendMs >= RF_TEST_DELAY_MS) {
        TestPacket pkt{};
        pkt.seq = nextSeq;
        memset(pkt.payload, TEST_PATTERN, sizeof(pkt.payload));

        xcvr.setMode(TRANSMIT);
        bool ok = xcvr.write(&pkt, sizeof(TestPacket));
        xcvr.setMode(RECEIVE);

        if (ok) {
            pendingSeq   = nextSeq++;
            pendingMs    = now;
            awaitingEcho = true;
            sent++;
        }
        lastSendMs = now;
    }

    // read echo
    TestPacket pong{};
    for (uint8_t i = 0; i < MAX_READS_PER_LOOP; i++) {
        if (!xcvr.read(&pong, sizeof(TestPacket))) break;

        echoed++;
        for (uint8_t b = 0; b < sizeof(pong.payload); b++)
            if (pong.payload[b] != TEST_PATTERN) { corrupt++; break; }

        if (awaitingEcho && pong.seq == pendingSeq)
            awaitingEcho = false;
    }

    renderTx();
}

// ── RX ───────────────────────────────────────────────────────────────────────

static uint32_t rxReceived = 0;

static void processRx(transceiver& xcvr) {
    TestPacket pkt{};
    for (uint8_t i = 0; i < MAX_READS_PER_LOOP; i++) {
        if (!xcvr.read(&pkt, sizeof(TestPacket))) break;

        rxReceived++;
        for (uint8_t b = 0; b < sizeof(pkt.payload); b++)
            if (pkt.payload[b] != TEST_PATTERN) { corrupt++; break; }

        // echo back
        xcvr.setMode(TRANSMIT);
        xcvr.write(&pkt, sizeof(TestPacket));
        xcvr.setMode(RECEIVE);
    }

    renderRx(rxReceived);
}

// ── public API ────────────────────────────────────────────────────────────────

void rfTestSetup(transceiver& xcvr) {
    sent = echoed = lost = corrupt = rxReceived = 0;
    nextSeq = 0; awaitingEcho = false;
    lastSendMs = pendingMs = millis();
    xcvr.setMode(RECEIVE);
}

void rfTestLoop(transceiver& xcvr) {
    uint32_t now = millis();
#if RF_TEST_TX
    processTx(xcvr, now);
#else
    processRx(xcvr);
#endif
}