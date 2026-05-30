#ifndef TEST_MESSAGE_H
#define TEST_MESSAGE_H

#include <config.h>
#include <stdint.h>

struct __attribute__((packed)) TestPacket {
    uint32_t seq;
    uint8_t payload[24];
};

static_assert(sizeof(TestPacket) == RF_TEST_PACKET_SIZE,
              "TestPacket size must match RF_TEST_PACKET_SIZE");

#endif  // TEST_MESSAGE_H
