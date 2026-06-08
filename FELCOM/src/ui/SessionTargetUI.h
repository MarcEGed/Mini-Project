#ifndef SESSION_TARGET_UI_H
#define SESSION_TARGET_UI_H

#include <stdint.h>

struct transceiver;

bool sessionTargetUITickInput(transceiver& xcvr, int8_t dirY, bool btnDown,
                              uint8_t currentDstNode,
                              uint8_t* outSelectedDstNode,
                              bool* outConfirmed);

void sessionTargetUIInitDisplay(transceiver& xcvr, uint8_t currentDstNode);
void sessionTargetUIUpdate(transceiver& xcvr);

#endif  // SESSION_TARGET_UI_H
