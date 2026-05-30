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
- Nodes must implement CSMA using `bool RF24::testRPD(void)` along with a **random backoff** mechanism before transmission to handle collisions gracefully.
- `dst_node_id`, if set to `0xFF`, means the packet is broadcast and is meant for everyone on the network.
- On *every* successful packet reception, nodes should read their current hardware timer. If the timer is slightly off from the expected slot time, they must perform a minor adjustment (`timerWrite`) to compensate for local clock drift. (Not yet implemented.)
- **Out of Sync**: If a node goes 1000 hops (~2 seconds) with no packets received or transmitted, it is assumed to be out of sync. It must stop hopping and enter a recovery state to broadcast an `ND_SYNC` packet. (Not yet implemented.)
- **`ND_SYNC` Packets**: Contain the hardware timer counter value in the data section. In the current implementation, replies are sent directly to the requester (not broadcast).
  - If the counter is `-1`, the transmitter is out of sync and is requesting a response to synchronize.

To join a network, nodes have two options:
1. Choose a random channel from the hoppable list, wait for *any* packet to be received, transmit an `ND_SYNC` packet (utilizing random backoff), wait for responses from other nodes, and synchronize with the highest value of the counter received. (Current implementation is non-blocking and driven by the main loop.)
2. Transmit an `ND_SYNC` packet with a counter value of `-1`, wait for a response, and set the counter value.

If no replies are received within the join timeout, the node resets its timer to 0, enables hopping, and forms a new network.

## Solution
- Low number of hoppable channels.
- Timer based hopping with implicit clock adjustment on every received packet.
- Designated Time Master for idle network heartbeats.
- `ND_SYNC` used for major synchronization, node discovery, and joining networks (similar to ARP).

# Implementation Plan

- **List of hoppable channels**: `110, 111, 112, 113, 114, 115`
- **Timer frequency**: `2 ms` (Worst case 12 ms rotation)
	- `timerBegin(uint8_t num, uint16_t divider, bool countUp)`
	- `timerAttachInterrupt(hw_timer_t * timer, void (*userFunc)(void))`
	- `timerAlarm(hw_timer_t * timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count)`
	- `timerRead(hw_timer_t * timer)`
	- `timerWrite(hw_timer_t * timer, uint64_t val)`

## Steps
1. ~~**Packet Structure Update**~~ (Done): Define the new 32-byte generic packet struct using C++ bit-fields (`uint16_t type: 4; uint16_t fec: 12;`) to handle the `packet_type` and `FEC`. Protocol types live in `include/protocol.h`.
2. ~~**Address Filtering**~~ (Done): Update the payload size to 32 bytes (`sizeof(FHSSPacket)`) in `setup()`. In `read()`, cast incoming data to `FHSSPacket*` and discard packets where `dst_node_id` is neither our node ID nor the broadcast ID (`0xFF`).
3. ~~**CSMA & Random Backoff**~~ (Done): Implement carrier sensing using `RF24::testRPD()` in `write()`. Before transmitting, switch briefly to `RECEIVE` mode, wait 200us, verify the channel is clear, and if busy apply a random backoff delay (e.g., 1-10ms) before retrying.
4. ~~**Setup Timer & Hopping Sequence**~~ (Done): Initialize the hardware timer with a 2ms frequency. Attach an interrupt that switches the NRF24 channel sequentially through the hoppable list.
5. ~~**Network Joining Mechanism**~~ (Done): 
	- Implement active join (broadcast `ND_SYNC` with counter -1, wait for response).
	- Implement passive join (pick a random channel, wait for a packet, then broadcast `ND_SYNC` with backoff to synchronize).
	- Auto-join on first transmit if not already joined.
6. **Synchronization Maintenance (Implicit Piggybacking)**: 
    - On every successful packet reception, compare the expected timer value with the actual hardware timer.
    - Write a minor adjustment to the hardware timer (`timerWrite()`) to compensate for local clock drift.
7. **Out-of-Sync Handling**: Implement a counter to track consecutive silent hops. If the network is entirely silent for 1000 hops (~2 seconds), declare the node out of sync and broadcast an `ND_SYNC` packet.

## Relevant files
- `include/protocol.h` — Protocol-level packet types and `FHSSPacket`/`NDSyncData`.
- `lib/transceiver/transceiver.h` — FHSS state machine, join state, and packet helpers.
- `lib/transceiver/transceiver.cpp` — FHSS hopping, CSMA, address filtering, join logic.
- `src/chat/ChatMessage.h` — Chat payload struct.
- `src/pong/PongMessage.h` — Pong payload struct.
- `src/rf_test/TestMessage.h` — RF test payload struct.
- `src/main.cpp` — Timer initialization and interrupt attachment.