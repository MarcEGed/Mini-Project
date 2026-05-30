#include "transceiver.h"

#include <Arduino.h>
#include <config.h>
#include <debug.h>
#include <string.h>

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

void transceiver::noteSyncPeer(uint8_t node_id) {
    if (node_id == NODE_ID) return;
    if (node_id == 0xFF) return;
    if (synced_nodes[node_id]) return;

    synced_nodes[node_id] = true;
    if (synced_count < 255) {
        synced_count++;
    }
}

uint8_t transceiver::syncedPeerCount() const { return synced_count; }

bool transceiver::isSyncedPeer(uint8_t node_id) const {
    if (node_id == NODE_ID || node_id == 0xFF) return true;
    return synced_nodes[node_id];
}

void transceiver::markActivity() { last_activity_hop = hop_count; }

void transceiver::sendSyncTo(uint8_t node_id) {
    if (!joined || !fhss_timer) return;
    if (node_id == NODE_ID || node_id == 0xFF) return;
    if (synced_nodes[node_id]) return;

    NDSyncData reply;
    reply.timer_val = timerRead(fhss_timer);
    reply.channel_idx = channel_idx;
    bool ok = writeRaw(PacketType::ND_SYNC, &reply, sizeof(reply), node_id);
    if (ok) {
        noteSyncPeer(node_id);
    }
}

void transceiver::requestSyncFrom(uint8_t node_id) {
    if (node_id == NODE_ID || node_id == 0xFF) return;

    NDSyncData req;
    req.timer_val = -1;
    req.channel_idx = 0;
    writeRaw(PacketType::ND_SYNC, &req, sizeof(req), node_id);
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

    // CSMA with random backoff to avoid collisions
    bool channel_clear = false;
    for (uint8_t attempt = 0; attempt < FHSS_CSMA_ATTEMPTS; attempt++) {
        radio->startListening();
        delayMicroseconds(200);

        if (!radio->testRPD()) {
            channel_clear = true;
            break;
        }

        radio->stopListening();
        uint32_t backoff_us =
            random(FHSS_CSMA_BACKOFF_MIN_US, FHSS_CSMA_BACKOFF_MAX_US + 1);
        delayMicroseconds(backoff_us);
    }

    bool ok = false;
    if (channel_clear) {
        radio->stopListening();
        ok = radio->write(&pkt, sizeof(FHSSPacket));
    }

    if (ok) {
        markActivity();
    }

    if (oldMode != TRANSMIT) setMode(oldMode);
    return ok;
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

    FHSSPacket pkt;
    bool got_pkt = false;
    if (has_pending_pkt) {
        pkt = pending_pkt;
        has_pending_pkt = false;
        got_pkt = true;
    } else if (radio->available()) {
        radio->read(&pkt, sizeof(FHSSPacket));
        got_pkt = true;
    }
    if (!got_pkt) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;
    }

    (void)join_state;

    if (pkt.dst_node_id != NODE_ID && pkt.dst_node_id != 0xFF) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;
    }

    markActivity();

    if (pkt.packet_type != static_cast<uint16_t>(PacketType::ND_SYNC) &&
        !isSyncedPeer(pkt.src_node_id)) {
        if (!joined || synced_count == 0) {
            requestSyncFrom(pkt.src_node_id);
        } else {
            sendSyncTo(pkt.src_node_id);
        }
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
    hop_count++;
    if (joined && FHSS_OUT_OF_SYNC_HOPS > 0 &&
        (hop_count - last_activity_hop) >= FHSS_OUT_OF_SYNC_HOPS) {
        joined = false;
        join_state = JOIN_IDLE;
        if (fhss_timer) {
            timerAlarmDisable(fhss_timer);
            timer_alarm_active = false;
        }
        startActiveScan(millis());
        return;
    }

    channel_idx = channel_idx % HOPPING_CHANNELS_SIZE;  // guard against corruption
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
        // Save to pending buffer so read() can consume it; don't just drop it,
        // otherwise this call in the main loop would silently eat chat/pong packets.
        if (!has_pending_pkt) {
            pending_pkt = pkt;
            has_pending_pkt = true;
        }
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

    synced_count = 0;
    memset(synced_nodes, 0, sizeof(synced_nodes));
    last_activity_hop = hop_count;

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
            timerWrite(fhss_timer, 0);
            timerAlarmEnable(fhss_timer);  // Enable now — counter is 0, alarm at 200000
            timer_alarm_active = true;
            joined = true;
            join_state = JOIN_IDLE;

            NDSyncData created;
            created.timer_val = timerRead(fhss_timer);
            created.channel_idx = channel_idx;
            writeRaw(PacketType::ND_SYNC, &created, sizeof(created), 0xFF);
            LOG_INFO("No synchronized reply. Formed new network.");
            return;
        }

        if (now_ms - join_last_tx_ms >= FHSS_BOOTSTRAP_RETRY_MS) {
            NDSyncData req;
            req.timer_val = -1;
            req.channel_idx = 0;
            writeRaw(PacketType::ND_SYNC, &req, sizeof(req), 0xFF);
            join_last_tx_ms = now_ms;
        }
        return;
    }

    // Passive listen / active-wait removed: spec requires active scan only.
}

void transceiver::triggerManualSync(uint32_t now_ms) {
    if (fhss_timer) {
        timerAlarmDisable(fhss_timer);
        timer_alarm_active = false;
    }
    needs_hop = false;
    joined = false;
    join_state = JOIN_IDLE;
    channel_idx = 0;
    synced_count = 0;
    memset(synced_nodes, 0, sizeof(synced_nodes));
    startActiveScan(now_ms);
}

void transceiver::handleBackgroundSync(FHSSPacket* pkt) {
    if (!fhss_timer || !pkt) return;

    markActivity();

    NDSyncData* syncData = (NDSyncData*)pkt->data;

    // --- Sync request (timer_val == -1) ---
    if (syncData->timer_val == -1) {
        NDSyncData reply;
        if (!joined) {
            // Echoing -1 back is useless: when both devices scan simultaneously
            // they both echo forever and both timeout into separate networks.
            // Instead, the device that receives the request creates the network
            // immediately so the requester can join it.
            timerAlarmWrite(fhss_timer, FHSS_TIMER_PERIOD_US, true);
            timerWrite(fhss_timer, 0);
            timerAlarmEnable(fhss_timer);
            timer_alarm_active = true;
            joined = true;
            join_state = JOIN_IDLE;
            noteSyncPeer(pkt->src_node_id);
            reply.timer_val = timerRead(fhss_timer);
            reply.channel_idx = channel_idx;
            writeRaw(PacketType::ND_SYNC, &reply, sizeof(reply), pkt->src_node_id);
            LOG_INFO("Created network for 0x%02X", pkt->src_node_id);
            return;
        }
        noteSyncPeer(pkt->src_node_id);
        reply.timer_val = timerRead(fhss_timer);
        reply.channel_idx = channel_idx;
        writeRaw(PacketType::ND_SYNC, &reply, sizeof(reply), pkt->src_node_id);
        LOG_INFO("Sent sync reply: timer=%lld ch=%u", reply.timer_val,
                 reply.channel_idx);
        return;
    }

    // --- Valid timer value received ---

    // Reject packets with an impossible channel index (corrupted / RF noise)
    if (syncData->channel_idx >= HOPPING_CHANNELS_SIZE) return;

    // Reject timer values outside [0, FHSS_TIMER_PERIOD_US). A negative value
    // (other than -1 caught above) cast to uint64 produces a near-max counter
    // value, so timerWrite() would prevent the alarm from ever firing again.
    if (syncData->timer_val < 0 ||
        (uint64_t)syncData->timer_val >= (uint64_t)FHSS_TIMER_PERIOD_US) return;

    // New join: sync both timer and channel index, then switch to current channel
    if (!joined || synced_count == 0) {
        channel_idx = syncData->channel_idx;
        // The sender's channel_idx is the NEXT hop; current channel is one behind
        uint8_t cur_ch = (syncData->channel_idx - 1 + HOPPING_CHANNELS_SIZE) %
                         HOPPING_CHANNELS_SIZE;
        if (mode != TRANSMIT) radio->stopListening();
        radio->setChannel(HOPPING_CHANNELS[cur_ch]);
        if (mode != TRANSMIT) radio->startListening();
        timerWrite(fhss_timer, syncData->timer_val);
        timerAlarmEnable(fhss_timer);  // Enable immediately after write to avoid race
        timer_alarm_active = true;
        joined = true;
        join_state = JOIN_IDLE;
        noteSyncPeer(pkt->src_node_id);
        LOG_INFO("Joined network. timer=%lld ch_idx=%u on ch=%u",
                 syncData->timer_val, syncData->channel_idx,
                 HOPPING_CHANNELS[cur_ch]);
        return;
    }

    // Unknown peer while already joined: help them sync
    if (!isSyncedPeer(pkt->src_node_id)) {
        sendSyncTo(pkt->src_node_id);
        return;
    }

    // Known peer: apply drift correction if within the correction window
    int64_t my_val = (int64_t)timerRead(fhss_timer);
    int64_t diff = syncData->timer_val - my_val;
    // Fold diff into [-period/2, +period/2]
    if (diff > (int64_t)(FHSS_TIMER_PERIOD_US / 2))
        diff -= (int64_t)FHSS_TIMER_PERIOD_US;
    if (diff < -(int64_t)(FHSS_TIMER_PERIOD_US / 2))
        diff += (int64_t)FHSS_TIMER_PERIOD_US;
    int64_t diff_abs = diff < 0 ? -diff : diff;

    if (diff_abs <= (int64_t)FHSS_DRIFT_CORRECTION_US) {
        timerWrite(fhss_timer, (uint64_t)syncData->timer_val);
        // Also correct channel index if it drifted
        if (channel_idx != syncData->channel_idx) {
            channel_idx = syncData->channel_idx;
            uint8_t cur_ch = (syncData->channel_idx - 1 + HOPPING_CHANNELS_SIZE) %
                             HOPPING_CHANNELS_SIZE;
            if (mode != TRANSMIT) radio->stopListening();
            radio->setChannel(HOPPING_CHANNELS[cur_ch]);
            if (mode != TRANSMIT) radio->startListening();
        }
    }
}

void transceiver::periodicSync(uint32_t now_ms) {
    if (!joined || !fhss_timer || synced_count == 0) return;
    if (now_ms - last_sync_bcast_ms < 5000) return;
    last_sync_bcast_ms = now_ms;
    NDSyncData sync;
    sync.timer_val = timerRead(fhss_timer);
    sync.channel_idx = channel_idx;
    writeRaw(PacketType::ND_SYNC, &sync, sizeof(sync), 0xFF);
}
