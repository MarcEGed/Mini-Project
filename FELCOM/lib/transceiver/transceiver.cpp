#include "transceiver.h"

#include <config.h>
#include <debug.h>
#include <message.h>

void transceiver::setup() {
    radio = new RF24(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);
    radio->begin();
    radio->setPALevel(NRF24L01_POWER_LEVEL);
    radio->setDataRate(NRF24L01_DATA_RATE);
    radio->setChannel(USABLE_CHANNELS[channel_idx]);
    radio->setPayloadSize(sizeof(Message));
    // radio->enableDynamicPayloads();

    radio->setAutoAck(true);
    // wait 0*250us + 250us before each retry
    radio->setRetries(0, 3);
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

bool transceiver::write(const void* data, uint8_t len) {
    if (mode != TRANSMIT) {
        return false;
    }
    bool ok = radio->write(data, len);
    hop();
    return ok;
}

bool transceiver::read(void* data, uint8_t len) {
    if (mode != RECEIVE) return false;
    if (!radio->available()) return false;

    radio->read(data, len);
    hop();
    LOG_INFO("Packet received");
    return true;
}

void transceiver::hop() {
    if (mode != TRANSMIT) {
        radio->stopListening();
    }
    radio->setChannel(USABLE_CHANNELS[channel_idx]);
    if (mode != TRANSMIT) {
        radio->startListening();
    }
    channel_idx = (channel_idx + 1) % (NRF24L01_MAX_CHANNEL_INDEX + 1);
}