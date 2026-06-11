#include "audio.h"

#include <Arduino.h>
#include <driver/i2s.h>

#include <config.h>
#include <protocol.h>
#include <selected_destination.h>
#include <transceiver.h>

// All state is file-local. Everything runs in the main-loop (cooperative)
// context — there is no ISR and no second task — so no locking is needed.
namespace {

transceiver* g_xcvr = nullptr;
bool g_talking = false;
uint32_t g_rxOverruns = 0;

// --- DAC playback ring (remote RX in LISTEN, local mic monitor in TALK) ----
uint8_t rxRing[AUDIO_RX_RING];
uint16_t rxHead = 0, rxTail = 0;
constexpr uint16_t RX_MASK = AUDIO_RX_RING - 1;
inline uint16_t rxCount() { return (uint16_t)((rxHead - rxTail) & RX_MASK); }
inline void rxPush(uint8_t s) {
    uint16_t next = (uint16_t)((rxHead + 1) & RX_MASK);
    if (next != rxTail) {
        rxRing[rxHead] = s;
        rxHead = next;
    } else {
        g_rxOverruns++;
    }
}
inline uint8_t rxPop() {
    uint8_t s = rxRing[rxTail];
    rxTail = (uint16_t)((rxTail + 1) & RX_MASK);
    return s;
}
inline void rxDropOldestUntil(uint16_t maxQueued) {
    while (rxCount() > maxQueued) {
        rxTail = (uint16_t)((rxTail + 1) & RX_MASK);
    }
}

// --- TX accumulator + DSP state -------------------------------------------
AudioPayload txAccum;
uint8_t txIdx = 0;
uint16_t txSeq = 0;
int32_t dcOffset = 0;

uint32_t g_txPayloads = 0;
uint32_t g_rxPayloads = 0;
uint32_t g_i2sReads = 0;
uint32_t g_i2sEmptyReads = 0;
uint32_t g_micSamples = 0;
uint32_t g_micClippedLow = 0;
uint32_t g_micClippedHigh = 0;
uint32_t g_rxUnderruns = 0;
int32_t g_micPeak = 0;
uint32_t nextSampleUs = 0;
int32_t i2sBuf[64];

static_assert(AUDIO_I2S_SLOT_INDEX == 0 || AUDIO_I2S_SLOT_INDEX == 1,
              "AUDIO_I2S_SLOT_INDEX must be 0 or 1");
static_assert(AUDIO_LOCAL_MONITOR_MAX_QUEUED < AUDIO_RX_RING,
              "AUDIO_LOCAL_MONITOR_MAX_QUEUED must be smaller than AUDIO_RX_RING");

void initMic() {
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate = AUDIO_SAMPLE_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    // Stereo avoids ESP32 legacy ONLY_LEFT silence on this board. The selected
    // buffer slot is controlled by AUDIO_I2S_SLOT_INDEX for easy mic debugging.
    cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = 4;
    cfg.dma_buf_len = 128;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = false;
    cfg.fixed_mclk = 0;
    i2s_driver_install(I2S_PORT, &cfg, 0, NULL);

    i2s_pin_config_t pins = {};
    pins.bck_io_num = I2S_SCK_PIN;
    pins.ws_io_num = I2S_WS_PIN;
    pins.data_in_num = I2S_SD_PIN;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    i2s_set_pin(I2S_PORT, &pins);
    i2s_zero_dma_buffer(I2S_PORT);
}

// TALK: read whatever the mic DMA has (non-blocking), take the configured I2S
// slot, DC-remove + gain + 8-bit, accumulate into AudioPayloads for the FEC.
void capture() {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, i2sBuf, sizeof(i2sBuf), &bytesRead, 0);  // timeout 0
    g_i2sReads++;
    if (bytesRead == 0) {
        g_i2sEmptyReads++;
        return;
    }

    uint32_t count = bytesRead / sizeof(int32_t);
    for (uint32_t i = AUDIO_I2S_SLOT_INDEX; i < count; i += 2) {
        int32_t sample24 = i2sBuf[i] >> 8;
        dcOffset = (dcOffset * 63 + sample24) / 64;
        int32_t centered = sample24 - dcOffset;
        int32_t peak = centered >= 0 ? centered : -centered;
        if (peak > g_micPeak) g_micPeak = peak;
        g_micSamples++;

        int32_t scaled = (centered * AUDIO_GAIN) >> 16;
        int32_t v = 128 + scaled;
        if (v < 0) {
            v = 0;
            g_micClippedLow++;
        }
        if (v > 255) {
            v = 255;
            g_micClippedHigh++;
        }
        uint8_t out = (uint8_t)v;
#if AUDIO_LOCAL_MONITOR
        rxDropOldestUntil(AUDIO_LOCAL_MONITOR_MAX_QUEUED);
        rxPush(out);
#endif
        txAccum.samples[txIdx++] = out;
        if (txIdx >= AUDIO_PACKET_SAMPLES) {
            txAccum.seq = txSeq++;
#if AUDIO_RADIO_TX_ENABLE
            g_xcvr->audioTx(
                txAccum,
                SELECTED_DST_NODE);  // CRC + XOR block (+ parity per block)
            g_txPayloads++;
#endif
            txIdx = 0;
        }
    }
}

void dacPlayback(bool countUnderrun) {
    uint32_t now = micros();
    constexpr uint32_t SAMPLE_US = 1000000UL / AUDIO_SAMPLE_RATE;

    // If a blocking operation stalled us badly, skip ahead instead of carrying
    // hundreds of milliseconds of DAC latency in the software clock.
    if ((int32_t)(now - nextSampleUs) > (int32_t)(SAMPLE_US * 16)) {
        nextSampleUs = now;
    }

    // Emit a small burst if needed to recover from short stalls, but cap it so
    // radio/UI work still gets CPU time.
    for (uint8_t n = 0; n < 4 && (int32_t)(micros() - nextSampleUs) >= 0; n++) {
        nextSampleUs += SAMPLE_US;
        if (rxCount() > 0) {
            dacWrite(DAC_OUT_PIN, rxPop());
        } else {
            if (countUnderrun) g_rxUnderruns++;
            dacWrite(DAC_OUT_PIN, 128);
        }
    }
}

// TALK: play the locally captured/transmitted mic samples out the DAC so the
// operator can hear what is being sent before the radio path is involved.
void localMonitorPlayback() {
#if AUDIO_LOCAL_MONITOR
    dacPlayback(false);
#endif
}

// LISTEN: drain FEC-recovered payloads into the ring, emit one DAC sample per
// 1/AUDIO_SAMPLE_RATE second (micros-paced; silence on underrun).
void playback() {
    AudioPayload p;
    while (g_xcvr->audioRx(p)) {
        for (uint8_t i = 0; i < AUDIO_PACKET_SAMPLES; i++) rxPush(p.samples[i]);
        g_rxPayloads++;
    }
    dacPlayback(true);
}

}  // namespace

namespace audio {

void begin(transceiver* xcvr) {
    g_xcvr = xcvr;
    initMic();
    dacWrite(DAC_OUT_PIN, 128);  // idle the DAC at mid-scale (silence)
}

void enter() {
    rxHead = rxTail = 0;
    txIdx = 0;
    dcOffset = 0;
    g_txPayloads = 0;
    g_rxPayloads = 0;
    g_i2sReads = 0;
    g_i2sEmptyReads = 0;
    g_micSamples = 0;
    g_micClippedLow = 0;
    g_micClippedHigh = 0;
    g_rxUnderruns = 0;
    g_rxOverruns = 0;
    g_micPeak = 0;
    g_talking = false;  // start by listening
    i2s_zero_dma_buffer(I2S_PORT);
    nextSampleUs = micros();
}

void leave() {
    dacWrite(DAC_OUT_PIN, 128);
    if (g_xcvr) g_xcvr->setMode(RECEIVE);
}

void setTalking(bool talking) {
    if (talking == g_talking) return;
    g_talking = talking;
    if (talking) {
        txIdx = 0;
        rxHead = rxTail = 0;
        nextSampleUs = micros();
        i2s_zero_dma_buffer(I2S_PORT);  // drop stale mic samples
    } else {
        rxHead = rxTail = 0;
        nextSampleUs = micros();
        dacWrite(DAC_OUT_PIN, 128);
    }
}

bool isTalking() { return g_talking; }

void update() {
    if (!g_xcvr) return;
    if (g_talking) {
        capture();
        localMonitorPlayback();
    } else {
        playback();
    }
}

uint32_t txPayloads() { return g_txPayloads; }
uint32_t rxPayloads() { return g_rxPayloads; }

Stats stats() {
    return Stats{g_txPayloads,      g_rxPayloads,     g_i2sReads,
                 g_i2sEmptyReads,   g_micSamples,     g_micClippedLow,
                 g_micClippedHigh,  g_rxUnderruns,    g_rxOverruns,
                 rxCount(),         g_micPeak};
}

}  // namespace audio
