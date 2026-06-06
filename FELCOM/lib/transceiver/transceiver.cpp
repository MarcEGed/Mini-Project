#include "transceiver.h"

#include <config.h>
#include <debug.h>
#include <string.h>

void transceiver::setup() {
    radio = new RF24(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);
    radio->begin();
    radio->setPALevel(NRF24L01_POWER_LEVEL);
    radio->setDataRate(NRF24L01_DATA_RATE);
    radio->setChannel(HOPPING_CHANNELS[0]);
    radio->setPayloadSize(sizeof(FHSSPacket));
    // radio->enableDynamicPayloads();

    radio->setAutoAck(false);
    radio->setRetries(0, 0);
    radio->disableCRC();  // CRC is handled by the FEC layer, not the radio.

    radio->openWritingPipe(PHY_ADDRESSES[NRF24L01_PHY_ADDR]);
    radio->openReadingPipe(1, PHY_ADDRESSES[NRF24L01_PHY_ADDR]);

    fecInit(fec);
    rx_has = false;
    stats_printed_sum = 0;

    setMode(RECEIVE);
}

void transceiver::setMode(transceiverMode newMode) {
    mode = newMode;
    if (newMode == TRANSMIT) {
        radio->stopListening();
    } else {
        radio->startListening();
    }
}

// ── Low-level radio helpers (Rule 3) ───────────────────────────────────────

bool transceiver::transmitPacket(FHSSPacket& pkt) {
    transceiverMode oldMode = mode;
    if (mode != TRANSMIT) {
        setMode(TRANSMIT);
    }

    bool ok = false;
    uint8_t retries = 5;
    while (retries-- > 0) {
        // CSMA: briefly listen to check the channel is clear.
        radio->startListening();

        //===========================================================
        //=IM NOT SURE WHY, BUT THE FOLLOWING LINE DESTROYS BER MENU=
        //===========without it everything works fine - marc=========
        //===========================================================
        // delayMicroseconds(200);  // Allow RPD to lock onto a signal

        if (!radio->testRPD()) {
            radio->stopListening();
            ok = radio->write(&pkt, sizeof(FHSSPacket));
            break;
        }

        // Channel busy: random backoff (1-10 ms) then retry.
        delay(random(1, 10));
    }

    if (oldMode != TRANSMIT) setMode(oldMode);
    return ok;
}

bool transceiver::receiveRaw(FHSSPacket& pkt, uint8_t* out_src_node_id) {
    transceiverMode oldMode = mode;
    if (mode != RECEIVE) {
        setMode(RECEIVE);
    }

    bool got = false;
    while (radio->available()) {
        radio->read(&pkt, sizeof(FHSSPacket));
        if (pkt.dst_node_id == NODE_ID || pkt.dst_node_id == 0xFF) {
            if (out_src_node_id) *out_src_node_id = pkt.src_node_id;
            got = true;
            break;
        }
        // Not for us: discard and keep draining the FIFO.
    }

    if (oldMode != RECEIVE) setMode(oldMode);
    return got;
}

// ── Send paths ──────────────────────────────────────────────────────────────

bool transceiver::writeRaw(PacketType type, const void* data, uint8_t len,
                           uint8_t dst_node_id) {
    FHSSPacket pkt = {0};
    pkt.src_node_id = NODE_ID;
    pkt.dst_node_id = dst_node_id;
    pkt.packet_type = (uint16_t)type & 0x0F;
    pkt.fec = fecPackMeta((uint8_t)FecScheme::NONE, 0, 0);

    uint8_t n = len > sizeof(pkt.data) ? sizeof(pkt.data) : len;
    memcpy(pkt.data, data, n);

    return transmitPacket(pkt);
}

bool transceiver::write(PacketType type, const void* data, uint8_t len,
                        uint8_t dst_node_id) {
    // TEST stays raw so the BER menu measures real bit errors (Rule 4).
    if (type == PacketType::TEST) {
        return writeRaw(type, data, len, dst_node_id);
    }

    FHSSPacket pkt = {0};
    pkt.src_node_id = NODE_ID;
    pkt.dst_node_id = dst_node_id;
    fecEncodeProtected(pkt, type, FecScheme::CRC, 0, 0, data, len);
    return transmitPacket(pkt);
}

bool transceiver::writeReliable(PacketType type, const void* data, uint8_t len,
                                uint8_t dst_node_id) {
    uint8_t app_len = len > FEC_ARQ_PAYLOAD_SIZE ? FEC_ARQ_PAYLOAD_SIZE : len;
    uint16_t seq = fec.tx_seq++;

    // Wire layout: [seq(2) | app payload | pad] then CRC appended by encode.
    uint8_t wire[FEC_USER_SIZE] = {0};
    memcpy(wire, &seq, FEC_ARQ_SEQ_SIZE);
    memcpy(wire + FEC_ARQ_SEQ_SIZE, data, app_len);

    FHSSPacket pkt = {0};
    pkt.src_node_id = NODE_ID;
    pkt.dst_node_id = dst_node_id;
    fecEncodeProtected(pkt, type, FecScheme::CRC_ARQ, 0, 0, wire, FEC_USER_SIZE);

    fec.stats.arq_sent++;
    for (uint8_t attempt = 0; attempt <= FEC_ARQ_MAX_RETRIES; attempt++) {
        if (attempt > 0) fec.stats.arq_retx++;
        fec.arq_seq = seq;
        fec.arq_acked = false;
        fec.arq_waiting = true;

        transmitPacket(pkt);

        uint32_t start = millis();
        while ((uint32_t)(millis() - start) < FEC_ARQ_TIMEOUT_MS) {
            serviceRx();  // drains ACK/audio; sets arq_acked on a match
            if (fec.arq_acked) {
                fec.arq_waiting = false;
                fec.stats.arq_acked++;
                LOG_INFO("FEC ARQ: type=%u seq=%u acked (tries=%u)",
                         (unsigned)type, (unsigned)seq, (unsigned)(attempt + 1));
                return true;
            }
        }
    }

    fec.arq_waiting = false;
    fec.stats.arq_failed++;
    LOG_INFO("FEC ARQ: type=%u seq=%u FAILED (no ack after %u tries)",
             (unsigned)type, (unsigned)seq, (unsigned)(FEC_ARQ_MAX_RETRIES + 1));
    return false;  // delivery not confirmed
}

// ── ACK handling ──────────────────────────────────────────────────────────

void transceiver::sendAck(uint8_t dst_node_id, uint16_t seq) {
    AckPayload ack = {0};
    ack.seq = seq;
    ack.status = 0;  // OK

    FHSSPacket pkt = {0};
    pkt.src_node_id = NODE_ID;
    pkt.dst_node_id = dst_node_id;
    fecEncodeProtected(pkt, PacketType::ACK, FecScheme::CRC, 0, 0, &ack,
                       sizeof(ack));
    transmitPacket(pkt);
    fec.stats.ack_tx++;
}

void transceiver::handleAck(const FHSSPacket& pkt) {
    AckPayload ack;
    memcpy(&ack, pkt.data, sizeof(ack));
    if (fec.arq_waiting && ack.status == 0 && ack.seq == fec.arq_seq) {
        fec.arq_acked = true;
        fec.stats.ack_rx++;
    }
}

// ── Receive path ──────────────────────────────────────────────────────────

bool transceiver::serviceRx() {
    FHSSPacket pkt;
    uint8_t src;
    if (!receiveRaw(pkt, &src)) return false;

    PacketType type = (PacketType)pkt.packet_type;

    // Unprotected frames (scheme NONE): TEST (raw BER, Rule 4) and ND_SYNC
    // timing broadcasts. Delivered verbatim with no CRC, exactly like the
    // pre-FEC transceiver — a slightly corrupted sync packet must NOT be
    // dropped, or syncing becomes unreliable.
    if (fecGetScheme(pkt.fec) == (uint8_t)FecScheme::NONE) {
        if (!rx_has) {
            rx_pkt = pkt;
            rx_src = src;
            rx_has = true;
        }
        return true;
    }

    // All other types are CRC-protected. A failed CRC means corruption → drop.
    // Counted, never logged per-packet: a serial flush blocks ~50 ms at 9600
    // baud, which under noise would wreck radio timing. See logFecStats().
    if (!fecCheckCrc(pkt)) {
        fec.stats.crc_fail++;
        return true;
    }
    fec.stats.crc_ok++;

    if (type == PacketType::ACK) {
        handleAck(pkt);  // never buffered, never re-ACKed
        return true;
    }

    if (type == PacketType::AUDIO) {
        fecAudioAssemble(fec, pkt);  // XOR block forward correction
        return true;
    }

    // CHAT / PONG / ND_SYNC: a protected application frame.
    if (!rx_has) {
        rx_pkt = pkt;
        rx_src = src;
        rx_has = true;
        // Auto-ACK reliable types only after we have safely buffered them, so a
        // dropped frame is retransmitted rather than falsely acknowledged.
        if (fecGetScheme(pkt.fec) == (uint8_t)FecScheme::CRC_ARQ) {
            uint16_t seq;
            memcpy(&seq, pkt.data, FEC_ARQ_SEQ_SIZE);
            sendAck(src, seq);
        }
    }
    // else: pushback full — drop (ARQ senders will retransmit).
    return true;
}

bool transceiver::read(PacketType expected_type, void* data, uint8_t len,
                       uint8_t* out_src_node_id) {
    for (uint8_t i = 0; i < 8 && !rx_has; i++) {
        if (!serviceRx()) break;
    }
    if (!rx_has) return false;

    PacketType type = (PacketType)rx_pkt.packet_type;
    if (type != expected_type) {
        // Buffered frame is for a different consumer; drop it (single FIFO).
        rx_has = false;
        return false;
    }

    if (out_src_node_id) *out_src_node_id = rx_src;

    if (fecGetScheme(rx_pkt.fec) == (uint8_t)FecScheme::NONE) {
        // Raw, unprotected (TEST, ND_SYNC): copy the whole data region verbatim.
        uint8_t n = len > sizeof(rx_pkt.data) ? sizeof(rx_pkt.data) : len;
        memcpy(data, rx_pkt.data, n);
    } else if (fecGetScheme(rx_pkt.fec) == (uint8_t)FecScheme::CRC_ARQ) {
        // Strip the 2-byte ARQ seq; hand the app its payload.
        uint8_t n = len > FEC_ARQ_PAYLOAD_SIZE ? FEC_ARQ_PAYLOAD_SIZE : len;
        memcpy(data, rx_pkt.data + FEC_ARQ_SEQ_SIZE, n);
    } else {
        uint8_t n = len > FEC_USER_SIZE ? FEC_USER_SIZE : len;
        memcpy(data, rx_pkt.data, n);
    }

    rx_has = false;
    return true;
}

void transceiver::poll() {
    // Drain the FIFO: ACK/audio are handled, at most one app frame is buffered.
    for (uint8_t i = 0; i < 6; i++) {
        if (!serviceRx()) break;
    }
}

// ── Audio (XOR block FEC) ───────────────────────────────────────────────────

void transceiver::audioTx(const void* payload, uint8_t len) {
    uint8_t idx = fec.xor_tx_count;

    FHSSPacket pkt = {0};
    pkt.src_node_id = NODE_ID;
    pkt.dst_node_id = 0xFF;  // audio is broadcast
    fecEncodeProtected(pkt, PacketType::AUDIO, FecScheme::XOR,
                       fec.xor_tx_block_id, idx, payload, len);
    transmitPacket(pkt);

    // Accumulate parity over the protected user region of each data slot.
    for (uint8_t b = 0; b < FEC_USER_SIZE; b++) {
        fec.xor_tx_parity[b] ^= pkt.data[b];
    }
    fec.xor_tx_count++;

    if (fec.xor_tx_count >= FEC_XOR_BLOCK_SIZE) {
        FHSSPacket parity = {0};
        parity.src_node_id = NODE_ID;
        parity.dst_node_id = 0xFF;
        fecEncodeProtected(parity, PacketType::AUDIO, FecScheme::XOR,
                           fec.xor_tx_block_id, FEC_INDEX_PARITY,
                           fec.xor_tx_parity, FEC_USER_SIZE);
        transmitPacket(parity);

        memset(fec.xor_tx_parity, 0, FEC_USER_SIZE);
        fec.xor_tx_count = 0;
        fec.xor_tx_block_id = (fec.xor_tx_block_id + 1) & 0x0F;
    }
}

bool transceiver::audioRx(void* payload, uint8_t len) {
    // Service the radio so freshly arrived blocks are assembled before popping.
    poll();
    return fecAudioDequeue(fec, payload, len);
}

// ── Misc ────────────────────────────────────────────────────────────────────

void transceiver::sendSync(uint32_t value) {
    NDSyncData payload = {0};
    payload.timer_val = (int64_t)value;
    // ND_SYNC is sent RAW (no CRC, no ARQ), byte-for-byte as the pre-FEC
    // transceiver did. It is a best-effort timing broadcast: ARQ would block
    // hopping for ~800 ms, and CRC-dropping a slightly corrupted sync packet
    // makes syncing unreliable (sync is how nodes RECOVER from desync, so it
    // must be as forgiving as possible).
    writeRaw(PacketType::ND_SYNC, &payload, sizeof(payload));
}

void transceiver::hop() {
    if (mode != TRANSMIT) {
        radio->stopListening();
    }
    radio->setChannel(HOPPING_CHANNELS[0]);
    if (mode != TRANSMIT) {
        radio->startListening();
    }
}

void transceiver::logFecStats() {
    const FecStats& s = fec.stats;
    uint32_t sum = s.crc_ok + s.crc_fail + s.arq_sent + s.arq_retx +
                   s.arq_acked + s.arq_failed + s.ack_rx + s.ack_tx +
                   s.aud_blocks + s.aud_recovered + s.aud_lost;
    if (sum == stats_printed_sum) return;  // nothing new since the last print
    stats_printed_sum = sum;

    LOG_INFO(
        "FEC | CRC ok=%lu bad=%lu | ARQ tx=%lu retx=%lu ack=%lu fail=%lu | "
        "ACK rx=%lu tx=%lu | AUDIO blk=%lu rec=%lu lost=%lu",
        (unsigned long)s.crc_ok, (unsigned long)s.crc_fail,
        (unsigned long)s.arq_sent, (unsigned long)s.arq_retx,
        (unsigned long)s.arq_acked, (unsigned long)s.arq_failed,
        (unsigned long)s.ack_rx, (unsigned long)s.ack_tx,
        (unsigned long)s.aud_blocks, (unsigned long)s.aud_recovered,
        (unsigned long)s.aud_lost);
}

void transceiver::setChannel(uint8_t channel) {
    bool wasListening = (mode == RECEIVE);
    if (wasListening) {
        radio->stopListening();
    }
    radio->setChannel(channel);
    if (wasListening) {
        radio->startListening();
    }
}
