#include "testMode.h"

// packet types used by test mode
#define PKT_TYPE_SYN 0x01
#define PKT_TYPE_ACK 0x02
#define PKT_TYPE_DATA 0x03

// retransmission and liveness tunables for non-blocking state machine
#define HANDSHAKE_RETRY_MS 350
#define ECHO_TIMEOUT_MS (RF_TEST_DELAY_MS + 400)
#define LINK_TIMEOUT_MS (RF_TEST_DELAY_MS * 6)
#define MAX_READS_PER_LOOP 6

struct TestPacket {
    uint8_t type;
    uint8_t reserved[3];
    uint32_t seq;
    uint8_t payload[24];
};

static_assert(sizeof(TestPacket) == RF_TEST_PACKET_SIZE,
              "TestPacket size must match RF_TEST_PACKET_SIZE");

enum TestState { HANDSHAKE, ACTIVE };

static TestState state = HANDSHAKE;

static uint32_t sent = 0;
static uint32_t echoed = 0;
static uint32_t received = 0;
static uint32_t corrupted = 0;
static uint32_t lost = 0;
static uint32_t seqErrors = 0;
static uint32_t txWriteFail = 0;

static uint32_t handshakeAttempt = 0;
static uint32_t lastHandshakeTxMs = 0;
static uint32_t lastSendMs = 0;
static uint32_t lastRxMs = 0;

static uint32_t nextTxSeq = 0;
static uint32_t pendingSeq = 0;
static bool awaitingEcho = false;
static uint32_t pendingSinceMs = 0;

static uint32_t lastRxDataSeq = 0;
static bool hasLastRxDataSeq = false;

static void resetLinkState() {
    state = HANDSHAKE;
    handshakeAttempt = 0;
    lastHandshakeTxMs = 0;
    lastSendMs = 0;
    awaitingEcho = false;
    pendingSinceMs = 0;
#if !RF_TEST_TX
    hasLastRxDataSeq = false;
    lastRxDataSeq = 0;
#endif
}

static bool payloadLooksValid(const TestPacket& pkt, uint8_t* badBytesOut) {
    uint8_t badBytes = 0;
    for (uint8_t i = 0; i < sizeof(pkt.payload); i++) {
        if (pkt.payload[i] != TEST_PATTERN) {
            badBytes++;
        }
    }
    if (badBytesOut != nullptr) {
        *badBytesOut = badBytes;
    }
    return badBytes == 0;
}

static void fillDataPacket(TestPacket* pkt, uint32_t sequence) {
    memset(pkt, 0, sizeof(TestPacket));
    pkt->type = PKT_TYPE_DATA;
    pkt->seq = sequence;
    memset(pkt->payload, TEST_PATTERN, sizeof(pkt->payload));
}

static void sendPacket(transceiver& xcvr, const TestPacket& pkt,
                       bool countWriteFail) {
    xcvr.setMode(TRANSMIT);
    bool ok = xcvr.write(&pkt, sizeof(TestPacket));
    xcvr.setMode(RECEIVE);

    if (!ok && countWriteFail) {
        txWriteFail++;
    }
}

static void renderHeader() {
    display.setCursor(0, 0);
#if RF_TEST_TX
    display.print("RF TEST  [TX]");
#else
    display.print("RF TEST  [RX]");
#endif
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
}

static void renderHandshake(const char* status) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    renderHeader();

    display.setCursor(0, 16);
    display.print("Handshake...");

    display.setCursor(0, 28);
    display.print(status);

    char line[22];
    snprintf(line, sizeof(line), "Attempt: %lu",
             (unsigned long)handshakeAttempt);
    display.setCursor(0, 40);
    display.print(line);

    display.display();
}

static void renderStats() {
    float ber =
        received > 0 ? (100.0f * (float)corrupted / (float)received) : 0.0f;
    float lossPct = sent > 0 ? (100.0f * (float)lost / (float)sent) : 0.0f;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    renderHeader();

    char line[22];
#if RF_TEST_TX
    snprintf(line, sizeof(line), "Sent:%lu Echo:%lu", (unsigned long)sent,
             (unsigned long)echoed);
    display.setCursor(0, 14);
    display.print(line);

    snprintf(line, sizeof(line), "Lost:%lu SeqE:%lu", (unsigned long)lost,
             (unsigned long)seqErrors);
    display.setCursor(0, 26);
    display.print(line);

    snprintf(line, sizeof(line), "Bad:%lu BER:%.1f%%", (unsigned long)corrupted,
             ber);
    display.setCursor(0, 38);
    display.print(line);

    snprintf(line, sizeof(line), "WFail:%lu L:%.1f%%",
             (unsigned long)txWriteFail, lossPct);
    display.setCursor(0, 50);
    display.print(line);
#else
    snprintf(line, sizeof(line), "Rcv:%lu Bad:%lu", (unsigned long)received,
             (unsigned long)corrupted);
    display.setCursor(0, 14);
    display.print(line);

    snprintf(line, sizeof(line), "Echo:%lu SeqE:%lu", (unsigned long)echoed,
             (unsigned long)seqErrors);
    display.setCursor(0, 26);
    display.print(line);

    snprintf(line, sizeof(line), "BER:%.1f%%", ber);
    display.setCursor(0, 38);
    display.print(line);
#endif

    display.display();
}

static void processHandshake(transceiver& xcvr, uint32_t now) {
#if RF_TEST_TX
    if (now - lastHandshakeTxMs >= HANDSHAKE_RETRY_MS) {
        TestPacket syn = {};
        syn.type = PKT_TYPE_SYN;

        handshakeAttempt++;
        lastHandshakeTxMs = now;
        sendPacket(xcvr, syn, false);

        LOG_INFO("Handshake: SYN sent attempt=%lu",
                 (unsigned long)handshakeAttempt);
    }

    TestPacket pkt = {};
    for (uint8_t i = 0; i < MAX_READS_PER_LOOP; i++) {
        if (!xcvr.read(&pkt, sizeof(TestPacket))) {
            break;
        }

        if (pkt.type == PKT_TYPE_ACK) {
            state = ACTIVE;
            lastRxMs = now;
            lastSendMs = now;
            awaitingEcho = false;
            LOG_INFO("Handshake complete: ACK received");
            return;
        }
    }

    renderHandshake("Sending SYN...");

#else
    TestPacket pkt = {};
    for (uint8_t i = 0; i < MAX_READS_PER_LOOP; i++) {
        if (!xcvr.read(&pkt, sizeof(TestPacket))) {
            break;
        }

        if (pkt.type == PKT_TYPE_SYN) {
            TestPacket ack = {};
            ack.type = PKT_TYPE_ACK;
            sendPacket(xcvr, ack, false);

            handshakeAttempt++;
            state = ACTIVE;
            lastRxMs = now;
            LOG_INFO("Handshake complete: SYN received, ACK sent");
            return;
        }
    }

    handshakeAttempt++;
    renderHandshake("Waiting for SYN...");
#endif
}

static void processActiveTx(transceiver& xcvr, uint32_t now) {
    if (awaitingEcho && (now - pendingSinceMs > ECHO_TIMEOUT_MS)) {
        lost++;
        awaitingEcho = false;
        LOG_ERROR("Echo timeout seq=%lu", (unsigned long)pendingSeq);
    }

    if (!awaitingEcho && now - lastSendMs >= RF_TEST_DELAY_MS) {
        TestPacket ping = {};
        fillDataPacket(&ping, nextTxSeq);

        xcvr.setMode(TRANSMIT);
        bool ok = xcvr.write(&ping, sizeof(TestPacket));
        xcvr.setMode(RECEIVE);

        if (ok) {
            sent++;
            pendingSeq = nextTxSeq;
            pendingSinceMs = now;
            awaitingEcho = true;
            nextTxSeq++;
            LOG_INFO("Ping seq=%lu", (unsigned long)ping.seq);
        } else {
            txWriteFail++;
            LOG_ERROR("Ping write failed seq=%lu", (unsigned long)ping.seq);
        }
        lastSendMs = now;
    }

    TestPacket pong = {};
    for (uint8_t i = 0; i < MAX_READS_PER_LOOP; i++) {
        if (!xcvr.read(&pong, sizeof(TestPacket))) {
            break;
        }

        if (pong.type != PKT_TYPE_DATA) {
            continue;
        }

        lastRxMs = now;
        received++;

        uint8_t badBytes = 0;
        if (!payloadLooksValid(pong, &badBytes)) {
            corrupted++;
            LOG_ERROR("Corrupt pong seq=%lu bad=%u", (unsigned long)pong.seq,
                      (unsigned int)badBytes);
        }

        if (!awaitingEcho) {
            seqErrors++;
            LOG_ERROR("Unexpected pong seq=%lu (no outstanding ping)",
                      (unsigned long)pong.seq);
            continue;
        }

        if (pong.seq != pendingSeq) {
            seqErrors++;
            LOG_ERROR("Out-of-order pong seq=%lu expected=%lu",
                      (unsigned long)pong.seq, (unsigned long)pendingSeq);
            continue;
        }

        echoed++;
        awaitingEcho = false;
        LOG_INFO("Pong seq=%lu OK", (unsigned long)pong.seq);
    }
}

static void processActiveRx(transceiver& xcvr, uint32_t now) {
    TestPacket packet = {};
    for (uint8_t i = 0; i < MAX_READS_PER_LOOP; i++) {
        if (!xcvr.read(&packet, sizeof(TestPacket))) {
            break;
        }

        if (packet.type != PKT_TYPE_DATA) {
            continue;
        }

        lastRxMs = now;
        received++;

        uint8_t badBytes = 0;
        if (!payloadLooksValid(packet, &badBytes)) {
            corrupted++;
            LOG_ERROR("Corrupt ping seq=%lu bad=%u", (unsigned long)packet.seq,
                      (unsigned int)badBytes);
        }

        if (hasLastRxDataSeq && packet.seq <= lastRxDataSeq) {
            seqErrors++;
            LOG_ERROR("Duplicate/old ping seq=%lu last=%lu",
                      (unsigned long)packet.seq, (unsigned long)lastRxDataSeq);
        }
        lastRxDataSeq = packet.seq;
        hasLastRxDataSeq = true;

        sendPacket(xcvr, packet, true);
        echoed++;
    }
}

void rfTestSetup(transceiver& xcvr) {
    LOG_INFO("RF test mode start");
#if RF_TEST_TX
    LOG_INFO("Role: TX");
#else
    LOG_INFO("Role: RX");
#endif

    sent = 0;
    echoed = 0;
    received = 0;
    corrupted = 0;
    lost = 0;
    seqErrors = 0;
    txWriteFail = 0;
    nextTxSeq = 0;
    lastRxMs = millis();

    resetLinkState();
    xcvr.setMode(RECEIVE);
    renderHandshake("Starting...");
}

void rfTestLoop(transceiver& xcvr) {
    uint32_t now = millis();

    if (state == HANDSHAKE) {
        processHandshake(xcvr, now);
        return;
    }

#if RF_TEST_TX
    processActiveTx(xcvr, now);
#else
    processActiveRx(xcvr, now);
#endif

    if (now - lastRxMs > LINK_TIMEOUT_MS) {
        LOG_ERROR("Link timeout, restarting handshake");
        resetLinkState();
        renderHandshake("Link timeout...");
        return;
    }

    renderStats();
}