#include "audio.h"

#include <Arduino.h>
#include <driver/i2s.h>

#include <config.h>
#include <protocol.h>
#include <transceiver.h>

// All state is file-local. Everything runs in the main-loop (cooperative)
// context — there is no ISR and no second task — so no locking is needed.
namespace {

transceiver* g_xcvr = nullptr;
bool g_talking = false;

// --- RX playback ring (smooths radio jitter before the DAC) ---------------
uint8_t rxRing[AUDIO_RX_RING];
uint16_t rxHead = 0, rxTail = 0;
constexpr uint16_t RX_MASK = AUDIO_RX_RING - 1;
inline uint16_t rxCount() { return (uint16_t)((rxHead - rxTail) & RX_MASK); }
inline void rxPush(uint8_t s) {
    uint16_t next = (uint16_t)((rxHead + 1) & RX_MASK);
    if (next != rxTail) {
        rxRing[rxHead] = s;
        rxHead = next;
    }
}
inline uint8_t rxPop() {
    uint8_t s = rxRing[rxTail];
    rxTail = (uint16_t)((rxTail + 1) & RX_MASK);
    return s;
}

// --- TX accumulator + DSP state -------------------------------------------
AudioPayload txAccum;
uint8_t txIdx = 0;
uint16_t txSeq = 0;
int32_t dcOffset = 0;

uint32_t g_txPayloads = 0;
uint32_t g_rxPayloads = 0;
uint32_t nextSampleUs = 0;
int32_t i2sBuf[64];

void initMic() {
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate = AUDIO_SAMPLE_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    // Stereo: the mic drives slot 0. ONLY_LEFT returns silence on this board.
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

// TALK: read whatever the mic DMA has (non-blocking), take slot 0, DC-remove +
// gain + 8-bit, accumulate into an AudioPayload and hand full ones to the FEC.
void capture() {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, i2sBuf, sizeof(i2sBuf), &bytesRead, 0);  // timeout 0
    uint32_t count = bytesRead / sizeof(int32_t);
    for (uint32_t i = 0; i < count; i += 2) {  // slot 0 = even samples
        int32_t sample24 = i2sBuf[i] >> 8;
        dcOffset = (dcOffset * 63 + sample24) / 64;
        int32_t scaled = ((sample24 - dcOffset) * AUDIO_GAIN) >> 16;
        int32_t v = 128 + scaled;
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        txAccum.samples[txIdx++] = (uint8_t)v;
        if (txIdx >= AUDIO_PACKET_SAMPLES) {
            txAccum.seq = txSeq++;
            g_xcvr->audioTx(txAccum);  // CRC + XOR block (+ parity per block)
            txIdx = 0;
            g_txPayloads++;
        }
    }
}

// LISTEN: drain FEC-recovered payloads into the ring, emit one DAC sample per
// 1/AUDIO_SAMPLE_RATE second (micros-paced; silence on underrun).
void playback() {
    AudioPayload p;
    while (g_xcvr->audioRx(p)) {
        for (uint8_t i = 0; i < AUDIO_PACKET_SAMPLES; i++) rxPush(p.samples[i]);
        g_rxPayloads++;
    }
    if ((int32_t)(micros() - nextSampleUs) >= 0) {
        nextSampleUs += 1000000UL / AUDIO_SAMPLE_RATE;
        if (rxCount() > 0)
            dacWrite(DAC_OUT_PIN, rxPop());
        else
            dacWrite(DAC_OUT_PIN, 128);
    }
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
    if (g_talking)
        capture();
    else
        playback();
}

uint32_t txPayloads() { return g_txPayloads; }
uint32_t rxPayloads() { return g_rxPayloads; }

// Pure mic monitor for bring-up — no radio, no sync, no FEC. Reads the mic in
// stereo and tracks each slot's signal span separately: whichever slot swings
// when you talk is the live one (slot 0 on our boards). It also plays slot 0 to
// the DAC so you can hear the mic. Diagnosing a friend's mic:
//   - SLOT0 span jumps when you talk, SLOT1 ~0  -> mic OK (expected).
//   - BOTH stay ~0                              -> no data: check SD wire, 3V3,
//                                                  and that L/R is tied to GND.
//   - only SLOT1 swings                         -> mic is on the other slot.
void micProbe() {
    initMic();
    Serial.println();
    Serial.println(F("=== MIC PROBE (talk into the mic) ==="));
    Serial.println(F("SLOT0 span should jump when you talk; SLOT1 stays ~0."));
    Serial.println(F("If BOTH stay near 0 -> no mic data (SD wire / 3V3 / L-R)."));

    int32_t buf[64];
    const uint32_t periodUs = 1000000UL / AUDIO_SAMPLE_RATE;
    int32_t s0Min = 0x7FFFFFFF, s0Max = -0x7FFFFFFF;
    int32_t s1Min = 0x7FFFFFFF, s1Max = -0x7FFFFFFF;
    uint32_t pairCount = 0;

    while (true) {
        size_t bytesRead = 0;
        i2s_read(I2S_PORT, buf, sizeof(buf), &bytesRead, portMAX_DELAY);
        uint32_t count = bytesRead / sizeof(int32_t);
        for (uint32_t i = 0; i < count; i++) {
            int32_t sample24 = buf[i] >> 8;
            if (i & 1) {  // slot 1 = odd samples
                if (sample24 < s1Min) s1Min = sample24;
                if (sample24 > s1Max) s1Max = sample24;
            } else {  // slot 0 = even samples (the live mic channel)
                if (sample24 < s0Min) s0Min = sample24;
                if (sample24 > s0Max) s0Max = sample24;
                int32_t v = 128 + (sample24 >> 16);  // monitor slot0 on speaker
                if (v < 0) v = 0;
                if (v > 255) v = 255;
                dacWrite(DAC_OUT_PIN, (uint8_t)v);
                delayMicroseconds(periodUs);  // 8 kHz pacing
                if (++pairCount >= 4000) {     // ~0.5 s @ 8 kHz
                    Serial.printf("SLOT0 span=%ld | SLOT1 span=%ld\n",
                                  (long)(s0Max - s0Min), (long)(s1Max - s1Min));
                    s0Min = s1Min = 0x7FFFFFFF;
                    s0Max = s1Max = -0x7FFFFFFF;
                    pairCount = 0;
                }
            }
        }
    }
}

}  // namespace audio
