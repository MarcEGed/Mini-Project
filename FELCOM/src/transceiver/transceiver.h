#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <config.h>
#include <RF24.h>
#include <stdbool.h>

enum transceiverMode
{
    TRANSMIT,
    RECEIVE
};

// TODO: research RF24::startConstCarrier.
struct transceiver
{
    RF24 *radio;

    transceiverMode mode;
    rf24_pa_dbm_e powerLevel = RF24_PA_LOW;
    rf24_datarate_e dataRate = RF24_1MBPS;
    uint8_t channel = 125;

    void setup()
    {
        radio = new RF24(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);
        radio->begin();
        radio->setPALevel(powerLevel);
        radio->setDataRate(dataRate);
        radio->setChannel(channel);

        radio->setAutoAck(false);
        radio->setRetries(0, 0);
    }

    void setMode(transceiverMode newMode)
    {
        mode = newMode;
        if (newMode == TRANSMIT)
        {
            radio->stopListening();
        }
        else
        {
            radio->startListening();
        }
    }
    bool write(const void *data, uint8_t len)
    {
        if (mode != TRANSMIT)
        {
            return false;
        }
        return radio->write(data, len);
    }

    void read(void *data, uint8_t len)
    {
        if (mode != RECEIVE)
        {
            return;
        }
        if (radio->available())
        {
            radio->read(data, len);
        }
    }
};

#endif // TRANSCEIVER_H