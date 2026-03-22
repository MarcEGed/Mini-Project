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
    rf24_pa_dbm_e powerLevel = RF24_PA_LOW;
    rf24_datarate_e dataRate = RF24_1MBPS;
    uint8_t channel = 125;

    void setup();
    void setMode(transceiverMode newMode);
    bool write(const void *data, uint8_t len);
    bool read(void *data, uint8_t len);
};

#endif // TRANSCEIVER_H