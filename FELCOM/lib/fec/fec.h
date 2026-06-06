#ifndef FEC_H
#define FEC_H

// ===========================================================================
// FEC layer — Forward Error Correction logic for FELCOM.
//
// This module holds ALL FEC algorithms (CRC16-CCITT detection, ARQ retransmit
// bookkeeping, and XOR block forward correction). It performs NO radio I/O:
// the transceiver (lib/transceiver) owns the radio, does CSMA / mode switching,
// and calls into these helpers. Application code never calls this module
// directly — every FEC interaction goes through the transceiver. (Design Rule 3)
// ===========================================================================

#include <config.h>
#include <protocol.h>
#include <stddef.h>
#include <stdint.h>

// All mutable FEC state lives here. The transceiver embeds one instance so the
// FEC behaviour is "inbuilt" within the transceiver rather than global.
struct FecState {
    // --- ARQ (transmit side) ---
    uint16_t tx_seq;        // monotonic sequence counter for outgoing ARQ frames
    bool arq_waiting;       // an ARQ send is awaiting its ACK
    bool arq_acked;         // matching ACK has arrived
    uint16_t arq_seq;       // seq we are currently waiting to be ACKed

    // --- XOR block (transmit side) ---
    uint8_t xor_tx_block_id;                 // current block id (0-15, wraps)
    uint8_t xor_tx_count;                    // data slots sent in this block
    uint8_t xor_tx_parity[FEC_USER_SIZE];    // running XOR parity over data slots

    // --- XOR block (receive side, single block in progress) ---
    uint8_t rx_block_id;                              // 0xFF = no block assembling
    bool rx_slot_valid[FEC_XOR_BLOCK_SIZE];
    uint8_t rx_slot[FEC_XOR_BLOCK_SIZE][FEC_USER_SIZE];
    bool rx_parity_valid;
    uint8_t rx_parity[FEC_USER_SIZE];

    // --- recovered audio delivery queue (ring buffer) ---
    AudioPayload audio_q[FEC_AUDIO_RX_QUEUE];
    uint8_t audio_q_head;
    uint8_t audio_q_tail;
    uint8_t audio_q_count;
};

// Reset every counter, block buffer and queue. Called from transceiver::setup().
void fecInit(FecState& s);

// --- CRC16-CCITT (poly 0x1021, init 0xFFFF), big-endian processing. ---
uint16_t fecCrc16(const uint8_t* data, size_t len);

// Build a protected packet in place: copy up to FEC_USER_SIZE user bytes into
// data[0..25], append CRC16 into data[26..27], and stamp packet_type + fec
// metadata. Does NOT touch the network header (src/dst). (Rule 1, Rule 2)
void fecEncodeProtected(FHSSPacket& pkt, PacketType type, FecScheme scheme,
                        uint8_t blockId, uint8_t index, const void* user,
                        uint8_t len);

// Recompute CRC over data[0..25] and compare with data[26..27].
bool fecCheckCrc(const FHSSPacket& pkt);

// --- XOR block FEC (audio) ---
// Feed one CRC-verified AUDIO packet into block assembly. Recovered / received
// AudioPayloads are pushed onto the audio queue. (transceiver calls this)
void fecAudioAssemble(FecState& s, const FHSSPacket& pkt);

// Pop one recovered/received AudioPayload. Returns false if the queue is empty.
bool fecAudioDequeue(FecState& s, void* out, uint8_t len);

#endif  // FEC_H
