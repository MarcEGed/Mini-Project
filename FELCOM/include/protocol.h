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

// ===========================================================================
// FEC layer — see "FEC Prototype — Design Document".
//
// Hard constraints (do not break):
//   Rule 1: data[28] never changes size. The CRC lives INSIDE data, in the
//           last 2 bytes. User payload is the first 26 bytes.
//   Rule 2: The 12-bit `fec` bit-field carries metadata, NOT the CRC (16 bits
//           does not fit in 12). It is split scheme(4) | block_id(4) | index(4).
// ===========================================================================

// data[] layout for protected packets (Rule 1)
#define FHSS_DATA_SIZE 28               // sizeof(FHSSPacket::data) — unchanged
#define FEC_CRC_SIZE 2                  // CRC16 occupies data[26..27]
#define FEC_USER_SIZE (FHSS_DATA_SIZE - FEC_CRC_SIZE)  // 26 user bytes: data[0..25]

// ARQ wire layout (inside the 26 user bytes): [seq(2) | app payload | pad]
#define FEC_ARQ_SEQ_SIZE 2
#define FEC_ARQ_PAYLOAD_SIZE (FEC_USER_SIZE - FEC_ARQ_SEQ_SIZE)  // 24 app bytes

// Which complementary mechanism a packet uses. Stored in fec bits [11:8].
enum class FecScheme : uint8_t {
    NONE = 0,     // raw, unprotected (TEST + internal)
    CRC = 1,      // CRC16 detection only (PONG, ACK, broadcast)
    XOR = 2,      // CRC + XOR block forward correction (AUDIO)
    CRC_ARQ = 3   // CRC + automatic retransmit (CHAT, ND_SYNC)
};

// 12-bit `fec` field layout (Rule 2):
//   bits [11:8] scheme    — FecScheme
//   bits  [7:4] block_id  — XOR block identifier (0-15, wraps)
//   bits  [3:0] index     — slot within block (0..3 data, 0xF = parity)
#define FEC_INDEX_PARITY 0x0F

#define fecPackMeta(scheme, blockId, index)                       \
    ((uint16_t)((((uint16_t)(scheme) & 0x0F) << 8) |              \
                (((uint16_t)(blockId) & 0x0F) << 4) |             \
                ((uint16_t)(index) & 0x0F)))
#define fecGetScheme(fec) ((uint8_t)(((fec) >> 8) & 0x0F))
#define fecGetBlockId(fec) ((uint8_t)(((fec) >> 4) & 0x0F))
#define fecGetIndex(fec) ((uint8_t)((fec) & 0x0F))

struct __attribute__((packed)) NDSyncData {
    int64_t timer_val;
    uint8_t nbSyncedNodes;
    // we can only fit 10 synced nodes
    uint8_t syncedNodes[10];
};

// ACK control packet (CRC-protected, never itself ACKed).
struct __attribute__((packed)) AckPayload {
    uint16_t seq;       // the ARQ seq being acknowledged
    uint8_t status;     // 0 = OK
    uint8_t reserved;
};

static_assert(sizeof(AckPayload) <= FEC_USER_SIZE,
              "AckPayload must fit in the protected user region");

// Audio frame for XOR block FEC. Sender identity comes from the network
// header (FHSSPacket::src_node_id), so it is not duplicated here.
#define FEC_AUDIO_SAMPLES 24
struct __attribute__((packed)) AudioPayload {
    uint16_t seq;                          // 2 bytes
    uint8_t samples[FEC_AUDIO_SAMPLES];    // 24 samples @ 8 kHz = 3 ms / packet
};

static_assert(sizeof(AudioPayload) == FEC_USER_SIZE,
              "AudioPayload should exactly fill the protected user region");

#endif  // PROTOCOL_H
