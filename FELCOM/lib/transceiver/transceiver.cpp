#include "transceiver.h"
#include <message.h>

#include <config.h>
#include <debug.h>

void transceiver::setup(){
    radio = new RF24(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);
    radio->begin();
    radio->setPALevel(NRF24L01_POWER_LEVEL);
    radio->setDataRate(NRF24L01_DATA_RATE);
    radio->setChannel(channel);
    radio->setPayloadSize(sizeof(Message));

    radio->disableCRC();

    radio->setAutoAck(false);
    radio->setRetries(0, 0);

    static const uint64_t PIPE_ADDR = 0xF0F0F0F0E1LL;
    radio->openWritingPipe(PIPE_ADDR);
    radio->openReadingPipe(1, PIPE_ADDR);

    setMode(RECEIVE);
}

void transceiver::setMode(transceiverMode newMode){
    mode = newMode;
    if (newMode == TRANSMIT){
        radio->stopListening();
    }else{
        radio->startListening();
    }
}

bool transceiver::write(const void *data, uint8_t len){
    if (mode != TRANSMIT){
        return false;
    }
    return radio->write(data, len);
}

bool transceiver::read(void *data, uint8_t len){
    if (mode != RECEIVE) return false;
    if (!radio->available()) return false;

    radio->read(data, len);
    LOG_INFO("Packet received");
    return true;
}