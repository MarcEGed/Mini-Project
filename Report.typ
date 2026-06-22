#import "@preview/starry-ulfg:0.2.0": starry-ulfg

#show: starry-ulfg.with(
  document-title: "FELCOM: A Secure Handheld Communication Device",
  candidates: ("Marc Gedeon", "Yorgo Hassabou", "Charbel Assaad"),
  title: "FELCOM: A Secure Handheld Communication Device",
  course: "Mini-Project",
  year: [2025/2026],
  professors: ("Dr. Hadi Jerdek",),
  // acknowledgment: [#lorem(180)],
  show-table-of-contents: false,
)

#show heading.where(level: 1): it => it
#set heading(numbering: "C1.1-", supplement: [Chapter])
#show table: set par(justify: false)

// ===========================================================================
// HELPERS
//
//  #ph(...)   -> a dashed "diagram placeholder" box. Wrap it in a #figure with a
//               caption + label so it shows up in the Table of Figures. When the
//               real diagram is ready, replace `ph[...]` with `image("path.png")`
//               and keep the surrounding #figure / caption / label untouched.
//
//  #todo(...) -> a highlighted note for the team: data still to be measured /
//               inserted before submission. Remove all #todo blocks for the
//               final version.
//
//  Code listings: use a ```cpp ... ``` block inside a #figure(kind: raw,
//  supplement: [Listing], ...). They are auto-numbered as "Listing N" and do NOT
//  appear in the Table of Figures (which only lists images).
// ===========================================================================
#let ph(desc, height: 4.5cm) = box(
  width: 100%,
  height: height,
  inset: 12pt,
  radius: 4pt,
  stroke: (dash: "dashed", thickness: 1pt, paint: luma(150)),
  fill: luma(248),
)[
  #align(center + horizon)[
    #text(fill: luma(110))[#emph[Diagram placeholder (to be drawn)] \ #v(4pt) #desc]
  ]
]

#let todo(body) = block(
  fill: rgb("#fff3cd"),
  inset: 7pt,
  radius: 3pt,
  width: 100%,
)[#text(fill: rgb("#7a5b00"))[*TODO (insert before submission):* #body]]

// Boxed, slightly smaller code blocks.
#show raw.where(block: true): it => block(
  fill: luma(247),
  inset: (x: 9pt, y: 7pt),
  radius: 4pt,
  width: 100%,
  stroke: 0.5pt + luma(215),
)[#text(size: 8.5pt)[#it]]

// ===========================================================================
// FRONT MATTER
// ===========================================================================

#outline(title: [Table of Contents], depth: 2)
#pagebreak()

#outline(title: [Table of Figures], target: figure.where(kind: image))
#pagebreak()

#heading(numbering: none, outlined: false)[Abstract]

FELCOM is a pair of battery-powered handheld radios that provide secure,
infrastructure-free text and voice communication in the 2.4#sym.space.nobreak GHz
ISM band. Each unit is built around an ESP32 microcontroller driving an
nRF24L01+ transceiver, an INMP441 I#super[2]S microphone, a DAC, amplifier and
speaker audio chain, an OLED display and tactile controls, all integrated on a
custom PCB. To resist narrowband jamming and lower the probability of
interception, the link continuously hops across six interference-free channels
using software-controlled Frequency-Hopping Spread Spectrum (FHSS); the two nodes
are kept aligned by a shared, timer-driven hopping schedule and a lightweight
node-discovery synchronization protocol. A custom 32-byte packet format
multiplexes text, voice and control traffic over the same radio and carries a
compact forward-error-correction header. Reliability is provided by a layered FEC
subsystem owned by the transceiver: a CRC-16 detects corruption on every
protected packet, an automatic-repeat-request (ARQ) scheme guarantees exact
delivery of chat messages, and an XOR block code reconstructs lost real-time
audio packets without retransmission. Voice is captured as 8#sym.space.nobreak kHz
8-bit PCM and streamed half-duplex through a fully non-blocking cooperative
pipeline, so channel hopping is never interrupted. Chat text is obfuscated with a
lightweight XOR cipher. A built-in bit-error-rate (BER) test mode and live FEC
counters allow link quality to be measured and analysed. The result demonstrates
that robust, jam-resistant, low-cost digital voice and messaging can be realised
entirely on commodity hardware.

#pagebreak()

// ===========================================================================
// CHAPTER 1: INTRODUCTION
// ===========================================================================
= Introduction

== Background and Motivation

Reliable communication is usually taken for granted because it rests on a large,
fixed infrastructure: cell towers, base stations, the Internet backbone. That
infrastructure is also a single point of failure. In a natural disaster, in a
remote area, or in any situation where the network is congested, censored or
deliberately disabled, the same devices that work everywhere suddenly work
nowhere. This motivates *off-grid* communication: a direct radio link between two
people that depends on no third party.

A direct radio link, however, is exposed. A fixed-frequency transmitter is easy
to locate, easy to intercept and trivial to jam, since a single interfering
source on the right frequency is enough to silence it. The classical answer to
this problem is *spread spectrum*, an idea whose frequency-hopping variant was
famously patented by Hedy Lamarr and George Antheil in 1942 as a jam-resistant
guidance scheme for torpedoes. By rapidly and unpredictably changing the carrier
frequency, a frequency-hopping system spreads its energy across a wide band: a
narrowband jammer can only spoil the few hops that happen to land on its
frequency, and an eavesdropper who does not know the hopping sequence sees only
brief, scattered bursts. The same principle underlies Bluetooth today.

== The Project

FELCOM (a contraction of the team's initials and "communication") applies these
ideas on inexpensive, off-the-shelf hardware. It is a self-contained handheld
device (no phone, no router, no SIM card) that lets two units exchange text
messages and live voice directly over the 2.4#sym.space.nobreak GHz band. The
link is made resilient by software Frequency-Hopping Spread Spectrum, protected
by a custom packet protocol with forward error correction, and the text is
obfuscated by a lightweight cipher. Two extra modes, a real-time multiplayer Pong
game and an RF/BER test screen, were added to validate, respectively, low-latency
bidirectional traffic and raw link quality.

== Objectives

The project set out to:

- build a working two-node handheld system on an ESP32 + nRF24L01+ platform;
- implement software FHSS over interference-free channels, with a synchronization
  mechanism robust to clock drift and to new nodes joining;
- define a flexible packet protocol able to multiplex text, voice and control
  traffic over one radio;
- add a layered forward-error-correction subsystem (detection, retransmission,
  and forward recovery) matched to each traffic type;
- stream intelligible real-time voice without breaking channel hopping;
- provide a debug mode that measures packet loss and bit-error rate; and
- integrate everything onto a custom PCB.

== Impact

On the environment and on society, a device of this kind is useful precisely
where conventional networks are not: disaster relief, hiking and expeditions,
events with saturated cellular coverage, and education. It is built from cheap,
widely available parts (a complete two-unit build costs well under
\$100), which keeps it accessible. Equally, the project is a compact, hands-on
study of the entire wireless stack (radio, channel access, framing, error
control, real-time media and basic security), which is its main pedagogical
value.

== Technology and Methodology

The system is built on the Espressif ESP32 (dual-core, hardware timers, I#super[2]S
and DAC peripherals) paired with the Nordic nRF24L01+, a 2.4#sym.space.nobreak GHz
GFSK transceiver that performs modulation in hardware and exposes 125
software-selectable 1#sym.space.nobreak MHz channels, the property that makes
software FHSS possible. Development followed the phased plan laid out in the
project proposal: components and basic UI first, then the FHSS and packet
protocol, then forward error correction, then real-time audio, and finally
hardware integration on a PCB, with continuous BER testing throughout. The
remaining chapters follow the resulting architecture from the bottom of the stack
upward.

#figure(
  image("diagrams/context.png", width: 100%),
  caption: [System context: two handhelds communicating directly, with no
  network infrastructure, over a frequency-hopping link.],
  kind: image,
) <fig-context>

#pagebreak()

// ===========================================================================
// CHAPTER 2: SYSTEM OVERVIEW
// ===========================================================================
= System Overview

== Hardware

Each FELCOM unit is identical and runs the same firmware (only a one-byte
`NODE_ID` differs). The core is an *ESP32 DevKit V1*. Around it:

#table(
  columns: (auto, 1fr),
  inset: 5pt,
  align: (left, left),
  [*Block*], [*Role and interface*],
  [nRF24L01+], [2.4 GHz GFSK radio; SPI; CE = GPIO4, CSN = GPIO5; 1 Mbps, max PA level.],
  [INMP441], [I#super[2]S MEMS microphone (voice in); SCK = GPIO13, WS = GPIO16, SD = GPIO17.],
  [DAC, LM386, speaker], [Voice out; ESP32 DAC1 = GPIO25 into an audio amplifier.],
  [OLED 0.96" (SSD1306)], [128#sym.times#h(0pt)64 status/UI display; I#super[2]C, SDA = GPIO21, SCL = GPIO22.],
  [Buttons], [Navigation: Up/Down/Select/Back (GPIO32/33/27/14).],
)

The complete schematic and the routed PCB are shown below.

#figure(
  image("hardware/imgs/schematic.png", width: 95%),
  caption: [Full hardware schematic of a FELCOM unit (ESP32, nRF24L01+, INMP441,
  audio amplifier, OLED and controls).],
) <fig-schematic>

#figure(
  image("hardware/imgs/pcb.png", width: 70%),
  caption: [The custom-designed FELCOM PCB layout.],
) <fig-pcb>

== Firmware Architecture

The firmware is organised in clear layers, which keeps each block (FHSS, framing,
FEC, audio) independent and testable:

- *Physical layer*: the RF24 driver and the raw nRF24L01+ register access.
- *Transceiver layer* (`lib/transceiver`): owns the radio. It performs channel
  access (CSMA), address filtering, and *all* error-control logic. Crucially, the
  FEC subsystem is *built into* the transceiver: application code never calls FEC
  routines directly, it simply uses `write`, `writeReliable`, `read`, `audioTx`
  and `audioRx`.
- *Application layer*: chat, audio walkie-talkie, Pong, and the RF/BER test, each
  with its own payload struct and OLED screen.

A deliberate design decision shapes everything above the radio: the main loop is
*cooperative and non-blocking*. There are no FreeRTOS tasks for audio. Channel
hopping, UI input, message handling and audio capture/playback are all short
steps pumped from a single `loop()`. A hardware timer increments a free-running
counter every 500#sym.space.nobreak ms; the loop reads it and hops when it
changes. Because no step blocks, hopping always happens on time even while audio
is streaming, which is the central constraint the whole architecture is built
around.

#figure(
  image("diagrams/architecture.png", width: 60%),
  caption: [Layered firmware architecture and the non-blocking main loop.],
  kind: image,
) <fig-arch>

#pagebreak()

// ===========================================================================
// CHAPTER 3: FHSS
// ===========================================================================
= Frequency-Hopping Spread Spectrum

FHSS is the heart of the project and the source of its jam- and
detection-resistance. The nRF24L01+ does the modulation; FHSS is the software
layer that keeps changing *which* of its 125 channels is active.

== Channel Selection

The 2.4#sym.space.nobreak GHz band is crowded, mostly by Wi-Fi, whose channels
are 20 to 22#sym.space.nobreak MHz wide. Hopping blindly would land many hops
inside a Wi-Fi channel and lose them. We therefore restrict hopping to the top of
the band (around 2.51#sym.space.nobreak GHz, nRF channels in the 110 to 115
range), which sits above the standard Wi-Fi allocations and is normally quiet.
The current build uses a deliberately small hop set of six channels,
`{110, 111, 112, 113, 114, 115}`, which trades a little spreading gain for far
easier and faster synchronization (see below).

== Timer-Driven Hopping

Both nodes share the same hop schedule. A hardware timer increments a global
counter every 500#sym.space.nobreak ms; the active channel is simply
`HOPPING_CHANNELS[counter mod 6]`. Because both units derive the channel from the
same counter value, they stay on the same frequency *as long as their counters
agree*. A full sweep of the six channels takes 3#sym.space.nobreak seconds.

#figure(
  ```cpp
  // Every 500 ms the shared timer counter ticks; the channel follows it.
  uint32_t slot = readCounter() % HOPPING_CHANNELS_SIZE;
  xcvr.setChannel(HOPPING_CHANNELS[slot]);   // one of 110..115
  ```,
  caption: [Timer-driven hopping: both nodes pick the channel from the same
  free-running counter, so they stay aligned without exchanging anything.],
  kind: raw,
  supplement: [Listing],
) <lst-hop>

== The Synchronization Problem

Synchronization is the hardest part of any FHSS system, and we evaluated the
classic approaches and their failure modes:

- *Hop on packet reception*: a receiver that misses a packet stalls on the wrong
  channel until the transmitter happens to revisit it.
- *Hop on a timer*: robust moment to moment, but the two clocks slowly *drift*
  apart, and once they diverge they may never re-align on their own.

Both also share the *new-node* problem: a unit powering on has no idea where in
the schedule the network currently is. Our solution combines timer-based hopping
(for steady-state simplicity) with an explicit discovery/resync packet:

- *`ND_SYNC`* (node-discovery / sync) packets carry the sender's current counter
  value. A joining or drifted node listens, then adopts the counter it hears,
  snapping its own timer into alignment. If it hears nothing within a timeout it
  assumes it is alone and forms a new network at counter 0.
- A small hop-set means a new node has to wait at most a few hops before it lands
  on the same channel as an active node, so worst-case join latency is bounded and
  short.

In practice the two units are aligned from the dedicated *SYNC* screen before a
chat or audio session; because the clocks drift, a re-sync is occasionally needed
during long sessions. `ND_SYNC` is sent *raw* (no CRC, no retransmission) on
purpose: synchronization is how nodes *recover* from desync, so it must be as
forgiving as possible. A slightly corrupted sync packet is better than a dropped
one.

== Channel Access (CSMA)

Because several packet types share the link, ordinary transmissions use *Carrier
Sense Multiple Access*: before sending, the radio briefly listens
(`testRPD()`); if the channel is busy it backs off a random 1 to 10#sym.space.nobreak ms
and retries (up to five times). Real-time audio is the one exception: it
deliberately *bypasses* CSMA backoff, because millisecond stalls would starve the
audio pipeline (Chapter 6).

#figure(
  image("diagrams/hop-timeline.png", width: 80%),
  caption: [Timer-driven hopping schedule across the six channels and re-alignment
  via an `ND_SYNC` packet.],
  kind: image,
) <fig-hop-timeline>

#figure(
  image("diagrams/sync-fsm.png", width: 58%),
  caption: [Node synchronization and network-join state machine.],
  kind: image,
) <fig-sync-fsm>

#pagebreak()

// ===========================================================================
// CHAPTER 4: PACKET PROTOCOL / DATA LINK
// ===========================================================================
= The Packet Protocol and Data Link

Every transmission, regardless of type, is a fixed *32-byte* frame, the maximum
nRF24L01+ payload. A single frame format multiplexing all traffic is what lets
text, voice and control share one hopping radio.

== Frame Format

#table(
  columns: (auto, auto, 1fr),
  inset: 5pt,
  [*Field*], [*Size*], [*Purpose*],
  [`src_node_id`], [1 byte], [Sender's node ID.],
  [`dst_node_id`], [1 byte], [Destination ID; `0xFF` = broadcast to all.],
  [`packet_type`], [4 bits], [PONG, CHAT, AUDIO, ND_SYNC, TEST or ACK.],
  [`fec`], [12 bits], [FEC metadata: scheme(4) | XOR block id(4) | slot index(4).],
  [`data`], [28 bytes], [Payload region.],
)

The `packet_type` and `fec` fields are packed together into a single 16-bit
bit-field, keeping the header to exactly 4 bytes and leaving 28 bytes of payload.
Addressing is by one-byte node ID, with `0xFF` reserved for broadcast. This is
how, for example, an RF test ping or a sync beacon reaches every node at once.

#figure(
  ```cpp
  struct __attribute__((packed)) FHSSPacket {
      uint8_t  src_node_id;       // sender
      uint8_t  dst_node_id;       // 0xFF = broadcast
      uint16_t packet_type : 4;   // PONG / CHAT / AUDIO / ND_SYNC / TEST / ACK
      uint16_t fec : 12;          // scheme | XOR block id | slot index
      uint8_t  data[28];          // payload (CRC in the last 2 bytes)
  };
  ```,
  caption: [The single 32-byte frame; `packet_type` and `fec` are packed as
  bit-fields so the header is just four bytes.],
  kind: raw,
  supplement: [Listing],
) <lst-packet>

== The Payload Region and the FEC Convention

A strict, project-wide convention governs the 28-byte `data` region so that the
framing and the error-control layer never fight over the same bytes:

- The frame size *never* changes; it is always 32 bytes on air.
- For *protected* packets, the last 2 bytes of `data` hold a CRC-16, leaving 26
  user bytes. Reliable (ARQ) packets further spend the first 2 of those on a
  sequence number, leaving 24 application bytes.
- The 12-bit `fec` header field carries *metadata only* (which scheme, and the
  XOR block/slot identifiers), never the CRC itself, which would not fit in 12
  bits.

This single, fixed layout is what allows one `read()` path to demultiplex six
different packet types and hand each application exactly its own payload.

#figure(
  image("diagrams/packet.png", width: 100%),
  caption: [The 32-byte frame and the two payload layouts (protected and ARQ).],
  kind: image,
) <fig-packet>

#pagebreak()

// ===========================================================================
// CHAPTER 5: FEC
// ===========================================================================
= Forward Error Correction

The radio's own CRC and auto-acknowledgement are *disabled*; error control is
handled entirely by our own FEC subsystem, which lives inside the transceiver.
This was a conscious choice: different traffic types need very different
guarantees, and doing it ourselves lets us pick the right mechanism per type. The
subsystem combines three complementary mechanisms.

== The Three Mechanisms

- *CRC-16-CCITT (detection).* Every protected packet carries a 16-bit checksum in
  the last two payload bytes. On receipt the CRC is recomputed; a mismatch means
  corruption and the packet is silently dropped. This is the baseline that turns a
  noisy link into a clean-or-nothing one.
- *ARQ (retransmission).* For chat, which is infrequent but must be exact, the
  sender attaches a sequence number, transmits, and *waits for an ACK*,
  retransmitting on a 200#sym.space.nobreak ms timeout up to three times. The
  receiver only ACKs a message after it has safely buffered it, so a lost frame is
  re-sent rather than falsely confirmed. ARQ guarantees delivery but blocks
  briefly, so it is used only where latency does not matter.
- *XOR block code (forward recovery).* For audio, which is real-time and where
  waiting for a retransmission is pointless, the sender groups every four data
  packets and transmits a fifth *parity* packet equal to their bitwise XOR. If any
  *one* of the five packets in a block is lost, the receiver reconstructs it by
  XOR-ing the four it did receive. No round trip and no waiting: losses are
  repaired in the forward direction only.

#figure(
  ```cpp
  // TX: accumulate a running parity across the data slots of a block.
  for (uint8_t b = 0; b < FEC_USER_SIZE; b++)
      parity[b] ^= pkt.data[b];

  // After FEC_XOR_BLOCK_SIZE (=4) data packets, emit the parity packet.
  if (++count >= FEC_XOR_BLOCK_SIZE)
      transmitAudioPacket(parityPkt);   // lets RX rebuild one lost packet
  ```,
  caption: [The XOR parity is just the bitwise XOR of the four data packets; the
  receiver rebuilds any single missing packet from the other four plus the parity.],
  kind: raw,
  supplement: [Listing],
) <lst-xor>

== Per-Type Policy

Each packet type is matched to the mechanism that fits its needs:

#table(
  columns: (auto, auto, auto, auto, 1fr),
  inset: 5pt,
  align: (left, center, center, center, left),
  [*Type*], [*CRC*], [*ARQ*], [*XOR*], [*Rationale*],
  [CHAT], [#sym.checkmark], [#sym.checkmark], [#sym.crossmark], [Infrequent; must be exact.],
  [AUDIO], [#sym.checkmark], [#sym.crossmark], [#sym.checkmark], [Real-time; forward recovery only.],
  [PONG], [#sym.checkmark], [#sym.crossmark], [#sym.crossmark], [Frequent; ARQ would backlog.],
  [ACK], [#sym.checkmark], [#sym.crossmark], [#sym.crossmark], [Small control packet.],
  [ND_SYNC], [#sym.crossmark (raw)], [#sym.crossmark], [#sym.crossmark], [Must stay forgiving for resync.],
  [TEST], [#sym.crossmark (raw)], [#sym.crossmark], [#sym.crossmark], [Must measure *real* bit errors.],
)

The TEST packet is sent completely raw on purpose, so the BER screen measures the
true error rate of the channel rather than a CRC-cleaned version of it.

#figure(
  image("diagrams/xor-fec.png", width: 92%),
  caption: [XOR block forward error correction: one lost packet per block of four
  is reconstructed from the parity packet.],
  kind: image,
) <fig-xor>

#figure(
  image("diagrams/arq.png", width: 52%),
  caption: [ARQ exchange for reliable chat delivery, including retransmission on
  timeout.],
  kind: image,
) <fig-arq>

#pagebreak()

// ===========================================================================
// CHAPTER 6: AUDIO
// ===========================================================================
= Real-Time Audio

The audio mode turns the pair into a half-duplex *walkie-talkie*: one node TALKS,
the other LISTENS, and Select toggles the role. It is the most demanding feature,
because live voice must coexist with channel hopping that interrupts the link
every 500#sym.space.nobreak ms.

== Format and Capture

Voice is captured from the INMP441 as *8#sym.space.nobreak kHz, 8-bit mono PCM*,
deliberately low-fidelity, which is enough for intelligible speech and keeps the
data rate low. The microphone is read over I#super[2]S without blocking (zero
timeout). Each sample is then conditioned in software: a running DC-offset
estimate is subtracted (an exponential moving average), a gain factor is applied,
and the result is clamped to 8 bits. Twenty-four conditioned samples
(3#sym.space.nobreak ms of audio) are packed into one `AudioPayload`, which is
sized to *exactly* fill the 26-byte protected user region.

== Transport and Playback

Each payload is handed to `audioTx`, which wraps it with CRC + XOR-block FEC and
transmits it (skipping CSMA backoff, as noted earlier). On the listening side,
recovered payloads come back through `audioRx` and are written into a large ring
buffer that absorbs network jitter; samples are then drained to the DAC (GPIO25)
at a precise 8#sym.space.nobreak kHz using `micros()` timing. No extra hardware
timer is needed, since the hop counter already owns one.

== Non-Blocking Pipeline

The whole chain (capture, condition, transmit, receive, play) is a set of short
cooperative steps pumped from the main loop. Nothing blocks, so FHSS hopping
continues uninterrupted while audio flows. The XOR FEC matters most here: at a hop
boundary the in-flight packet is often lost, and the block code transparently
rebuilds it, smoothing what would otherwise be an audible click every half second.

#figure(
  image("diagrams/audio-pipeline.png", width: 90%),
  caption: [The non-blocking real-time audio pipeline, from microphone to speaker.],
  kind: image,
) <fig-audio>

#pagebreak()

// ===========================================================================
// CHAPTER 7: SECURITY & APPLICATIONS
// ===========================================================================
= Security and Applications

== Text Confidentiality

Chat text is passed through a lightweight *XOR stream cipher* (a repeating 4-byte
key) before transmission and reversed on receipt, so the message is not sent in
clear over the air. Combined with the unknown hopping sequence, which already
makes the traffic hard to capture coherently, this gives a basic layer of
confidentiality appropriate to the platform. We are explicit that this is
obfuscation, not strong cryptography; replacing it with a real cipher is listed in
future work.

== Applications

Four screens exercise the stack:

- *Chat*: reliable (ARQ + CRC), encrypted point-to-point or broadcast text, shown
  on a scrolling OLED log.
- *Audio*: the half-duplex walkie-talkie of Chapter 6.
- *Pong*: a two-player real-time game whose paddle/ball updates validate
  low-latency *bidirectional* traffic over the hopping link.
- *RF / BER Test*: a diagnostic mode (next chapter).

#pagebreak()

// ===========================================================================
// CHAPTER 8: TESTING & RESULTS
// ===========================================================================
= Testing, Results and Analysis

== Method

Two instruments are built into the firmware. The *RF/BER test* sends raw,
known-pattern packets (`0xAB` repeated) at a fixed rate; the receiver echoes them
back, and both sides count sent / received / echoed / corrupt packets. Because
TEST packets bypass the FEC, this measures the *true* channel error rate. Second,
the transceiver keeps live *FEC counters* (CRC pass/fail, ARQ
sent/retransmitted/acked/failed, and audio blocks/recovered/lost), printed as a
throttled serial summary. Together they let us separate raw link quality from what
the FEC layer recovers.

== Results

#todo[Insert the measured numbers from your test runs. Suggested figures/tables:
(a) packet-loss and corruption rate from the RF/BER screen, fixed channel vs.
hopping; (b) FEC counters during a chat session (CRC ok/bad, ARQ retransmissions);
(c) audio session counters (blocks finalised, packets recovered by XOR, packets
lost); (d) any jamming experiment: loss with a narrowband interferer present, with
and without hopping. Replace the placeholder below with the real chart/table.]

#figure(
  ph(height: 4.5cm)[Results chart/table: e.g. bar chart of packet-loss %
  (fixed-frequency vs. FHSS, with and without an interferer), or a table of the FEC
  counters captured during a representative chat and audio session.],
  caption: [Measured link reliability and FEC recovery results.],
  kind: image,
) <fig-results>

== Analysis

#todo[Write 1 to 2 paragraphs interpreting the numbers once measured: how much
loss FHSS avoids under interference, how often XOR FEC rebuilds audio packets
(especially at hop boundaries), how many ARQ retransmissions chat needs, and where
the link still struggles (e.g. after clock drift).]

#pagebreak()

// ===========================================================================
// CHAPTER 9: CHALLENGES
// ===========================================================================
= Challenges and How We Tackled Them

- *Synchronization vs. clock drift.* Pure timer hopping drifts; pure
  reception-based hopping stalls on a missed packet. We combined timer hopping
  with raw `ND_SYNC` resync packets and a small hop-set to bound join time, and
  expose a manual SYNC screen for long sessions.

- *I#super[2]S microphone returned silence.* On our boards the ESP32 legacy I#super[2]S
  `ONLY_LEFT` channel format reads all zeros, a known quirk. We capture in stereo
  and select the correct DMA slot in software (`AUDIO_I2S_SLOT_INDEX`); this cost a
  lot of debugging time and is now documented so it is not rediscovered.

- *Audio glitching at hop boundaries.* Each 500#sym.space.nobreak ms hop tends to
  drop the in-flight packet. The XOR block code rebuilds one loss per block of
  four, and a sizeable RX ring buffer absorbs the jitter, which together smooth the
  audible clicks.

- *CSMA stalling real-time audio.* The 1 to 10#sym.space.nobreak ms CSMA backoff,
  fine for chat, starved the audio pipeline and caused choppy playback. Audio
  transmission was made to bypass backoff; collisions are tolerated because the
  mode is half-duplex and already FEC-protected.

- *A subtle CSMA / BER interaction.* A short carrier-sense settling delay that
  seemed correct in theory broke the BER screen in practice; we identified it
  empirically and removed it for the test path. It is flagged in the code as a
  known, not-fully-explained interaction: honest engineering rather than a silent
  fudge.

- *Fitting everything in 32 bytes.* Reserving CRC and sequence bytes inside the
  fixed 28-byte payload forced the chat text length and the audio frame size to be
  re-derived so nothing overflowed the single on-air frame.

#pagebreak()

// ===========================================================================
// CHAPTER 10: DISTRIBUTION OF WORK
// ===========================================================================
= Distribution of Work

#table(
  columns: (auto, auto, 1fr),
  inset: 5pt,
  align: horizon,
  [*Member*], [*Primary focus*], [*Contributions*],
  [Charbel Assaad], [RF & Protocol],
  [FHSS implementation and the custom packet protocol; BER monitoring; support on
  UI integration.],
  [Marc Gedeon], [Audio & Encryption],
  [Real-time audio capture/streaming, the FEC layer it rides on, and text
  encryption; control firmware; protocol and FHSS testing.],
  [Yorgo Hassabou], [Hardware & UI],
  [PCB design, component integration and power; OLED menus and the debug
  interface; firmware support and FHSS testing.],
)

All three members contributed to integration, testing and debugging across the
whole system.

#pagebreak()

// ===========================================================================
// CONCLUSION
// ===========================================================================
= Conclusion and Future Work

FELCOM shows that resilient, jam-resistant, infrastructure-free communication,
both text and live voice, can be built on inexpensive, commodity hardware. The
working system hops across six interference-free channels under software control,
keeps two nodes aligned despite clock drift, multiplexes several traffic types
over one 32-byte frame, and matches a forward-error-correction mechanism to each:
CRC detection everywhere, ARQ for exact chat delivery, and an XOR block code that
repairs real-time audio without retransmission. The non-blocking architecture is
what ties it together, letting voice stream while the radio keeps hopping.

Several directions would extend the work:

- *Stronger encryption*: replace the XOR cipher with an authenticated block cipher
  (e.g. AES) and proper key exchange for genuine end-to-end security, and extend
  confidentiality to the audio stream.
- *Better synchronization*: implement the planned implicit clock correction on
  every received packet and automatic out-of-sync recovery, removing the need for
  manual re-SYNC.
- *Audio quality*: push-to-talk hardware, full-duplex operation, a short RX
  pre-buffer, and light compression for clearer voice.
- *Larger networks*: extend addressing and discovery beyond point-to-point toward
  a small multi-node mesh.
- *Power and form factor*: battery management and a smaller enclosure for a truly
  portable handheld.

#pagebreak()

// ===========================================================================
// ACKNOWLEDGMENTS & REFERENCES
// ===========================================================================
= Acknowledgments

We thank our supervisor, Dr. Hadi Jerdek, for his guidance throughout the project,
and the Faculty of Engineering at the Lebanese University for the resources and
support that made this work possible.

= References

#set enum(numbering: "[1]")

+ Nordic Semiconductor, _nRF24L01+ Single Chip 2.4 GHz Transceiver Product
  Specification_.
+ Espressif Systems, _ESP32 Technical Reference Manual_ and _ESP32 Series
  Datasheet_.
+ InvenSense (TDK), _INMP441 Omnidirectional Microphone with Bottom Port and
  I#super[2]S Digital Output, Datasheet_.
+ Texas Instruments, _LM386 Low Voltage Audio Power Amplifier, Datasheet_.
+ TMRh20, _RF24: Optimized Driver for nRF24L01(+) on Arduino & Raspberry Pi_
  (open-source library).
+ H. K. Markey (Lamarr) and G. Antheil, _Secret Communication System_, U.S. Patent
  2,292,387, 1942.
+ ITU-T Recommendation V.41, _Code-independent error-control system_ (CRC-16-CCITT).
+ A. Goldsmith, _Wireless Communications_, Cambridge University Press (spread
  spectrum and FHSS fundamentals).
