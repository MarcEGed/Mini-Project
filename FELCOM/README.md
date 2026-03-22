## Problems Encountered
1. Build failing due to `ModuleNotFoundError: No module named 'intelhex'`
    - `source ~/.platformio/penv/bin/activate`
    - `pip install intelhex`

    Windows fix: `~/.platformio/penv/Scripts/pip install intelhex (same but for dummies)`

## Code Guidelines

- Includes within the `src` directory should use `" "`
- Includes to other libraries (`lib`) or the `include` directory should use `< >`
- We camel the Case 
- Header files should only include the necessary for them to work, and included first in the cpp (refer to to this [SO answer](https://stackoverflow.com/a/2596554)) 
- All macros even if they are meant to be constants should be defined in `config.h` this makes it easier to change them if necessary, and make it easier to debug/get an idea of what's going on under the hood

## Progress
### Completed
- [x] Test the Display code
- [X] ~~include statements are acting goofy, should check that, fixable by modifying platformio.ini~~ never mind, my download resolved and everything works
- [X] Texting works (tested with 2 nodes/devices, not more)
- [x] Test pong

### Next Up
- [ ] Make the rotary encoder less of a pain in the ass
- [ ] Clean up all unecessary includes from texting related code
- [ ] Encode text messaging
- [ ] Transcevier should be a ~~class~~ struct (marc note: does it matter?)
- [ ] Make moving ball pong
