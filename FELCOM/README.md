# Progress
## Completed
- [x] Test the Display code
- [X] ~~include statements are acting goofy, should check that, fixable by modifying platformio.ini~~ never mind, my download resolved and everything works
- [X] Texting works (tested with 2 nodes/devices, not more)
- [x] Test pong

## Next Up
- [ ] Make the rotary encoder less of a pain in the ass
- [ ] Clean up all unecessary includes from texting related code
- [ ] Encode text messaging
- [ ] Transcevier should be a ~~class~~ struct (marc note: does it matter?)
- [ ] Make moving ball pong

# Code Guidelines

- Includes within the `src` directory should use `" "`
- Includes to other libraries (`lib`) or the `include` directory should use `< >`
- We camel the Case 
- Header files should only include the necessary for them to work, and included first in the cpp (refer to to this [SO answer](https://stackoverflow.com/a/2596554)) 
- All macros even if they are meant to be constants should be defined in `config.h` this makes it easier to change them if necessary, and make it easier to debug/get an idea of what's going on under the hood

# Problems Encountered
1. Build failing due to `ModuleNotFoundError: No module named 'intelhex'`
    - `source ~/.platformio/penv/bin/activate`
    - `pip install intelhex`

    Windows fix: `~/.platformio/penv/Scripts/pip install intelhex (same but for dummies)`


# nRF24L01
- At the time of writing, we are forced to use the Enhanced ShockBurst Engine the module provides.
- Enhanced ShockBurst is a packet based data link layer
- Auto Acknowledgement, Auto Retransmission, and CRC can be disabled.
- Our data is stored in the Payload. 
## TMRh20 Library
- ` setAddressWidth()`
- `stopListening( const uint8_t * txAddress)` sets the TX addr
    - `write( const void * buf, uint8_t len )`
- `openReadingPipe(1, addr)`
    - `startListening()`
        - `available()` checks if there's data to read
        - `read()` reads the data
            - `getPayloadSize()`
            - `getDynamicPayloadSize()`
            > [!IMPORTANT]
            > A payload is not removed from the RX FIFO until it's entire length (or more) is fetched using `read()`.
- `printPrettyDetails()`
- `testCarrier()` Useful to check for interference on the current channel.
- `testRPD()` whether a signal (carrier or otherwise) greater than or equal to -64dBm is present on the channel
- `closeReadingPipe()`
- `setChannel()`
- `setStatusFlags()` Set which flags shall be reflected on the radio's IRQ pin.
- `clearStatusFlags()` Clear the Status flags that caused an interrupt event.
- `powerDown()`
- `powerUp()` 
> [!NOTE]
> This will take up to 5ms for maximum compatibility 
- `startConstCarrier()` Transmission of constant carrier wave with defined frequency and output power
- `stopConstCarrier()`
> [!IMPORTANT]
> If isPVariant() returns true, please remember to re-configure the radio's settings
> ```// re-establish default settings```
> ```setCRCLength(RF24_CRC_16);```
> ```setAutoAck(true);```
> ```setRetries(5, 15);```

### IRQ
```C
void isrCallbackFunction() {
  bool tx_ds, tx_df, rx_dr;
  uint8_t flags = radio.clearStatusFlags(); // resets the IRQ pin to HIGH
  radio.available();                        // returned data should now be reliable
}

pinMode(IRQ_PIN, INPUT);
attachInterrupt(digitalPinToInterrupt(IRQ_PIN), isrCallbackFunction, FALLING);
```
## Block Diagram
![](assets/nRF24L01%20block%20diagram.png)
## Channel
The channel occupies a bandwidth of 1MHz at 1Mbps and 2MHz at 2Mbps.
nRF24L01 can operate on frequencies from 2.400GHz to 2.525GHz
## Data Link Packet
![](assets/Enhanced%20shockburst%20packet.png)
### Address (Pipe)
> A data pipe is a logical channel in the physical RF channel

This is the physical address of the receiver.

It can be 3, 4 or, 5 bytes long.

> [!NOTE]
> Note: Addresses where the level shifts only one time (that is, 000FFFFFFF) can often be detected in noise and can give a false detection, which may give a raised Packet-Error-Rate. Addresses as a continuation of the preamble (hi-low toggling) raises the Packet-Error-Rate.

### Packet Control Field
![](assets/Packer%20Control%20Field.png)
#### Payload Length
The chip can be configured to use Static and Dynamic Payload Length.
## Disable Enhanced ShockBurst
Disabling the Enhanced ShockBurst features is done by setting register EN_AA=0x00 and the ARC = 0.

In addition, the nRF24L01 air data rate must be set to 1Mbps.

This will make the chip use ShockBurst.

The differences between the ShockBurst packet and the Enhanced ShockBurst packet are:
- The 9 bit Packet Control Field is not present in the ShockBurst packet format.
- The CRC is optional in the ShockBurst packet format and is controlled by the EN_CRC bit in the CONFIG register.