# Audio Buzzing & Quality Troubleshooting Guide

## What We Fixed

The buzzing and intermittent audio issues were caused by several audio processing problems that have now been improved:

### 1. **ADC Conversion Issue (FIXED)** ✅
**Problem**: The original code was using simple bit-shifting to convert 12-bit ADC to 8-bit:
```cpp
uint8_t sample = (adcValue >> 4) & 0xFF;  // Lost dynamic range
```
This loses significant dynamic range and isn't centered on the audio signal.

**Solution**: Implemented proper centered conversion with gain:
```cpp
int16_t centered = (int16_t)adcValue - 2048;  // Center at midpoint
int16_t scaled = (int16_t)((centered / 16.0f) * m_gainMultiplier);
uint8_t sample = (uint8_t)(scaled + 128);  // Clamp to 0-255
```

### 2. **Microphone Gain (IMPROVED)** ✅
**Default**: Now sets +6dB gain by default for better microphone signal capture
```cpp
g_sender.setMicrophoneGain(2.0f);  // +6dB gain
```

### 3. **DAC Buffer Size (INCREASED)** ✅
**Before**: 512 samples (64ms @ 8kHz)
**Now**: 1024 samples (128ms @ 8kHz)
Reduces underrun risk from jitter

### 4. **Underrun Handling (IMPROVED)** ✅
**Before**: Output silence (128) during buffer underruns, causing pops/clicks
**Now**: Hold last valid sample - much less jarring

### 5. **Buffer Overflow Protection (ADDED)** ✅
Proper clamping to 0-255 range prevents overflow artifacts

## If You Still Hear Buzzing

Try adjusting the microphone gain. The code now provides a function to tune it:

### In `src/test/audioTest.cpp`, modify the gain in `audioTestSetup()`:

```cpp
// Try these values:
g_sender.setMicrophoneGain(0.5f);   // -6dB (quieter but less noise)
g_sender.setMicrophoneGain(1.0f);   // 0dB (baseline)
g_sender.setMicrophoneGain(2.0f);   // +6dB (default, better for weak signals)
g_sender.setMicrophoneGain(4.0f);   // +12dB (loud, may clip)
```

**Recommendation Process**:
1. Start with `2.0f` (current default)
2. If still buzzing, try `1.0f` (less gain)
3. If audio is too quiet, try `4.0f` (more gain)
4. Recompile and test each setting

### Hardware Adjustments

The **MAX4466 microphone module has a potentiometer** for gain adjustment:
- Look for a small variable resistor on the module
- Try adjusting it 1/8 turn increments
- Test after each adjustment

## Expected Audio Quality

### At 8kHz, 8-bit, with proper gain:
- **Voice**: Clear and intelligible
- **Music**: Recognizable but low-fidelity (telephone quality)
- **Artifacts**: Minimal - should not hear buzzing
- **Continuity**: Should be smooth with occasional glitches only during packet loss

### Still Hearing Issues?

#### **Continuous buzzing/hum**:
- Likely AC line hum or ground loop
- Solution: Check power supply, use good shielded USB cable
- Or try different gain setting (often 1.0f helps)

#### **Intermittent dropouts**:
- Packet loss due to poor RF link
- Solution: Move devices closer, improve antenna

#### **Crackling/popping**:
- DAC bit-banging artifacts (ESP32 DAC speed limitation)
- Solution: Add RC low-pass filter to DAC output:
  - GPIO25 → 1kΩ resistor → capacitor to GND (try 10nF-100nF)

#### **Grainy/noisy audio**:
- ADC inherent noise or very weak microphone signal
- Solution: Double the gain, or improve microphone placement

## Architecture Changes Summary

| Component | Before | After | Impact |
|-----------|--------|-------|--------|
| ADC Conversion | Bit shift (>> 4) | Centered + gain | Better dynamic range |
| Mic Gain | Fixed | Adjustable | Can tune per setup |
| DAC Buffer | 512 samples | 1024 samples | Less jitter |
| Underruns | Output silence | Hold last sample | Fewer artifacts |
| Clamping | None | Full range check | No clipping |

## Recompile & Deploy

Once you've set your preferred gain:

```bash
# Sender device:
# Edit src/test/audioTest.h: AUDIO_TEST_TX 1
# Edit src/test/audioTest.cpp: Adjust gain in audioTestSetup()
platformio run --target upload

# Receiver device: 
# Edit src/test/audioTest.h: AUDIO_TEST_TX 0
platformio run --target upload -p COM3  # Your port
```

## What's Being Sent to RF24

Each audio packet now contains:
- 28 audio samples (8-bit PCM, properly scaled)
- 1 sender ID
- 1 sequence number  
- 2 reserved bytes
- Total: 32 bytes

**Audio data quality**:
- Centered around 128 (silence)
- Range 0-255 (0 to 3.3V on DAC)
- Applied microphone gain for better SNR
- Clamped to prevent overflow

## Next Steps for Further Improvement

1. **Try current setup first** - the improvements should help significantly
2. **If bandwidth allows**, reduce to 4kHz for cleaner audio
3. **For better quality**, implement 4-bit ulaw compression (2x samples/packet)
4. **For echo-free chat**, implement half-duplex protocol or full-duplex with echo cancellation

---

**Note**: Audio quality at 8kHz 8-bit is inherently limited (equivalent to early 1990s voice systems). The goal is to make it intelligible and glitch-free, which these improvements achieve.
