# Audio Streaming Implementation Guide

## Overview

This document describes the audio streaming system implemented for your FELCOM project. The system allows bidirectional audio transmission between two ESP32 devices via RF24 radio, with microphone input on the sender and speaker output on the receiver.

## Architecture

### Packet Format (`audioMessage.h`)

The audio system uses a simple 32-byte packet structure aligned with RF24's maximum payload size:

```
AudioPacket (32 bytes total):
├── senderId (1 byte)      - Sender identifier
├── sequenceNum (1 byte)   - Packet sequence number for ordering
├── reserved (2 bytes)     - Alignment/future use
└── audioData[28]          - Raw PCM audio samples (8-bit or 16-bit)
```

With 28 bytes of audio data per packet:
- **8-bit samples**: 28 samples per packet
- **16-bit samples**: 14 samples per packet

At 8kHz sample rate with 28-byte payload (28 samples):
- **Time per packet**: 3.5ms
- **Throughput**: ~256 kbps raw (achievable with RF24 at 1Mbps)

## Core Components

### 1. ADC Acquisition Module (`lib/adcAcquisition/`)

Handles microphone input with circular buffering for continuous non-blocking acquisition.

**Key Features:**
- Dual-buffer design: one buffer fills while the other is read
- Software timing-based sampling (not DMA, but can be extended)
- Configurable sample rate (8kHz recommended for typical microphones)
- 8-bit sample resolution (0-255, corresponding to 0-3.3V on ESP32)

**Usage:**
```cpp
ADCBuffer adcBuffer;
adcBuffer.begin(34, 8000);  // ADC pin 34, 8kHz sample rate

while(true) {
    adcBuffer.update();  // Call frequently to acquire samples
    
    if (adcBuffer.isBufferReady()) {
        const uint8_t* samples = adcBuffer.getReadBuffer();
        // Process 128 samples
    }
}
```

**Buffer Details:**
- `BUFFER_SIZE`: 128 samples per buffer
- `NUM_BUFFERS`: 2 (dual buffer)
- Sample timing: interrupt-free, polling-based (meets real-time requirements for 8kHz)

### 2. Audio Sender (`lib/audioSender/`)

Acquires microphone samples and transmits them via RF24.

**Key Features:**
- Integrates ADC acquisition with RF24 transmission
- Automatic packet sequence numbering
- Statistics tracking (packets sent, errors)
- Automatic transceiver mode switching

**Usage:**
```cpp
AudioSender sender;
sender.begin(34, 8000);  // Mic on pin 34, 8kHz

void loop() {
    if (sender.update()) {
        // Packet was successfully sent
    }
}
```

**What It Does:**
1. Acquires audio buffer from ADC
2. Packs samples into AudioPacket structure
3. Transmits via RF24 in TRANSMIT mode
4. Increments sequence number automatically

### 3. Audio Receiver (`lib/audioReceiver/`)

Receives audio packets and buffers them for DAC output.

**Key Features:**
- Receives RF24 packets and extracts audio samples
- Circular playback buffer (128 samples)
- Sequence number tracking to detect packet loss
- Statistics tracking

**Usage:**
```cpp
AudioReceiver receiver;
receiver.begin();

void loop() {
    receiver.update();  // Receive and buffer packets
    
    if (receiver.isPlaybackBufferReady()) {
        const uint8_t* buffer = receiver.getPlaybackBuffer();
        playAudioSamples(buffer, 128);
    }
    
    updateDACStreaming();  // Output DAC samples
}
```

**Buffer Management:**
- Input: 28 samples per received packet
- Output: 128-sample playback buffer
- Packing ratio: ~4.6 packets per playback buffer
- This accumulation reduces underrun risk and buffers network jitter

### 4. DAC Audio Output (Updated `src/dac/`)

Extended the existing DAC module with streaming audio support.

**New Functions:**
- `setupDACStreamingMode(pin, sampleRateHz)` - Initialize streaming mode
- `playAudioSamples(samples, count)` - Supply audio samples
- `updateDACStreaming()` - Output samples at correct timing
- `isDACStreamingDone()` - Check if buffer is empty

**Key Features:**
- 8-bit DAC output (0-255 maps to 0-3.3V on ESP32)
- Circular buffer (512 samples) to handle timing jitter
- Silence output (128) when buffer is empty
- Coexists with original sine-wave generation

**Usage:**
```cpp
setupDACStreamingMode(25, 8000);  // GPIO25 DAC pin, 8kHz

// In loop:
const uint8_t* audioBuffer = ...; // Get from receiver
playAudioSamples(audioBuffer, 128);
updateDACStreaming();  // Call frequently
```

**DAC Pins:**
- GPIO 25: DAC1 (primary)
- GPIO 26: DAC2 (alternative)

## Hardware Wiring

### Sender (Microphone Input)

```
MAX4466 Microphone Module
├── GND  → ESP32 GND
├── VCC  → ESP32 3.3V
└── OUT  → ESP32 GPIO 34 (ADC0)

RF24 Module:
├── GND     → ESP32 GND
├── VCC     → ESP32 3.3V (with 10µF cap)
├── CE      → GPIO 4
├── CSN     → GPIO 5
├── MOSI    → GPIO 23
├── MISO    → GPIO 19
└── SCK     → GPIO 18
```

### Receiver (Speaker Output)

```
DAC Output:
├── GPIO 25 (DAC1) → Speaker AMP Input (or 1kΩ resistor → speaker)

RF24 Module:
├── Same as above (CE=4, CSN=5)
```

## Audio Test Mode (`src/test/audioTest.h/cpp`)

Simple bidirectional audio streaming test that mirrors the RF test pattern.

**Configuration:**
```cpp
#define AUDIO_TEST_TX 1          // 1=sender, 0=receiver
#define AUDIO_MIC_PIN 34         // ADC pin for microphone
#define AUDIO_DAC_PIN 25         // DAC pin for speaker
#define AUDIO_SAMPLE_RATE 8000   // Sample rate in Hz
```

**What to Do:**

1. **Flash Sender:**
   ```cpp
   // In audioTest.h
   #define AUDIO_TEST_TX 1
   // Compile and flash sender device with microphone
   ```

2. **Flash Receiver:**
   ```cpp
   // In audioTest.h
   #define AUDIO_TEST_TX 0
   // Compile and flash receiver device with speaker
   ```

3. **Enable in main.cpp:**
   ```cpp
   // In main.cpp setup()
   audioTestSetup();
   
   // In main.cpp loop()
   audioTestLoop();
   ```

4. **Monitor:**
   - OLED display shows real-time stats
   - Serial debug output shows errors
   - Sender: packets sent, acquisition errors, transmission errors
   - Receiver: packets received, sequence errors, corrupted packets

## Packet Format & RF24 Settings

**RF24 Configuration (unchanged):**
```cpp
dataRate: RF24_250KBPS (configured in transceiver)
channel: 125
payloadSize: 32 bytes (AudioPacket)
CRC: Disabled
AutoAck: Disabled
```

**Why This Works:**

1. **32-byte Payload**: Matches RF24 FIFO (max 32 bytes)
2. **250kbps Rate**: Stable, long-range (best for ~50m+)
3. **No CRC/ACK**: Accepts packet loss for real-time audio (acceptable for streaming)
4. **Sequence Numbers**: Detects dropped packets in receiver

**Approximate Link Performance:**
- At 250kbps: ~32 bytes per frame = ~1ms per packet
- With 28-byte audio data: ~224 kbps actual audio throughput
- This leaves headroom for RF overhead and timing

## Timing & Performance

### Sender Side (ADC → RF24)

```
ADC acquisition:  8000 Hz sample rate → 125 µs per sample
Buffer fill time: 128 samples × 125 µs = 16 ms
Packet interval:  ~16 ms between transmissions
TX time:          ~1-2 ms per 32-byte packet
```

### Receiver Side (RF24 → DAC)

```
Packet interval:        16 ms apart (from sender)
Playback buffer size:   128 samples @ 8kHz = 16 ms
DAC output timing:      125 µs per sample
Buffering strategy:     Accumulates ~4-5 packets before playback
Jitter absorption:      Built-in circular buffer handles +/- timing variance
```

**Real-time Safety:**
- Audio acquisition runs in `update()` at main loop frequency (fast enough for 8kHz)
- DAC output timing controlled by microsecond-precision `micros()`
- No interrupts or RTOS tasks needed; deterministic polling

## Integration Notes

The implementation follows existing project patterns:

1. **Transceiver Integration**: Each module creates its own transceiver instance for mode control
2. **Message Pattern**: AudioPacket mirrors Message struct (packed, 32 bytes, static_assert)
3. **DCL Logging**: All modules use existing debug macros
4. **Display Integration**: Audio test renders to OLED like RF test

## Extending the System

### Adding DMA

For production, enhance ADC with DMA:

```cpp
// Pseudo-code for I2S/DMA:
void setupDMA_ADC() {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_ADC_ONLY,
        .sample_rate = 8000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    };
    // ... configure I2S/DMA for continuous ADC streaming
}
```

### Adding Compression

To achieve lower bitrate:

```cpp
// Simple 4-bit compression (2 samples per byte)
uint8_t compress4bit(uint8_t sample) {
    return (sample >> 4) & 0x0F;  // Store high 4 bits
}

// 2x throughput improvement at cost of quality
```

### Improving DAC Output Quality

For better audio quality:

1. **Add low-pass filter** (RC filter on DAC output)
2. **Use I2S DAC** for higher resolution
3. **Implement compression** for better SNR
4. **Add AGC** for microphone input

## Testing Checklist

- [ ] Both devices compile without errors
- [ ] Sender runs, shows "Packets: N" increasing on display
- [ ] Receiver runs, shows "Packets: N" increasing on display
- [ ] Microphone input produces audio samples (debug serial output)
- [ ] Speaker plays audio from receiver DAC
- [ ] Sequence errors remain low (<5% packet loss)
- [ ] No acquisition errors on sender
- [ ] No transmission errors on sender

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| No packets sent | AUDIO_TEST_TX=0 on sender | Verify #define AUDIO_TEST_TX 1 |
| No audio received | RF24 not communicating | Check CE/CSN pins match config |
| Crackling audio | Buffer underrun | Increase OUTPUT_BUFFER_SIZE in receiver |
| High sequence errors | Poor RF link | Move devices closer or improve antenna |
| Acquisition errors | ADC read timing | Verify GPIO 34 not used elsewhere |
| DAC out of range | Sample values > 255 | Check ADC conversion (should be 8-bit) |

## Files Created

```
include/
  └── audioMessage.h          - AudioPacket structure

lib/
  ├── adcAcquisition/
  │   ├── adcAcquisition.h
  │   └── adcAcquisition.cpp  - ADC buffer management
  ├── audioSender/
  │   ├── audioSender.h
  │   └── audioSender.cpp     - Sender logic
  └── audioReceiver/
      ├── audioReceiver.h
      └── audioReceiver.cpp   - Receiver logic

src/
  ├── dac/
  │   ├── dac.h               - Extended with streaming
  │   └── dac.cpp             - Updated with streaming
  └── test/
      ├── audioTest.h
      └── audioTest.cpp       - Audio test mode

Modified:
  src/main.cpp                - Updated to include audio test
```

## Next Steps

1. **Flash and test**: Follow testing checklist above
2. **Optimize bitrate**: Consider compression or lower sample rate
3. **Implement compression**: 4:1 or adaptive ADPCM for better quality at lower bitrate
4. **Add AEC**: Echo cancellation for bi-directional use
5. **Production hardening**: Error recovery, timeout handling, power management
