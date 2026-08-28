# FELCOM Audio + FEC — What's New

This documents the **real-time voice** feature and the **FEC layer** it rides on,
added on top of the FHSS transceiver. If you just want to use it, jump to
[How to use it](#how-to-use-it). If something doesn't work, read
[Hardware & gotchas](#hardware--gotchas) first — most issues are wiring/sync.

---

## TL;DR

- New **AUDIO** screen in the menu (between `SYNC TEST` and `ABOUT`): a
  half-duplex walkie-talkie. One node **TALKS**, the other **LISTENS**; press
  **SELECT** to toggle role, **BACK** to exit.
- Audio is **8 kHz / 8-bit mono PCM**, captured from the INMP441 mic, sent over
  the existing radio, and played out the ESP32 DAC → amp → speaker.
- It is **non-blocking** (no FreeRTOS tasks) — capture/transmit/receive/playback
  are cooperative steps pumped from `loop()`, so FHSS hopping still works.
- It rides the transceiver's **FEC**: every audio packet is CRC-protected and
  carries XOR parity so one lost packet per block of 4 can be reconstructed.

---

## How to use it

Both boards run the **same firmware** (`env:esp32dev`). Flash each:

```
pio run -e esp32dev -t upload --upload-port COM3
pio run -e esp32dev -t upload --upload-port COM4
```

1. **Sync the two boards first.** Audio rides FHSS, so the nodes must be on the
   same channel-hopping schedule (exactly like chat). On *both* boards open
   **SYNC TEST** and press **SYNC**.
2. On both boards go **back to the menu → AUDIO**.
3. On one board press **SELECT** → it shows **TALKING**. Leave the other on
   **LISTENING**.
4. Talk into the mic; you hear it on the listener's speaker.

> If you hear nothing, the boards have most likely **drifted out of sync**
> (clock drift, same limitation as chat) — re-run SYNC. See gotchas below.

---

## Hardware & gotchas

### Wiring (per board)
| INMP441 mic | ESP32 |
| ----------- | ----- |
| SCK         | GPIO13 |
| WS          | GPIO16 |
| SD          | GPIO17 |
| L/R         | **GND** (selects the left slot) |
| VDD / GND   | 3.3 V / GND |

| Audio out | ESP32 |
| --------- | ----- |
| DAC → amp (LM386) input | **GPIO25** (DAC1) |

These pins live in `include/config.h` (`I2S_*_PIN`, `DAC_OUT_PIN`).

### The two gotchas that cost us the most time
1. **`I2S_CHANNEL_FMT_ONLY_LEFT` returns silence on our DevKit boards.** This is
   a known ESP32 *legacy* I2S quirk. We capture in **stereo
   (`RIGHT_LEFT`) and use `AUDIO_I2S_SLOT_INDEX`** to select the DMA slot. If the
   mic reads all zeros, this is almost always why.
2. **DAC is on GPIO25, not 26.** The amp is wired to DAC1 (GPIO25).

### Other things to know
- **Sync is required.** No sync ⇒ the two nodes hop to different channels ⇒ no
  audio. Clock drift will desync them over time; just re-SYNC.
- **Half-duplex.** Only one node should be TALKING at a time; if both talk,
  neither listens.
- **Quality.** It's 8 kHz / 8-bit voice — fine for intelligible speech, not
  music. Expect some hiss and occasional clicks at channel-hop boundaries.

---

## Architecture

```
loop():
  hop channel (FHSS, from the 500 ms counter)        ── unchanged
  AUDIO screen tick → audio::update()                ── non-blocking
        │
   TALK │                              LISTEN │
        ▼                                     ▼
  i2s_read(timeout 0)  → DC-removal + gain   xcvr.audioRx()  ← CRC + XOR recover
        → 8-bit → accumulate 24 samples       → RX ring (4096)
        → xcvr.audioTx(AudioPayload)          → micros-paced 8 kHz → DAC (GPIO25)
              (CRC + XOR block FEC + CSMA)
```

- **Capture** (`audio.cpp`): non-blocking `i2s_read` (timeout 0) so the loop
  never stalls; DC-offset EMA `(dc*63 + s)/64`, gain `(centered*AUDIO_GAIN)>>16`,
  clamp to 8-bit. 24 samples → one `AudioPayload`.
- **`AudioPayload`** (`include/protocol.h`): `uint16_t seq + uint8_t samples[24]`
  = 26 bytes, which exactly fills the FEC "user region" (the last 2 of the
  28 data bytes hold the CRC).
- **Playback**: `xcvr.audioRx()` returns FEC-recovered payloads; samples go into
  a ring buffer that is drained to the DAC at exactly 8 kHz using `micros()`
  pacing (no extra hardware timer — the FHSS counter uses one already).
- **FHSS**: audio uses the normal hop logic in `loop()`; nothing audio-specific
  hops.

---

## The FEC layer (what audio rides on)

FEC is **owned by the transceiver** — the app never calls FEC functions
directly, it just uses `xcvr.write` / `writeReliable` / `read` / `audioTx` /
`audioRx`. Three complementary mechanisms, applied per packet type:

| Packet type | CRC16 | ARQ (retransmit) | XOR block | Why |
| ----------- | :---: | :--------------: | :-------: | --- |
| CHAT        | ✓ | ✓ | ✗ | infrequent, must be exact |
| ND_SYNC     | ✗ (raw) | ✗ | ✗ | timing broadcast; must stay forgiving |
| PONG        | ✓ | ✗ | ✗ | frequent; ARQ would backlog |
| AUDIO       | ✓ | ✗ | ✓ | real-time; forward recovery only; bypasses CSMA backoff |
| TEST (BER)  | ✗ (raw) | ✗ | ✗ | measures real bit errors |
| ACK         | ✓ | ✗ | ✗ | small control packet |

- **CRC16-CCITT** lives in the last 2 of the 28 data bytes (the air frame is
  unchanged at 32 bytes). The 12-bit `fec` header field carries metadata only
  (scheme / XOR block id / slot index), not the CRC.
- **XOR block FEC** (audio): 4 data packets + 1 parity packet; recovers exactly
  one lost packet per block.
- **ARQ** (chat): blocks waiting for an ACK and retransmits; not used for audio
  (too much latency).

See `lib/fec/` for the implementation and `lib/transceiver/transceiver.h` for
the public API.

---

## Tuning & debugging

- **Mic slot:** `AUDIO_I2S_SLOT_INDEX` in `include/config.h` selects which
  stereo DMA slot is treated as the INMP441 sample. With L/R tied to GND the
  mic should be on the left slot, but ESP32 legacy I2S can expose that as
  buffer index `0` or `1` depending on board/framework behavior. If diagnostics
  show `mic` increasing but `peak` near zero, try the other slot before changing
  gain.
- **Mic volume:** `AUDIO_GAIN` in `include/config.h` (currently `18`).
  Higher = louder but more clipping/hiss; lower = cleaner but quieter.
  This is a *capture-side* setting, so only the **talking** board needs
  reflashing when you change it. If `clip` rises quickly while talking, reduce
  `AUDIO_GAIN`.
- **Local mic monitor:** `AUDIO_LOCAL_MONITOR` in `include/config.h` plays the
  same 8-bit samples being transmitted out the local DAC while TALKING. This is
  useful for separating mic/I2S/gain problems from radio/FEC problems. If local
  monitor is still choppy, temporarily set `AUDIO_RADIO_TX_ENABLE` to `0`; that
  tests only mic → DSP → DAC without nRF24/FEC transmit stalls. If sidetone has
  too much delay, lower `AUDIO_LOCAL_MONITOR_MAX_QUEUED`; if it sounds too
  rough, raise it a bit.
- **Serial debug:** set `AUDIO_SERIAL_DEBUG` to `1` in `include/config.h` to
  print every 2 s: role, TX/RX payload counts, I2S read activity, mic sample
  count/peak/clips, RX ring level, underruns/overruns, and FEC counters
  (`CRC ok/bad`, `AUDIO blk/rec/lost`). Keep it `0` for normal playback because
  serial printing stalls the cooperative audio loop and causes audible skips.
  - `i2s` first/second number = non-empty reads / total reads. If total rises
    but non-empty stays low, the loop is polling faster than DMA fills; this is
    usually OK. If both stay at 0 while TALKING, capture is not running.
  - `mic` should rise while TALKING; `peak` should move when you speak. Near-zero
    `peak` usually means the wrong I2S slot or mic wiring/power.
  - `clip` rising quickly means the 8-bit conversion is saturating; lower
    `AUDIO_GAIN`.
  - `under` rising on LISTEN means playback is starved: radio/FEC is not
    delivering enough samples, sync is bad, or debug/logging is stalling audio.
  - `over` rising means the RX jitter ring is filling faster than playback can
    drain it.
  - `CRC bad` = corrupt packets dropped, `rec` = packets XOR rebuilt,
    `lost` = losses XOR couldn't fix (these become gaps).

---

## Files

**New**
- `lib/adio/audio.h`, `lib/adio/audio.cpp` — the non-blocking audio subsystem.
- `src/ui/AudioUI.h`, `src/ui/AudioUI.cpp` — the AUDIO screen.

**Changed**
- `include/config.h` — audio pins/params (`AUDIO_GAIN`, `AUDIO_RX_RING`, …),
  `DAC_OUT_PIN = 25`.
- `src/main.cpp` — `audio::begin()`, the AUDIO menu entry + mode tick, and the
  opt-in debug line.
- `src/ui/ui.{h,cpp}`, `src/ui/MenuUI.{h,cpp}` — the AUDIO menu item + dispatch.

---

## Known limitations / next steps

- **Skipping at hop boundaries.** Each 500 ms hop can drop the in-flight packet;
  XOR only recovers one loss per block of 4. Options if it's too choppy:
  pin AUDIO mode to a fixed channel, or add a short RX pre-buffer.
- **Sync drift** desyncs the nodes over time — re-SYNC as needed.
- **Half-duplex only**; no push-to-talk hardware button yet (toggle via SELECT).
- **Audio TX bypasses CSMA backoff** to avoid millisecond stalls at the audio
  packet rate. If both nodes transmit audio at once, collisions are expected;
  keep AUDIO half-duplex.
