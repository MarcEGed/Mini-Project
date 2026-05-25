#include "transceiver.h"

#include <config.h>
#include <debug.h>

void transceiver::setup() {
    radio = new RF24(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);
    radio->begin();
    radio->setPALevel(NRF24L01_POWER_LEVEL);
    radio->setDataRate(NRF24L01_DATA_RATE);
    radio->setChannel(110);
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

    uint8_t payload_len = len > sizeof(pkt.data) ? sizeof(pkt.data) : len;
    memcpy(pkt.data, data, payload_len);

    uint8_t retries = 5;
    while (retries-- > 0) {
        radio->startListening();
        delayMicroseconds(200);

        if (!radio->testRPD()) {
            radio->stopListening();
            bool ok = radio->write(&pkt, sizeof(FHSSPacket));

            if (oldMode != TRANSMIT) setMode(oldMode);
            return ok;
        }

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

    (void)join_state; // join state not used here in current implementation

    if (pkt.dst_node_id != NODE_ID && pkt.dst_node_id != 0xFF) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;
    }

    if (pkt.packet_type != static_cast<uint16_t>(expected_type)) {
        if (pkt.packet_type == static_cast<uint16_t>(PacketType::ND_SYNC)) {
            handleBackgroundSync(&pkt);
        }
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;
    }

    if (out_src_node_id) {
        *out_src_node_id = pkt.src_node_id;
    }

    uint8_t payload_len = len > sizeof(pkt.data) ? sizeof(pkt.data) : len;
    memcpy(data, pkt.data, payload_len);

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

// Passive join removed per spec: only active scan on channel 110 is supported.

void transceiver::startActiveScan(uint32_t now_ms) {
    if (joined) return;

    join_state = JOIN_ACTIVE_SCAN;
    join_state_since_ms = now_ms;
    join_timeout_ms = now_ms + FHSS_BOOTSTRAP_TIMEOUT_MS;
    join_last_tx_ms = (now_ms >= FHSS_BOOTSTRAP_RETRY_MS)
                          ? now_ms - FHSS_BOOTSTRAP_RETRY_MS
                          : 0;

    radio->setChannel(110);
    setMode(RECEIVE);
}

void transceiver::updateJoin(uint32_t now_ms) {
    if (joined) return;
    if (!fhss_timer) return;

    if (join_state == JOIN_IDLE) startActiveScan(now_ms);

    if (join_state == JOIN_ACTIVE_SCAN) {
        readSyncPacket();

        if (joined) return;

        if (now_ms >= join_timeout_ms) {
            timerAlarmWrite(fhss_timer, FHSS_TIMER_PERIOD_US, true);
            uint64_t preload_ticks = timerRead(fhss_timer);
            timerWrite(fhss_timer, preload_ticks);
            joined = true;
            join_state = JOIN_IDLE;
            
            NDSyncData created;
            created.timer_val = preload_ticks;
            writeRaw(PacketType::ND_SYNC, &created, sizeof(created), 0xFF);
            LOG_INFO("No synchronized reply. Formed new network.");
            timerAlarmEnable(fhss_timer);
            return;
        }

        if (now_ms - join_last_tx_ms >= FHSS_BOOTSTRAP_RETRY_MS) {
            NDSyncData req;
            req.timer_val = -1;
            writeRaw(PacketType::ND_SYNC, &req, sizeof(req), 0xFF);
            join_last_tx_ms = now_ms;
        }
        return;
    }

    // Passive listen / active-wait removed: spec requires active scan only.
}

void transceiver::handleBackgroundSync(FHSSPacket* pkt) {
    if (!fhss_timer || !pkt) return;

    NDSyncData* syncData = (NDSyncData*)pkt->data;
    if (syncData->timer_val == -1) {
        NDSyncData reply;
        if (!joined) {
            reply.timer_val = -1;
            writeRaw(PacketType::ND_SYNC, &reply, sizeof(reply),
                     pkt->src_node_id);
            LOG_INFO("Bootstrap sync request received from unsynchronized node.");
            return;
        }

        reply.timer_val = timerRead(fhss_timer);
        writeRaw(PacketType::ND_SYNC, &reply, sizeof(reply), pkt->src_node_id);
        LOG_INFO("Sent sync reply: %lld", reply.timer_val);
        return;
    }

    timerWrite(fhss_timer, syncData->timer_val);
    timerAlarmEnable(fhss_timer);
    joined = true;
    join_state = JOIN_IDLE;
    LOG_INFO("Joined network. Timer set to %lld", syncData->timer_val);
}
