#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <RF24.h>
#include <stdbool.h>
#include <esp32-hal-timer.h>
#include <protocol.h>

enum transceiverMode { TRANSMIT, RECEIVE };

enum joinState {
    JOIN_IDLE,
    JOIN_PASSIVE_LISTEN,
    JOIN_ACTIVE_WAIT,
    JOIN_ACTIVE_SCAN,
};

// TODO: research RF24::startConstCarrier.
struct transceiver {
    RF24* radio;

    transceiverMode mode;
    uint8_t channel_idx = 0;
    hw_timer_t* fhss_timer = nullptr;
    bool joined = false;
    joinState join_state = JOIN_IDLE;
    bool join_heard_packet = false;
    uint32_t join_state_since_ms = 0;
    uint8_t join_scan_channel_idx = 0;
    bool join_scan_listen_phase = true;
    uint32_t join_scan_next_ms = 0;

    void setup();
    void setMode(transceiverMode newMode);

    /**
     * @brief Transmits a packet using CSMA/CA to avoid collisions.
     * 
     * @param type The PacketType (CHAT, PONG, TEST, etc.) which is embedded in the FHSS network header.
     * @param data Pointer to the application payload (e.g. Message struct).
     * @param len Length of the application payload in bytes (max 28).
     * @param dst_node_id The destination NODE_ID. Defaults to 0xFF for a network broadcast.
     * @return true if the channel was clear and transmission was attempted, false on CSMA failure or wrong mode.
     */
    bool write(PacketType type, const void* data, uint8_t len, uint8_t dst_node_id = 0xFF);

    /**
     * @brief Reads a packet filtering by NODE_ID and expected PacketType.
     * 
     * @param expected_type The exact PacketType the caller is expecting to process. Other types are dropped.
     * @param data Pointer to the buffer where the extracted inner application payload should be stored.
     * @param len Max number of payload bytes to copy into the data buffer.
     * @param out_src_node_id Optional pointer to capture the sender's NODE_ID from the network header.
     * @return true if a matching packet was received and successfully copied, false otherwise.
     */
    bool read(PacketType expected_type, void* data, uint8_t len, uint8_t* out_src_node_id = nullptr);
    
    /**
     * @brief Template wrapper for write() that automatically calculates payload size.
     */
    template <typename T>
    bool write(PacketType type, const T& data, uint8_t dst_node_id = 0xFF) {
        return write(type, &data, sizeof(T), dst_node_id);
    }

    /**
     * @brief Template wrapper for read() that automatically calculates payload size.
     */
    template <typename T>
    bool read(PacketType expected_type, T& data, uint8_t* out_src_node_id = nullptr) {
        return read(expected_type, &data, sizeof(T), out_src_node_id);
    }

    bool readAudio(void* data, uint8_t len);
    void hop();

    bool writeRaw(PacketType type, const void* data, uint8_t len,
                  uint8_t dst_node_id = 0xFF);
    bool readSyncPacket();
    void startPassiveJoin(uint32_t now_ms);
    void startActiveScan(uint32_t now_ms);
    void updateJoin(uint32_t now_ms);
    void handleBackgroundSync(FHSSPacket* pkt);
};

#endif  // TRANSCEIVER_H