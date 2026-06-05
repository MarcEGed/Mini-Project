#include "transceiver.h"

#include <config.h>
#include <debug.h>

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
    radio->disableCRC();

    radio->openWritingPipe(PHY_ADDRESSES[NRF24L01_PHY_ADDR]);
    radio->openReadingPipe(1, PHY_ADDRESSES[NRF24L01_PHY_ADDR]);

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

    if (pkt.dst_node_id != NODE_ID && pkt.dst_node_id != 0xFF) {
        if (oldMode != RECEIVE) setMode(oldMode);
        return false;  // Packet not meant for this node
    }

    if (pkt.packet_type != static_cast<uint16_t>(expected_type)) {
        // Intercept ND_SYNC requests
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
    radio->setChannel(HOPPING_CHANNELS[0]);
    if (mode != TRANSMIT) {
        radio->startListening();
    }
    //channel_idx = (channel_idx + 1) % HOPPING_CHANNELS_SIZE;
}

void transceiver::sendSync(uint32_t value) {
    NDSyncData payload = {0};
    payload.timer_val = (int64_t)value;
    write(PacketType::ND_SYNC, payload);
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