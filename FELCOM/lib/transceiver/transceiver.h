#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <RF24.h>
#include <stdbool.h>

enum transceiverMode{
    TRANSMIT,
    RECEIVE
};

// TODO: research RF24::startConstCarrier.
struct transceiver{
    RF24 *radio;

    transceiverMode mode;
    uint8_t channel = 125;

    void setup();
    void setMode(transceiverMode newMode);
    bool write(const void *data, uint8_t len);
    bool read(void *data, uint8_t len);
    bool readAudio(void* data, uint8_t len);
};

#endif // TRANSCEIVER_H