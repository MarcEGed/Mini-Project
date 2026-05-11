#include "audio.h"

#include <Arduino.h>
#include <config.h>
#include <transceiver.h>
#include "driver/dac.h"
#include "driver/i2s.h"

static transceiver* audioXcvr = nullptr;

static constexpr uint32_t AUDIO_RING_MASK = AUDIO_RING_SIZE - 1;

static volatile uint8_t  audioRing[AUDIO_RING_SIZE];
static volatile uint32_t ringHead      = 0;
static volatile uint32_t ringTail      = 0;
static uint32_t          lastDacTickUs = 0;

static int32_t micI2sBuffer[AUDIO_I2S_DMA_BUF_LEN];
static int32_t micDcOffset = 0;
static uint8_t txPacket[AUDIO_PACKET_SIZE];
static uint8_t txSeq       = 0;
static uint8_t txPacketIdx = AUDIO_PACKET_HEADER_BYTES;

static bool      audioRunning = false;
static audioMode activeMode   = AUDIO_MODE_RX;

static void setupMicI2S() {
    i2s_config_t cfg = {};
    cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate = AUDIO_SAMPLE_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.dma_buf_count = AUDIO_I2S_DMA_BUF_COUNT;
    cfg.dma_buf_len = AUDIO_I2S_DMA_BUF_LEN;

    i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);

    i2s_pin_config_t pins = {};
    pins.bck_io_num = AUDIO_I2S_SCK_PIN;
    pins.ws_io_num = AUDIO_I2S_WS_PIN;
    pins.data_in_num = AUDIO_I2S_SD_PIN;
    pins.data_out_num = I2S_PIN_NO_CHANGE;

    i2s_set_pin(I2S_NUM_0, &pins);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

static void pollAudioRxRadio() {
    if (audioXcvr == nullptr) {
        return;
    }

    uint8_t packet[AUDIO_PACKET_SIZE];
    while (audioXcvr->read(packet, sizeof(packet))) {
        for (uint8_t i = AUDIO_PACKET_HEADER_BYTES; i < sizeof(packet); i++) {
            const uint32_t nextHead = (ringHead + 1) & AUDIO_RING_MASK;
            if (nextHead == ringTail) {
                break; // Ring buffer full, drop remaining packet bytes.
            }
            audioRing[ringHead] = packet[i];
            ringHead = nextHead;
        }
    }
}

static void pollAudioRxDac() {
    const uint32_t periodUs = 1000000UL / AUDIO_SAMPLE_RATE;
    const uint32_t nowUs = micros();

    if ((nowUs - lastDacTickUs) < periodUs) {
        return;
    }
    lastDacTickUs = nowUs;

    if (ringTail != ringHead) {
        dac_output_voltage(AUDIO_DAC_CHANNEL, audioRing[ringTail]);
        ringTail = (ringTail + 1) & AUDIO_RING_MASK;
    }
}

static void pollAudioTxMicAndSend() {
    if (audioXcvr == nullptr) {
        return;
    }

    size_t bytesRead = 0;
    i2s_read(I2S_NUM_0, micI2sBuffer, sizeof(micI2sBuffer), &bytesRead, 0);
    if (bytesRead == 0) {
        return;
    }

    const uint32_t sampleCount = bytesRead / sizeof(int32_t);
    for (uint32_t i = 0; i < sampleCount; i++) {
        const int32_t sample24 = micI2sBuffer[i] >> 8;

        micDcOffset = (micDcOffset * AUDIO_DC_FILTER_ALPHA + sample24) / AUDIO_DC_FILTER_DIV;
        const int32_t centered = sample24 - micDcOffset;

        const int32_t scaled = (centered * AUDIO_MIC_GAIN) >> 16;
        int32_t       val = AUDIO_DAC_IDLE_VALUE + scaled;
        if (val > 255) {
            val = 255;
        } else if (val < 0) {
            val = 0;
        }

        txPacket[txPacketIdx++] = static_cast<uint8_t>(val);
        if (txPacketIdx == sizeof(txPacket)) {
            txPacket[0] = txSeq++;
            audioXcvr->write(txPacket, sizeof(txPacket));
            txPacketIdx = AUDIO_PACKET_HEADER_BYTES;
            delayMicroseconds(AUDIO_TX_PACING_US);
        }
    }
}

void audioSetup(transceiver& xcvr, audioMode mode) {
    audioStop();

    audioXcvr = &xcvr;
    activeMode = mode;

    if (mode == AUDIO_MODE_RX) {
        ringHead = 0;
        ringTail = 0;
        lastDacTickUs = micros();

        audioXcvr->setMode(RECEIVE);

        dac_output_enable(AUDIO_DAC_CHANNEL); // GPIO25
        dac_output_voltage(AUDIO_DAC_CHANNEL, AUDIO_DAC_IDLE_VALUE);
    } else {
        txSeq = 0;
        txPacketIdx = AUDIO_PACKET_HEADER_BYTES;
        micDcOffset = 0;

        setupMicI2S();
        audioXcvr->setMode(TRANSMIT);
    }

    audioRunning = true;
}

void audioLoop() {
    if (!audioRunning) {
        return;
    }

    if (activeMode == AUDIO_MODE_RX) {
        pollAudioRxRadio();
        pollAudioRxDac();
        return;
    }

    pollAudioTxMicAndSend();
}

void audioStop() {
    if (!audioRunning) {
        return;
    }

    if (activeMode == AUDIO_MODE_TX) {
        i2s_driver_uninstall(I2S_NUM_0);
    }

    if (audioXcvr != nullptr) {
        audioXcvr->setMode(RECEIVE);
    }

    audioRunning = false;
}

bool audioIsRunning() {
    return audioRunning;
}
