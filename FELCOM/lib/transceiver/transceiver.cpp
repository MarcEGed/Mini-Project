#include "transceiver.h"

#include <config.h>
#include <debug.h>

namespace {
constexpr uint32_t kJoinListenMs = 6;
constexpr uint32_t kJoinSendGapMs = 2;
constexpr uint32_t kJoinJitterMs = 4;
constexpr uint32_t kJoinScanTimeoutMs = 1500;
}  // namespace

void transceiver::setup() {
    radio = new RF24(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);
    radio->begin();
    radio->setPALevel(NRF24L01_POWER_LEVEL);
    radio->setDataRate(NRF24L01_DATA_RATE);
    radio->setChannel(HOPPING_CHANNELS[channel_idx]);
    radio->setPayloadSize(sizeof(FHSSPacket));
    // radio->enableDynamicPayloads();

    radio->setAutoAck(false);
    radio->setRetries(0, 0);
    radio->disableCRC();

    radio->openWritingPipe(PHY_ADDRESSES[NRF24L01_PHY_ADDR]);
    radio->openReadingPipe(1, PHY_ADDRESSES[NRF24L01_PHY_ADDR]);

    setMode(RECEIVE);

    randomSeed(micros());
}

void transceiver::setMode(transceiverMode newMode) {
    mode = newMode;
    if (newMode == TRANSMIT) {
        radio->stopListening();
    } else {
        radio->startListening();
    }
}

bool transceiver::writeRaw(PacketType type, const void* data, uint8_t len,
                           uint8_t dst_node_id) {
    transceiverMode oldMode = mode;
    if (mode != TRANSMIT) {
        setMode(TRANSMIT);
    }

    FHSSPacket pkt = {0};
    pkt.src_node_id = NODE_ID;
    pkt.dst_node_id = dst_node_id;
    pkt.packet_type = static_cast<uint16_t>(type);
    pkt.fec = 0;

    // Copy payload (truncate to max data section size if needed)
    uint8_t payload_len = len > sizeof(pkt.data) ? sizeof(pkt.data) : len;
    memcpy(pkt.data, data, payload_len);

    uint8_t retries = 5;
    while (retries-- > 0) {
        // CSMA: Briefly switch to RECEIVE mode to check channel activity
        radio->startListening();
        delayMicroseconds(200);  // Allow RPD to lock onto a signal

        if (!radio->testRPD()) {
            // Channel is clear, switch back to TRANSMIT and send
            radio->stopListening();
            bool ok = radio->write(&pkt, sizeof(FHSSPacket));

            if (oldMode != TRANSMIT) setMode(oldMode);
            return ok;
        }

        // Channel busy, apply random backoff (1-10 ms)
        delay(random(1, 10));
    }

    if (oldMode != TRANSMIT) setMode(oldMode);
    return false;
}

bool transceiver::write(PacketType type, const void* data, uint8_t len,
                        uint8_t dst_node_id) {
    if (!joined && fhss_timer) {
        if (join_state == JOIN_IDLE) startActiveScan(millis());
        return false;
    }

    return writeRaw(type, data, len, dst_node_id);
}

bool transceiver::read(PacketType expected_type, void* data, uint8_t len,
                       uint8_t* out_src_node_id) {
    transceiverMode oldMode = mode;
    if (mode != RECEIVE) {
        setMode(RECEIVE);
    }

    if (!radio->available()) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;
    }

    FHSSPacket pkt;
    radio->read(&pkt, sizeof(FHSSPacket));

    if (!joined && join_state == JOIN_PASSIVE_LISTEN) {
        join_heard_packet = true;
    }

    if (pkt.dst_node_id != NODE_ID && pkt.dst_node_id != 0xFF) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;  // Packet not meant for this node
    }

    if (pkt.packet_type != static_cast<uint16_t>(expected_type)) {
        // Intercept ND_SYNC requests
        if (pkt.packet_type == static_cast<uint16_t>(PacketType::ND_SYNC)) {
            handleBackgroundSync(&pkt);
        }
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;  // Packet type mismatch
    }

    if (out_src_node_id) {
        *out_src_node_id = pkt.src_node_id;
    }

    // Pass payload
    uint8_t payload_len = len > sizeof(pkt.data) ? sizeof(pkt.data) : len;
    memcpy(data, pkt.data, payload_len);

    // hop();
    LOG_INFO("Packet received");
    if (oldMode != RECEIVE) setMode(oldMode);
    return true;
}

void transceiver::hop() {
    if (mode != TRANSMIT) {
        radio->stopListening();
    }
    radio->setChannel(HOPPING_CHANNELS[channel_idx]);
    if (mode != TRANSMIT) {
        radio->startListening();
    }
    channel_idx = (channel_idx + 1) % HOPPING_CHANNELS_SIZE;
}

bool transceiver::readSyncPacket() {
    transceiverMode oldMode = mode;
    if (mode != RECEIVE) setMode(RECEIVE);

    if (!radio->available()) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;
    }

    FHSSPacket pkt;
    radio->read(&pkt, sizeof(FHSSPacket));

    if (pkt.dst_node_id != NODE_ID && pkt.dst_node_id != 0xFF) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;
    }

    if (pkt.packet_type != static_cast<uint16_t>(PacketType::ND_SYNC)) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;
    }

    handleBackgroundSync(&pkt);

    if (oldMode != RECEIVE) setMode(oldMode);
    return true;
}

void transceiver::startPassiveJoin(uint32_t now_ms) {
    if (joined) return;

    join_state = JOIN_PASSIVE_LISTEN;
    join_state_since_ms = now_ms;
    join_heard_packet = false;

    radio->setChannel(HOPPING_CHANNELS[random(0, HOPPING_CHANNELS_SIZE)]);
    setMode(RECEIVE);
}

void transceiver::startActiveScan(uint32_t now_ms) {
    if (joined) return;

    join_state = JOIN_ACTIVE_SCAN;
    join_state_since_ms = now_ms;
    join_scan_channel_idx = random(0, HOPPING_CHANNELS_SIZE);
    join_scan_listen_phase = true;
    join_scan_next_ms = now_ms + random(0, kJoinJitterMs + 1);

    radio->setChannel(HOPPING_CHANNELS[join_scan_channel_idx]);
    setMode(RECEIVE);
}

void transceiver::updateJoin(uint32_t now_ms) {
    if (joined) return;
    if (!fhss_timer) return;

    if (join_state == JOIN_IDLE) startActiveScan(now_ms);

    if (join_state == JOIN_ACTIVE_SCAN) {
        readSyncPacket();

        if (now_ms >= join_scan_next_ms) {
            if (join_scan_listen_phase) {
                NDSyncData req;
                req.timer_val = -1;
                writeRaw(PacketType::ND_SYNC, &req, sizeof(req), 0xFF);
                join_scan_listen_phase = false;
                join_scan_next_ms = now_ms + kJoinSendGapMs;
            } else {
                join_scan_channel_idx =
                    (join_scan_channel_idx + 1) % HOPPING_CHANNELS_SIZE;
                radio->setChannel(HOPPING_CHANNELS[join_scan_channel_idx]);
                setMode(RECEIVE);
                join_scan_listen_phase = true;
                join_scan_next_ms = now_ms + kJoinListenMs +
                                    random(0, kJoinJitterMs + 1);
            }
        }

        if (now_ms - join_state_since_ms > kJoinScanTimeoutMs) {
            timerWrite(fhss_timer, 0);
            timerAlarmEnable(fhss_timer);
            joined = true;
            join_state = JOIN_IDLE;
            LOG_INFO("No sync replies. Formed new network.");
        }
        return;
    }

    if (join_state == JOIN_PASSIVE_LISTEN && join_heard_packet) {
        NDSyncData req;
        req.timer_val = -1;
        writeRaw(PacketType::ND_SYNC, &req, sizeof(req), 0xFF);
        join_state = JOIN_ACTIVE_WAIT;
        join_state_since_ms = now_ms;
        join_heard_packet = false;
        return;
    }

    if (join_state == JOIN_ACTIVE_WAIT) {
        if (now_ms - join_state_since_ms > 50) {
            startPassiveJoin(now_ms);
        }
    }
}

void transceiver::handleBackgroundSync(FHSSPacket* pkt) {
    if (!fhss_timer || !pkt) return;

    NDSyncData* syncData = (NDSyncData*)pkt->data;
    if (syncData->timer_val == -1) {
        if (!joined) {
            timerWrite(fhss_timer, 0);
            timerAlarmEnable(fhss_timer);
            joined = true;
            join_state = JOIN_IDLE;
        }

        // This is a join request; reply with our exact timer value
        NDSyncData reply;
        reply.timer_val = timerRead(fhss_timer);
        writeRaw(PacketType::ND_SYNC, &reply, sizeof(reply),
                 pkt->src_node_id);
        LOG_INFO("Sent sync reply: %lld", reply.timer_val);
    } else {
        timerWrite(fhss_timer, syncData->timer_val);
        timerAlarmEnable(fhss_timer);
        joined = true;
        join_state = JOIN_IDLE;
        LOG_INFO("Joined network. Timer set to %lld", syncData->timer_val);
    }
}
