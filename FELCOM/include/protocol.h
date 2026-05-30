#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

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

struct __attribute__((packed)) NDSyncData {
    int64_t timer_val;
    uint8_t channel_idx;
};

#endif  // PROTOCOL_H
