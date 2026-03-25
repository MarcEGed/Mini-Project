#include "chatLog.h"

#include <string.h>

void chatLog::init() {
    count = 0;
    head = 0;
}

void chatLog::push(Message& msg) {
    entries[head] = msg;
    // circular index
    head = (head + 1) % LOG_SIZE;
    if (count < LOG_SIZE) count++;
}

Message* chatLog::get(uint8_t index) {
    // index 0 = newest message, NOT OLDEST
    // the LOGSIZE * 2 is just some maths bs to make sure negatives aren't real
    int8_t i = ((int)head - 1 - index + LOG_SIZE * 2) % LOG_SIZE;
    return &entries[i];
}

uint8_t chatLog::size() { return count; }