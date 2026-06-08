#ifndef SELECTED_DESTINATION_H
#define SELECTED_DESTINATION_H

#include <stdint.h>

static constexpr uint8_t BROADCAST_DST_NODE = 0xFF;

extern uint8_t SELECTED_DST_NODE;

inline bool selectedDestinationIsBroadcast() {
    return SELECTED_DST_NODE == BROADCAST_DST_NODE;
}

#endif  // SELECTED_DESTINATION_H
