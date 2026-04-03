# Quick Start: Audio Streaming Setup

## Step 1: Hardware Setup

### Sender Device
1. Connect MAX4466 microphone module:
   - GND → ESP32 GND
   - VCC → ESP32 3.3V
   - OUT → GPIO 34 (ADC0)

2. RF24 module stays as configured (CE=GPIO4, CSN=GPIO5)

### Receiver Device
1. Connect speaker/DAC output:
   - GPIO 25 → 1kΩ resistor → Speaker (or audio amplifier)
   - Speaker other end → GND

2. RF24 module (same as sender)

## Step 2: Software Configuration

### Sender Device (`src/test/audioTest.h`)
```cpp
#define AUDIO_TEST_TX 1          // Enable TX mode
#define AUDIO_MIC_PIN 34         // Microphone on GPIO 34
#define AUDIO_DAC_PIN 25         // (Not used on sender)
#define AUDIO_SAMPLE_RATE 8000   // 8kHz sample rate
```

### Receiver Device (`src/test/audioTest.h`)
```cpp
#define AUDIO_TEST_TX 0          // Enable RX mode
#define AUDIO_MIC_PIN 34         // (Not used on receiver)
#define AUDIO_DAC_PIN 25         // Speaker on GPIO 25
#define AUDIO_SAMPLE_RATE 8000   // Same rate as sender
```

## Step 3: Enable Audio Test in main.cpp

In `src/main.cpp`, uncomment the audio test and comment out RF test:

```cpp
void setup(){
    xcvr.setup();
    xcvr.setMode(RECEIVE);
    setupDisplay();
    
    // Comment out:
    // rfTestSetup(xcvr);
    
    // Uncomment:
    audioTestSetup();
}

void loop(){
    // Comment out:
    // rfTestLoop(xcvr);
    
    // Uncomment:
    audioTestLoop();
}
```

## Step 4: Compile & Flash

### Sender
1. Edit `audioTest.h`: Set `AUDIO_TEST_TX 1`
2. Build: `platformio run`
3. Upload: `platformio run --target upload`
4. Monitor: `platformio device monitor`

### Receiver
1. Edit `audioTest.h`: Set `AUDIO_TEST_TX 0`
2. Build: `platformio run --target clean` then `platformio run`
3. Upload to different board: `platformio run --target upload --upload-port COM3` (or your port)

## Step 5: Test

1. **Power both devices**
2. **Monitor OLED displays:**
   - Sender: Should show "Packets: N" incrementing
   - Receiver: Should show "Packets: N" incrementing
3. **Listen to receiver speaker:**
   - Should hear audio from sender's microphone
   - Audio may be low fidelity at 8kHz but should be recognizable

## Troubleshooting

**No packets shown on display?**
- Check serial output for errors: `Baud: 9600`
- Verify RF24 pins (CE=4, CSN=5)
- Ensure devices are physically close (<1 meter)

**Packets received but no audio?**
- Check speaker wiring (GPIO 25 → resistor → speaker)
- Try adjusting microphone sensitivity (MAX4466 has adjustment pot)
- Increase microphone volume/gain

**Crackling/distorted audio?**
- Decrease sample volume or microphone gain
- Ensure good RF link (many packets, low sequence errors)
- Move devices closer together

**Serial debug output?**
```
COM9 @ 9600 baud (set in config.h: DEBUG_BAUD)
Shows: LOG_INFO, LOG_WARN, LOG_ERROR messages
```

## Performance Expectations

- **Latency**: ~50-100ms (acceptable for audio chat)
- **Quality**: ~8khz mono, 8-bit (telephone quality)
- **Range**: 50-100m line-of-sight (depending on RF environment)
- **Packet Loss**: <5% under good conditions

## Next: Going Bi-directional

To make this truly bi-directional (both devices can send and receive):

1. Each device needs both sender and receiver running
2. Requires time-division multiplexing or half-duplex protocol:
   ```cpp
   // Simple half-duplex: alternate send/receive each loop cycle
   if (cycleCount % 2 == 0) {
       audioSender.update();
   } else {
       audioReceiver.update();
   }
   ```

3. Or use separate RF24 channels for each direction

## Production Improvements

- [ ] Add noise filtering/AGC to microphone input
- [ ] Implement echo cancellation for full-duplex
- [ ] Add audio compression (4:1 or better)
- [ ] Packet loss recovery/FEC
- [ ] Lower latency: tune DMA/buffering
- [ ] Multi-channel support

See `AUDIO_STREAMING_GUIDE.md` for detailed architecture and extension options.
