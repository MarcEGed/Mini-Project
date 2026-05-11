#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/dac.h>
#include <debug/debug.h>
#include "config.h"
#include "transceiver.h"

#define MSG_TYPE_CHAT  0x01
#define MSG_TYPE_AUDIO 0x02

struct AudioPacket {
    uint8_t  type;                          // MSG_TYPE_AUDIO
    uint8_t  sender_id;
    uint16_t seq;
    uint8_t  samples[AUDIO_PACKET_SAMPLES]; // 8-bit PCM, unsigned
};

static_assert(sizeof(AudioPacket) == 32, "AudioPacket must be 32 bytes");

class AudioHandler {
public:
    AudioHandler(transceiver* radio) : _radio(radio), _seq(0) {}

    // Call in setup()
    void begin() {
#if AUDIO_ENABLED == 1 || AUDIO_ENABLED == 3
        _initMic();
#endif
#if AUDIO_ENABLED == 2 || AUDIO_ENABLED == 3
        _initDac();
#endif
    }

    // Call every loop() on the TX node
    void txTick() {
#if AUDIO_ENABLED == 1 || AUDIO_ENABLED == 3
        static uint8_t buf[AUDIO_PACKET_SAMPLES];
        static uint8_t idx = 0;

        int32_t raw[8];
        size_t  bytes_read = 0;

        // Non-blocking — returns immediately if DMA buffer empty
        i2s_read(I2S_PORT, raw, sizeof(raw), &bytes_read, 0);

        uint8_t count = bytes_read / sizeof(int32_t);
        for (uint8_t i = 0; i < count && idx < AUDIO_PACKET_SAMPLES; i++) {
            // INMP441: 24-bit left-aligned in 32-bit word
            // >> 8  strips zero-padding → 24-bit signed
            // >> 16 collapses to 8-bit range
            // + 128 shifts signed to unsigned 0–255
            int32_t s = raw[i] >> 8;
            buf[idx++] = (uint8_t)((s >> 16) + 128);
        }

        if (idx >= AUDIO_PACKET_SAMPLES) {
            idx = 0;
            _sendPacket(buf);
        }
#endif
    }

    // Call every loop() on the RX node
    void rxTick() {
#if AUDIO_ENABLED == 2 || AUDIO_ENABLED == 3
        AudioPacket pkt;
        if (!_radio->readAudio(&pkt, sizeof(pkt))) return;
        if (pkt.type != MSG_TYPE_AUDIO) return;

        // Write samples to DAC with correct timing
        uint32_t interval_us = 1000000UL / AUDIO_SAMPLE_RATE;
        for (uint8_t i = 0; i < AUDIO_PACKET_SAMPLES; i++) {
            dacWrite(DAC_OUT_PIN, pkt.samples[i]);
            delayMicroseconds(interval_us);
        }
#endif
    }

private:
    transceiver* _radio;
    uint16_t     _seq;

    void _initMic() {
        i2s_config_t cfg = {
            .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate          = AUDIO_SAMPLE_RATE,
            .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
            .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,  // L/R tied to GND
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count        = 4,
            .dma_buf_len          = 64,
            .use_apll             = false,
            .tx_desc_auto_clear   = false,
            .fixed_mclk           = 0
        };

        i2s_pin_config_t pins = {
            .bck_io_num   = I2S_SCK_PIN,
            .ws_io_num    = I2S_WS_PIN,
            .data_out_num = I2S_PIN_NO_CHANGE,
            .data_in_num  = I2S_SD_PIN
        };

        i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
        i2s_set_pin(I2S_PORT, &pins);
        i2s_zero_dma_buffer(I2S_PORT);
    }

    void _initDac() {
        dac_output_enable(DAC_CHANNEL_2); // GPIO 26 = DAC channel 2
        dac_output_voltage(DAC_CHANNEL_2, 128); // silence = midpoint
    }

    void _sendPacket(uint8_t* samples) {
        // Find min/max to see if signal is moving at all
        uint8_t mn = 255, mx = 0;
        for (uint8_t i = 0; i < AUDIO_PACKET_SAMPLES; i++) {
            if (samples[i] < mn) mn = samples[i];
            if (samples[i] > mx) mx = samples[i];
        }
        //LOG_INFO("Audio TX | min=%d max=%d s0=%d s1=%d s2=%d s3=%d",
                //mn, mx, samples[0], samples[1], samples[2], samples[3]);

        AudioPacket pkt;
        pkt.type      = MSG_TYPE_AUDIO;
        pkt.sender_id = SENDER_ID;
        pkt.seq       = _seq++;
        memcpy(pkt.samples, samples, AUDIO_PACKET_SAMPLES);

        _radio->setMode(TRANSMIT);
        _radio->write(&pkt, sizeof(pkt));
        _radio->setMode(RECEIVE);
    }
};

#endif // AUDIO_H