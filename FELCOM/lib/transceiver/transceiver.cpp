#include "transceiver.h"

#include <config.h>

void transceiver::setup(){
    radio = new RF24(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);
    radio->begin();
    radio->setPALevel(powerLevel);
    radio->setDataRate(dataRate);
    radio->setChannel(channel);

    radio->setAutoAck(false);
    radio->setRetries(0, 0);
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
    return true;
}