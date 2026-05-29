# FHSS Design (Current Implementation)

# Channel selection
We use channels in the upper NRF24 band to reduce interference. NRF24 operates from **2.400 GHz** to **2.525 GHz**.

Selected hop set (current):
```
110, 111, 112, 113, 114, 115
```

# Network layer
## Packet format
32 bytes total:

| section name | size      |
| ------------ | --------- |
| src_node_id  | 1 byte    |
| dst_node_id  | 1 byte    |
| packet_type  | 0.5 bytes |
| FEC          | 1.5 bytes |
| data         | 28 bytes  |

*Note: `packet_type` and `FEC` are packed using bit-fields (e.g., `uint16_t type: 4; uint16_t fec: 12;`).*

### Packet Type
| Type name |
| --------- |
| PONG      |
| CHAT      |
| AUDIO     |
| ND_SYNC   |
| TEST      |
| ACK       |

## Protocol rules
- **CSMA/CA**: Use `RF24::testRPD()` with random backoff before transmitting. Current backoff range is 1-10 ms, up to 3 attempts.
- **Broadcast**: `dst_node_id == 0xFF` means broadcast.
- **Unknown sender sync**: If a node receives any non-`ND_SYNC` packet from a sender it is not synchronized with, it requests sync from that sender when it has no peers; otherwise it sends a unicast `ND_SYNC` reply with its current timer value (best-effort with CSMA).
- **ND_SYNC**: Payload is a timer value. If `timer_val == -1`, the sender is requesting synchronization. Replies are unicast to the requester.

# Bootstrap synchronization (active scan)
1. Listen on the bootstrap channel `110`.
2. Broadcast `ND_SYNC` with `timer_val = -1` for up to the bootstrap timeout.
3. If a synchronized node replies with a timer value, set the local timer and join the network.
4. If no reply arrives before the timeout, create a new network: preload the timer, enable hopping, and broadcast `ND_SYNC` with that preload.

# Hopping
- Timer-based hopping every **200 ms** through the hop set in order.
- The ISR only sets a hop flag; the main loop performs the RF24 channel change.

# Out-of-sync handling
- Track activity by hop count. If no packet is received or transmitted for **1000 hops (~2 seconds)**, declare out of sync.
- On out-of-sync, stop hopping and restart the active scan procedure.

# Future work
- **Drift adjustment**: On every successful packet reception, compare the expected timer value with the actual hardware timer and apply minor correction (`timerWrite`).
- **Idle heartbeat / time master**: Define a time master to send periodic sync when the network is idle.

# Relevant files
- `include/protocol.h` — Protocol-level packet types and `FHSSPacket`/`NDSyncData`.
- `lib/transceiver/transceiver.h` — FHSS state machine, join state, and packet helpers.
- `lib/transceiver/transceiver.cpp` — FHSS hopping, CSMA, address filtering, join logic.
- `src/main.cpp` — Timer initialization and interrupt attachment.