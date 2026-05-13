#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <RF24.h>
#include <stdbool.h>

enum transceiverMode { TRANSMIT, RECEIVE };

enum class PacketType : uint8_t {
    PONG = 0,
    CHAT = 1,
    AUDIO = 2,
    ND_SYNC = 3,
    TEST = 4,
    ACK = 5
};

struct __attribute__((packed)) FHSSPacket {
    uint8_t src_node_id;
    uint8_t dst_node_id;
    uint16_t packet_type : 4;
    uint16_t fec : 12;
    uint8_t data[28];
};

static_assert(sizeof(FHSSPacket) == 32,
              "FHSSPacket size must be exactly 32 bytes");

// TODO: research RF24::startConstCarrier.
struct transceiver {
    RF24* radio;

    transceiverMode mode;
    uint8_t channel_idx = 0;

    void setup();
    void setMode(transceiverMode newMode);
    bool write(const void* data, uint8_t len);
    bool read(void* data, uint8_t len);
    bool readAudio(void* data, uint8_t len);
    void hop();
};

#endif  // TRANSCEIVER_H