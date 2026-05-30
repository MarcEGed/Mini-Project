#include "testMode.h"

#include <config.h>

#include "TestMessage.h"

#define MAX_READS_PER_LOOP 6


static RFTestStats stats{};
static bool txMode = false;

static uint32_t nextSeq = 0;
static uint32_t pendingSeq = 0;
static bool awaitingEcho = false;
static uint32_t pendingMs = 0;
static uint32_t lastSendMs = 0;

// ── TX ───────────────────────────────────────────────────────────────────────

static bool processTx(transceiver& xcvr, uint32_t now) {
    bool changed = false;

    // timeout outstanding echo → lost
    if (awaitingEcho && now - pendingMs > RF_TEST_DELAY_MS + 400) {
        stats.lost++;
        awaitingEcho = false;
        changed = true;
    }

    // send next ping
    if (!awaitingEcho && now - lastSendMs >= RF_TEST_DELAY_MS) {
        TestPacket pkt{};
        pkt.seq = nextSeq;
        memset(pkt.payload, TEST_PATTERN, sizeof(pkt.payload));

        bool ok = xcvr.write(PacketType::TEST, pkt, 0xFF); // broadcast ping

        if (ok) {
            pendingSeq = nextSeq++;
            pendingMs = now;
            awaitingEcho = true;
            stats.sent++;
            changed = true;
        }
        lastSendMs = now;
    }

    // read echo
    TestPacket pong;
    for (uint8_t i = 0; i < MAX_READS_PER_LOOP; i++) {
        if (!xcvr.read(PacketType::TEST, pong)) break;

        stats.echoed++;
        changed = true;
        for (uint8_t b = 0; b < sizeof(pong.payload); b++)
            if (pong.payload[b] != TEST_PATTERN) {
                stats.corrupt++;
                break;
            }

        if (awaitingEcho && pong.seq == pendingSeq) awaitingEcho = false;
    }

    return changed;
}

// ── RX ───────────────────────────────────────────────────────────────────────

static bool processRx(transceiver& xcvr) {
    bool changed = false;

    TestPacket pkt;
    uint8_t sender_id;
    for (uint8_t i = 0; i < MAX_READS_PER_LOOP; i++) {
        if (!xcvr.read(PacketType::TEST, pkt, &sender_id)) break;

        stats.received++;
        changed = true;
        for (uint8_t b = 0; b < sizeof(pkt.payload); b++)
            if (pkt.payload[b] != TEST_PATTERN) {
                stats.corrupt++;
                break;
            }

        // echo back
        xcvr.write(PacketType::TEST, pkt, sender_id);
    }

    return changed;
}

// ── public API
// ────────────────────────────────────────────────────────────────

void rfTestSetup(transceiver& xcvr) {
    stats = RFTestStats{};
    nextSeq = 0;
    pendingSeq = 0;
    awaitingEcho = false;
    lastSendMs = pendingMs = millis();
    xcvr.setMode(RECEIVE);
}

void rfTestLoop(transceiver& xcvr) { (void)rfTestTick(xcvr, millis()); }

bool rfTestTick(transceiver& xcvr, uint32_t now) {
    if (txMode) {
        return processTx(xcvr, now);
    }

    (void)now;
    return processRx(xcvr);
}

void rfTestSetTxMode(bool newTxMode) { txMode = newTxMode; }

const RFTestStats& rfTestGetStats() { return stats; }

bool rfTestIsTxMode() { return txMode; }