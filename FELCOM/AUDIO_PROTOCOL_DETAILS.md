# Audio Packet Format & Protocol Details

## RF24 Packet Structure

### AudioPacket (32 bytes)
```
Byte:  0         1         2-3       4-31
      +-------+-------+-------+-------+-------+-------+---...---+
Field:|sendId |seqNum |resrvd |     audioData[28]              |
Type: |uint8_t|uint8_t|uint16_t|     uint8_t[28]               |
      +-------+-------+-------+-------+-------+-------+---...---+
      |<----- Packet Header (4B) ---->|<--- Audio Payload (28B) --->|
```

### Byte-by-Byte Breakdown
| Byte | Name | Type | Range | Purpose |
|------|------|------|-------|---------|
| 0 | senderId | uint8_t | 0x00-0xFF | Identifies transmitting device |
| 1 | sequenceNum | uint8_t | 0x00-0xFF | Packet counter (wraps at 255→0) |
| 2-3 | reserved | uint16_t | 0x0000 | Alignment/future use |
| 4-31 | audioData | uint8_t[28] | 0x00-0xFF | Raw audio samples |

## Audio Sample Encoding

### 8-bit PCM (Current Implementation)
```
Sample Value  DAC Output
0x00 (0)      0.0V (GND)
0x80 (128)    1.65V (midpoint silence)
0xFF (255)    3.3V (max)

Linear mapping: V_out = (sample_value / 255) × 3.3V
```

### ADC to Sample Conversion
```
ADC Value (12-bit)  Sample Value (8-bit)
0x000 (0)           0x00 (0)
0x800 (2048)        0x80 (128)
0xFFF (4095)        0xFF (255)

Formula: sample_8bit = (adc_12bit >> 4) & 0xFF
```

## Transmission Protocol

### Sender → Receiver Flow
```
[Sender]                          [Receiver]
  |                                  |
  1. ADC acquires 128 samples        |
     (16ms @ 8kHz)                   |
  |                                  |
  2. Pack into AudioPacket           |
     - Copy 28-byte chunks           |
     - Set sequenceNum++             |
  |                                  |
  3. RF24 transmit                   |
     (1-2ms transmission)            |
  |                                  |
  4. Switch RF to RX                 |
  |                                  |
                                     5. RF24 RX interrupt
                                        (packet in FIFO)
                                     |
                                     6. audioReceiver::update()
                                        reads packet
                                     |
                                     7. Extract 28 samples
                                        accumulate to 128-sample
                                        playback buffer
                                     |
                                     8. Buffer ready
                                        → playAudioSamples()
                                     |
                                     9. DAC output at 8kHz timing
```

### Timing Characteristics

**Sender Side (TX Mode)**
```
ADC Sample:        Every 125 µs (1/8000 Hz)
Buffer Fill:       128 samples × 125 µs = 16 ms
Packet TX:         Every 16 ms (when buffer full)
TX Duration:       ~1-2 ms per 32-byte packet
Duty Cycle:        ~10-15% transmitting

Timeline:
  0ms    ├─ ADC sample 0
  0.125  ├─ ADC sample 1
  ...    
  16ms   └─ Buffer full → Transmit packet
         ├─ RF24 transmit (1-2ms)
         └─ Switch to RX
  17ms   ├─ ADC sample 128 (start new buffer)
  ...
  32ms   └─ Buffer full → Transmit packet 2
```

**Receiver Side (RX Mode)**
```
Packet RX:         ~16 ms intervals (from sender)
Sample Unpack:     28 samples × 4-5 packets = 112-140 samples
Playback Buffer:   128 samples
DAC Output:        Every 125 µs
Latency:           ~50-100 ms (4-5 packet buffering)

Timeline:
  0ms    ├─ Packet 1 RX → 28 samples added
  1ms    │
  16ms   ├─ Packet 2 RX → 56 samples received
  17ms   │
  32ms   ├─ Packet 3 RX → 84 samples received
  33ms   │
  48ms   ├─ Packet 4 RX → 112 samples received
  49ms   │
  60ms   └─ Playback buffer ready (128 samples) → Start outputting
         ├─ DAC output sample 0
  60.125 ├─ DAC output sample 1
  ...
  76ms   └─ DAC output sample 127 (buffer done)
```

## Sequence Number Usage

### Wraparound Handling
```
Valid Sequence:  0 → 1 → 2 → ... → 254 → 255 → 0 → 1 → ...

Packet Loss Detection:
- Last received: seqNum=10
- Current packet: seqNum=12
- → Detected loss of packet 11
- → Log: "Sequence error: expected 11, got 12"

Receiver Logic:
  if (m_hasLastSeq) {
      uint8_t expectedSeq = m_lastSequenceNum + 1;
      if (packet.sequenceNum != expectedSeq) {
          if (!(packet.sequenceNum == 0 && m_lastSequenceNum == 255)) {
              // Not a wraparound, log error
              m_sequenceErrors++;
          }
      }
  }
```

### Statistics from Sequence Numbers

With packet interval ~16ms:
- Sequence errors per second ≈ (packet loss %) / (16ms period)
- Example: 5% loss @ 8kHz
  - 1 packet per 4 received = ~25% loss expected
  - This would show as seqError rate ~60/sec

## RF24 Configuration Impact

### Current Settings (from transceiver.cpp)
```cpp
Data Rate:     RF24_250KBPS
PA Level:      RF24_PA_MAX
Channel:       125
Payload Size:  32 bytes (sizeof(AudioPacket))
CRC:           Disabled
AutoACK:       Disabled
Retries:       0
```

### Throughput Analysis
```
RF24 @ 250 kbps:
  Theoretical: 250,000 bits/sec ÷ 8 = 31,250 bytes/sec
  
Per 32-byte packet:
  Time = 32 bytes ÷ 31,250 bytes/sec = 1.024 ms

Per 16ms acquisition period (sending once):
  1 packet in 1ms out of 16ms = 6.25% RF duty cycle

Audio bandwidth used:
  - Sender TX: 1-2ms per 16ms interval = 28 bytes per 16ms
  - Effective: 28 bytes/16ms = 1,750 bytes/sec = 14 kbps
  - Left for overhead: 30,000 - 1,750 = 28 kbps margin
```

## Error Detection & Logging

### Sender Errors
```cpp
getPacketsSent()        // Total successful transmissions
getAcquisitionErrors()  // Failed ADC buffer reads
getTransmitErrors()     // RF24 write failures
```

### Receiver Errors
```cpp
getPacketsReceived()    // Total RF24 receptions
getSequenceErrors()     // Out-of-order or dropped packets
getCorruptedPackets()   // Reserved for future CRC checks
```

## Alternative Formats (Future)

### 16-bit Samples (Higher Quality)
```
Packet: 32 bytes
Header: 4 bytes
Audio:  28 bytes → 14 × 16-bit samples
Time:   14 samples / 8000 Hz = 1.75 ms per packet
Quality: Higher fidelity, but requires resampling or lower rate
```

### Compressed ADPCM (Lower Bandwidth)
```
4-bit compression: 2 samples per byte
Packet: 32 bytes
Audio: 28 bytes → 56 samples (7ms buffer)
Compression: 4:1 ratio
Tradeoff: CPU load vs bandwidth
```

### With Timestamps
```
Enable packet timing for sync:
  - Add 4B timestamp from sender
  - Receiver can synchronize jitter-affected playback
Currently: Implicit timing from packet rate
```

## Testing & Verification

### Monitor Serial Output (9600 baud)
```
Sender:          Receiver:
LOG_INFO "Packet received"
→ Check consistency

Transmission sequence:
  T+0ms    Packet 0 sent
  T+16ms   Packet 1 sent
  T+32ms   Packet 2 sent
```

### Check OLED Display
```
Sender shows:        Receiver shows:
Packets: 45          Packets: 44-45
Acq Err: 0           Seq Err: 0
TX Err: 0            Bad: 0
```

If sender packets >> receiver packets, RF link issue.
If sequence errors > 5%, poor RF signal.

## Debug Packet Format

For debugging, you can log packet contents:
```cpp
// In audioReceiver.cpp
void logPacket(const AudioPacket& pkt) {
    LOG_INFO("PKT: seq=%u, id=%u, samples=[%u,%u,%u...]",
             pkt.sequenceNum, pkt.senderId,
             pkt.audioData[0], pkt.audioData[1], pkt.audioData[2]);
}
```

This shows first 3 samples (audio levels) to verify signal presence.
