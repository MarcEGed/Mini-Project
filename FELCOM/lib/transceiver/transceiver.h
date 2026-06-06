#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <RF24.h>
#include <fec.h>
#include <protocol.h>
#include <stdbool.h>

enum transceiverMode { TRANSMIT, RECEIVE };

// TODO: research RF24::startConstCarrier.
struct transceiver {
    RF24* radio;

    transceiverMode mode;
    hw_timer_t* fhss_timer = nullptr;

    // FEC is inbuilt: the transceiver owns all FEC state and every FEC
    // interaction (CRC, ARQ, XOR audio) happens through transceiver methods.
    FecState fec;

    void setup();
    void setMode(transceiverMode newMode);

    // --- Application-facing send API -------------------------------------
    /**
     * @brief CRC16-protected send (PONG, broadcasts). TEST bypasses FEC.
     *
     * The 26-byte user region is protected by a CRC16 in data[26..27]; on
     * receipt a failed CRC drops the packet. PacketType::TEST is sent raw so
     * the BER menu still measures real bit errors (Rule 4).
     *
     * @param type The PacketType embedded in the FHSS network header.
     * @param data Pointer to the application payload.
     * @param len  Length of the payload in bytes (max 26, or 28 for TEST).
     * @param dst_node_id Destination NODE_ID. 0xFF = broadcast.
     * @return true if the channel was clear and transmission was attempted.
     */
    bool write(PacketType type, const void* data, uint8_t len,
               uint8_t dst_node_id = 0xFF);

    /**
     * @brief CRC + ARQ send (CHAT). Prepends a seq number, sends, then BLOCKS
     *        waiting for a matching ACK, retransmitting on timeout.
     *
     * Note: blocks for up to FEC_ARQ_TIMEOUT_MS * (1 + FEC_ARQ_MAX_RETRIES) and
     * does not hop channels meanwhile, so use only for infrequent, must-deliver
     * traffic between already-synced nodes. ND_SYNC uses plain write() instead.
     *
     * @param len Application payload length (max 24; 2 bytes reserved for seq).
     * @return true if an ACK was received within the retry budget, else false.
     */
    bool writeReliable(PacketType type, const void* data, uint8_t len,
                       uint8_t dst_node_id = 0xFF);

    /**
     * @brief No-FEC raw send (TEST and internal use). fec metadata = NONE.
     */
    bool writeRaw(PacketType type, const void* data, uint8_t len,
                  uint8_t dst_node_id = 0xFF);

    // --- Application-facing receive API ----------------------------------
    /**
     * @brief Reads a packet filtering by NODE_ID and expected PacketType.
     *
     * Verifies the CRC of protected packets (dropping corrupt frames), strips
     * the ARQ seq for reliable types and auto-replies with an ACK, and silently
     * services ACK / AUDIO frames in the background.
     *
     * @param expected_type The PacketType the caller wants. Others are dropped.
     * @param data Buffer for the extracted application payload.
     * @param len  Max payload bytes to copy.
     * @param out_src_node_id Optional sender NODE_ID.
     * @return true if a matching, CRC-valid packet was delivered.
     */
    bool read(PacketType expected_type, void* data, uint8_t len,
              uint8_t* out_src_node_id = nullptr);

    /**
     * @brief Drain background FEC traffic (incoming ACKs and XOR audio blocks).
     *
     * WARNING: this consumes frames from the single radio FIFO and buffers at
     * most one application frame, dropping the rest. Do NOT call it every main-
     * loop iteration alongside per-type read()s (it would starve high-rate
     * traffic such as the RF/BER test). It is invoked internally by audioRx();
     * ACKs are drained by writeReliable()'s own wait loop.
     */
    void poll();

    /**
     * @brief Print a one-line FEC counter summary to the serial log, but only
     *        if a counter changed since the last print. Call it throttled (e.g.
     *        every few seconds) — a serial flush blocks ~50 ms at 9600 baud.
     */
    void logFecStats();

    /** @brief Read-only access to the live FEC diagnostic counters. */
    const FecStats& fecStats() const { return fec.stats; }

    // --- Template payload helpers ----------------------------------------
    template <typename T>
    bool write(PacketType type, const T& data, uint8_t dst_node_id = 0xFF) {
        return write(type, &data, sizeof(T), dst_node_id);
    }

    template <typename T>
    bool writeReliable(PacketType type, const T& data,
                       uint8_t dst_node_id = 0xFF) {
        return writeReliable(type, &data, sizeof(T), dst_node_id);
    }

    template <typename T>
    bool read(PacketType expected_type, T& data,
              uint8_t* out_src_node_id = nullptr) {
        return read(expected_type, &data, sizeof(T), out_src_node_id);
    }

    // --- Audio (XOR block FEC) -------------------------------------------
    /** @brief Buffer + transmit one audio frame, emitting parity per block. */
    void audioTx(const void* payload, uint8_t len);
    template <typename T>
    void audioTx(const T& payload) {
        audioTx(&payload, sizeof(T));
    }
    /** @brief Pop a recovered/received audio frame. false if none ready. */
    bool audioRx(void* payload, uint8_t len);
    template <typename T>
    bool audioRx(T& payload) {
        return audioRx(&payload, sizeof(T));
    }

    // --- Low-level radio helpers (used by the FEC paths above) -----------
    /** @brief CSMA + raw radio write of a fully-built frame. (Rule 3) */
    bool transmitPacket(FHSSPacket& pkt);
    /** @brief Read one frame addressed to us (filters dst_node_id). (Rule 3) */
    bool receiveRaw(FHSSPacket& pkt, uint8_t* out_src_node_id = nullptr);

    void sendSync(uint32_t value);
    void hop();
    void setChannel(uint8_t channel);

   private:
    // Single-slot pushback for a verified application frame, so background
    // ACK/audio draining never loses a packet meant for read().
    bool rx_has;
    FHSSPacket rx_pkt;
    uint8_t rx_src;

    // Last printed counter total, so logFecStats() stays quiet when idle.
    uint32_t stats_printed_sum;

    // Read+dispatch one frame: ACK→handleAck, AUDIO→assemble, TEST→buffer raw,
    // CRC-valid app frame→buffer (+auto-ACK if ARQ). Returns false when no
    // for-us frame remains in the FIFO.
    bool serviceRx();
    void sendAck(uint8_t dst_node_id, uint16_t seq);
    void handleAck(const FHSSPacket& pkt);
};

#endif  // TRANSCEIVER_H
