# Channel selection
It is important for us to communicate on channels that don't suffer from interference.
The following is a list of nrf channels that falls between 2 wifi channels which should help us avoid interference.
```
13, 14, 15, 18, 19, 20, 23, 24, 25, 28, 29, 30, 33,
34, 35, 38, 39, 40, 43, 44, 45, 48, 49, 50, 53, 54,
55, 58, 59, 60, 63, 64, 65, 68, 69, 70, 77, 78, 79
```
![[Pasted image 20260512171434.png]]
## Nrf24 channels
The channel occupies a bandwidth of **1MHz at 1Mbps** and **2MHz at 2Mbps**. 
nRF24L01 can operate on frequencies from **2.400GHz** to **2.525GHz**

## Solution
With no way to physically check, using bands that are not used by WIFI will be the best choice.
This leaves with 2.5 -> 2.525
In terms of NRF24, **channel 100 -> 125**
# Synchronization
The biggest problem with FHSS is synchronization, many solutions are available, to name a few along with their problems:
1. Hoping on packet reception.
	- if we were to hop when a packet is received, when a receiver misses a packet, it will temporally become out of sync until the transmitter transmits a packet on the channel the receiver is currently listening on.
2. Hoping based on a timer.
	- When the clock drift becomes significant, the transmitter and the receiver will become out of sync with a really low probability to synchronize again.


Both of these suffer from the same problem, **new node synchronization**. 
When a new node connects to the network, for implementation:
1. The worst case is `len(hopping_channels)` messages lost, before successful synchronization.
2. The worst case is no synchronization ever!

A good implementation would be, one that minimizes the amount of messages lost due to the nodes being out of sync, and allows new nodes to hop on the network with no overhead or creating it's own network.

## Current Implementation

The current implementation uses a **manual synchronization** approach via the SyncTest UI:

- **Timer-based Hopping**: A hardware timer runs with a 500ms period (`timerAlarmWrite(COUNTER_TIMER, 500000, true)`). Nodes hop channels based on `readCounter() % HOPPING_CHANNELS_SIZE`, where `HOPPING_CHANNELS_SIZE = 6` (channels 110-115).

- **Manual Sync via UI**: In the SyncTest screen, pressing the button:
  1. Broadcasts an `ND_SYNC` packet containing the current counter value via `sendSync(counter)`
  2. Resets the local counter timer via `resetCounterTimer()`
  3. Flashes the SYNC button visually for 200ms

- **Receiving Sync**: When an `ND_SYNC` packet is received:
  1. The local counter is set to the received `timer_val` via `setCounter()`
  2. The received node is added to the synced nodes list
  3. The SYNC button flashes visually

- **ND_SYNC Packet Properties**:
  - Sent as **raw/unprotected** (FecScheme::NONE) to ensure it's never dropped by CRC validation
  - Contains: `timer_val` (int64_t), `nbSyncedNodes` (uint8_t), and `syncedNodes[10]` (array of node IDs)
  - Broadcast to all nodes (dst_node_id = 0xFF)

- **Synchronization Mechanism**: All nodes that receive the sync packet reset their counter to the same value, ensuring they all hop to the same channel at the same time. The counter increments every 500ms, and channel changes occur when `counter % 6` changes.

- **Limitations**:
  - Requires manual user intervention to synchronize
  - No automatic drift compensation between hops
  - No automatic out-of-sync detection
  - New nodes must manually sync via the SyncTest UI
  - Clock drift between nodes will eventually cause desynchronization

This approach prioritizes simplicity and reliability (sync packets are never dropped) over automatic synchronization.

# Network Layer
## packet format
32 Bytes can be used.

| section name | size      |
| ------------ | --------- |
| src_node_id  | 1 byte    |
| dst_node_id  | 1 byte    |
| packet_type  | 0.5 bytes |
| FEC          | 1.5 bytes |
| data         | 28 bytes  |

*Note: In C++, `packet_type` and `FEC` will be packed using bit-fields (e.g., `uint16_t type: 4; uint16_t fec: 12;`) to fit neatly into 2 bytes.*

### Packet Type
| Type name |
| --------- |
| PONG      |
| CHAT      |
| AUDIO     |
| ND_SYNC   |
| TEST      |
| ACK       |

## protocol
- Nodes implement CSMA using `bool RF24::testRPD(void)` along with a **random backoff** mechanism (1-10ms) before transmission to handle collisions gracefully. CSMA is **disabled for audio packets** to avoid blocking the real-time audio loop.
- `dst_node_id`, if set to `0xFF`, means the packet is broadcast and is meant for everyone on the network.
- **Automatic drift compensation is not implemented** - nodes do not adjust their timers on packet reception.
- **Out-of-sync detection is not implemented** - nodes do not detect when they are out of sync or automatically broadcast `ND_SYNC` packets.
- **`ND_SYNC` Packets**: Contain the hardware timer counter value in the data section. In the current implementation:
  - Broadcast to all nodes (dst_node_id = 0xFF)
  - Sent as raw/unprotected (FecScheme::NONE) to prevent CRC drops
  - Used only in the SyncTest UI for manual synchronization
  - The counter value is always the sender's current counter (never -1)

To join a network, nodes currently must:
1. Use the SyncTest UI to manually synchronize with an existing network by pressing the SYNC button
2. All nodes will then reset their counter to the same value and begin hopping in sync

If nodes are not synchronized, they will be on different channels at different times, and communication will fail until manual synchronization is performed.

## Solution
- Low number of hoppable channels (6 channels: 110-115).
- Timer based hopping using a hardware timer (500ms period).
- Manual synchronization via ND_SYNC packets broadcast from the SyncTest UI.
- `ND_SYNC` used for manual synchronization and tracking which nodes are in sync.

# Implementation Plan

- **List of hoppable channels**: `110, 111, 112, 113, 114, 115`
- **Timer frequency**: `500ms` (500000 microseconds, ~3s full rotation across 6 channels)

## Steps
1. ~~**Packet Structure Update**~~ (Done): Define the new 32-byte generic packet struct using C++ bit-fields (`uint16_t type: 4; uint16_t fec: 12;`) to handle the `packet_type` and `FEC`. Protocol types live in `include/protocol.h`.
2. ~~**Address Filtering**~~ (Done): Update the payload size to 32 bytes (`sizeof(FHSSPacket)`) in `setup()`. In `read()`, cast incoming data to `FHSSPacket*` and discard packets where `dst_node_id` is neither our node ID nor the broadcast ID (`0xFF`).
3. ~~**CSMA & Random Backoff**~~ (Done): Implement carrier sensing using `RF24::testRPD()` in `transmitPacket()`. Before transmitting, switch briefly to `RECEIVE` mode, verify the channel is clear, and if busy apply a random backoff delay (1-10ms) before retrying. Note: CSMA is disabled for audio packets via `transmitAudioPacket()`.
4. ~~**Setup Timer & Hopping Sequence**~~ (Done): Initialize the hardware timer with a 500ms period (`timerAlarmWrite(COUNTER_TIMER, 500000, true)`). The main loop reads the counter and changes channels based on `readCounter() % HOPPING_CHANNELS_SIZE`.
5. ~~**Manual Synchronization**~~ (Done): 
	- Implement SyncTest UI with manual sync button
	- Broadcast `ND_SYNC` packet with current counter value via `sendSync()`
	- Reset local counter on sync packet reception via `setCounter()`
	- Track synced nodes in `syncedNodes[]` array
6. **Synchronization Maintenance (Implicit Piggybacking)**: 
    - **Not implemented**: Automatic drift compensation on packet reception
    - **Not implemented**: Consecutive silent hop tracking
7. **Out-of-Sync Handling**: 
    - **Not implemented**: Automatic detection and recovery via `ND_SYNC` broadcast

## Relevant files
- `include/config.h` — Hopping channel configuration (`HOPPING_CHANNELS`, `HOPPING_CHANNELS_SIZE`).
- `include/protocol.h` — Protocol-level packet types, `FHSSPacket`, `NDSyncData`, `FecScheme`.
- `lib/ftimers/ftimers.h` / `ftimers.cpp` — Hardware timer initialization, counter read/write/reset.
- `lib/transceiver/transceiver.h` — FHSS state machine, synced nodes tracking, packet helpers.
- `lib/transceiver/transceiver.cpp` — FHSS hopping, CSMA, address filtering, `sendSync()`, `writeRaw()`.
- `src/ui/SyncTestUI.h` / `SyncTestUI.cpp` — Manual synchronization UI with SYNC button.
- `src/chat/ChatMessage.h` — Chat payload struct.
- `src/pong/PongMessage.h` — Pong payload struct.
- `src/rf_test/TestMessage.h` — RF test payload struct.
- `src/main.cpp` — Main loop with channel hopping logic based on counter modulo.