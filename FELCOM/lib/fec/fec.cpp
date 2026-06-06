#include "fec.h"

#include <string.h>

void fecInit(FecState& s) {
    memset(&s, 0, sizeof(FecState));
    s.rx_block_id = 0xFF;  // no XOR block in progress
}

// CRC16-CCITT (XModem variant): polynomial 0x1021, initial value 0xFFFF,
// no final XOR, MSB-first. 16 bits of detection in the last 2 data bytes.
uint16_t fecCrc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

void fecEncodeProtected(FHSSPacket& pkt, PacketType type, FecScheme scheme,
                        uint8_t blockId, uint8_t index, const void* user,
                        uint8_t len) {
    pkt.packet_type = (uint16_t)type & 0x0F;
    pkt.fec = fecPackMeta((uint8_t)scheme, blockId, index);

    uint8_t n = len > FEC_USER_SIZE ? FEC_USER_SIZE : len;
    memset(pkt.data, 0, FHSS_DATA_SIZE);
    if (user && n) memcpy(pkt.data, user, n);

    uint16_t crc = fecCrc16(pkt.data, FEC_USER_SIZE);
    memcpy(pkt.data + FEC_USER_SIZE, &crc, FEC_CRC_SIZE);
}

bool fecCheckCrc(const FHSSPacket& pkt) {
    uint16_t calc = fecCrc16(pkt.data, FEC_USER_SIZE);
    uint16_t stored;
    memcpy(&stored, pkt.data + FEC_USER_SIZE, FEC_CRC_SIZE);
    return calc == stored;
}

// --- XOR block FEC (audio) -------------------------------------------------

static void fecAudioEnqueue(FecState& s, const uint8_t* userRegion) {
    if (s.audio_q_count >= FEC_AUDIO_RX_QUEUE) {
        // Queue full: drop oldest so live audio keeps flowing.
        s.audio_q_head = (s.audio_q_head + 1) % FEC_AUDIO_RX_QUEUE;
        s.audio_q_count--;
    }
    memcpy(&s.audio_q[s.audio_q_tail], userRegion, sizeof(AudioPayload));
    s.audio_q_tail = (s.audio_q_tail + 1) % FEC_AUDIO_RX_QUEUE;
    s.audio_q_count++;
}

static void fecAudioResetBlock(FecState& s, uint8_t blockId) {
    s.rx_block_id = blockId;
    s.rx_parity_valid = false;
    for (uint8_t i = 0; i < FEC_XOR_BLOCK_SIZE; i++) s.rx_slot_valid[i] = false;
}

// Deliver the current block, XOR-recovering one missing data slot if possible.
static void fecAudioFinalizeBlock(FecState& s) {
    if (s.rx_block_id == 0xFF) return;

    uint8_t missing = 0xFF;
    uint8_t missingCount = 0;
    for (uint8_t i = 0; i < FEC_XOR_BLOCK_SIZE; i++) {
        if (!s.rx_slot_valid[i]) {
            missing = i;
            missingCount++;
        }
    }

    // Exactly one erasure + valid parity → reconstruct it. (XOR recovers one)
    if (missingCount == 1 && s.rx_parity_valid) {
        for (uint8_t b = 0; b < FEC_USER_SIZE; b++) {
            uint8_t v = s.rx_parity[b];
            for (uint8_t i = 0; i < FEC_XOR_BLOCK_SIZE; i++) {
                if (i != missing && s.rx_slot_valid[i]) v ^= s.rx_slot[i][b];
            }
            s.rx_slot[missing][b] = v;
        }
        s.rx_slot_valid[missing] = true;
    }

    // Deliver every slot we have (recovered or originally received), in order.
    for (uint8_t i = 0; i < FEC_XOR_BLOCK_SIZE; i++) {
        if (s.rx_slot_valid[i]) fecAudioEnqueue(s, s.rx_slot[i]);
    }

    s.rx_block_id = 0xFF;
}

void fecAudioAssemble(FecState& s, const FHSSPacket& pkt) {
    uint8_t blk = fecGetBlockId(pkt.fec);
    uint8_t idx = fecGetIndex(pkt.fec);

    if (s.rx_block_id == 0xFF) {
        fecAudioResetBlock(s, blk);
    } else if (blk != s.rx_block_id) {
        // A new block began before the previous one closed — flush the old one.
        fecAudioFinalizeBlock(s);
        fecAudioResetBlock(s, blk);
    }

    if (idx == FEC_INDEX_PARITY) {
        memcpy(s.rx_parity, pkt.data, FEC_USER_SIZE);
        s.rx_parity_valid = true;
    } else if (idx < FEC_XOR_BLOCK_SIZE) {
        memcpy(s.rx_slot[idx], pkt.data, FEC_USER_SIZE);
        s.rx_slot_valid[idx] = true;
    }

    // Close the block once every data slot is present, or once parity (the last
    // packet of a block) arrives so we can attempt recovery.
    bool allData = true;
    for (uint8_t i = 0; i < FEC_XOR_BLOCK_SIZE; i++) {
        if (!s.rx_slot_valid[i]) {
            allData = false;
            break;
        }
    }
    if (allData || idx == FEC_INDEX_PARITY) fecAudioFinalizeBlock(s);
}

bool fecAudioDequeue(FecState& s, void* out, uint8_t len) {
    if (s.audio_q_count == 0) return false;
    uint8_t n = len > sizeof(AudioPayload) ? sizeof(AudioPayload) : len;
    memcpy(out, &s.audio_q[s.audio_q_head], n);
    s.audio_q_head = (s.audio_q_head + 1) % FEC_AUDIO_RX_QUEUE;
    s.audio_q_count--;
    return true;
}
