#ifndef ENCODERS_H
#define ENCODERS_H

#include <config.h>
#include <debug.h>

void setupRotaryEncoder();

#if defined(ROTARY_ENCODER_CW_CALLBACK)
extern void ROTARY_ENCODER_CW_CALLBACK();
#endif

#if defined(ROTARY_ENCODER_CCW_CALLBACK)
extern void ROTARY_ENCODER_CCW_CALLBACK();
#endif

#if defined(ROTARY_ENCODER_SW_CALLBACK)
extern void ROTARY_ENCODER_SW_CALLBACK();
#endif

#endif // ENCODERS_H