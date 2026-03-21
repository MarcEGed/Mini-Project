#include "encoders.h"

#include <Arduino.h>
#include <debug.h>

// Variables to hold the current and last encoder position
volatile int encoderPos = 0;
volatile int lastEncoderPos = 0;

// Variables to keep track of the state of the pins
volatile int lastCLK;
volatile int currentCLK;
volatile unsigned long lastButtonInterruptMs = 0;

constexpr unsigned long BUTTON_DEBOUNCE_MS = 10;

void ARDUINO_ISR_ATTR readEncoder();
void ARDUINO_ISR_ATTR encoderButtonPressed();

void setupRotaryEncoder()
{
#if defined(USE_ROTARY_ENCODER) && USE_ROTARY_ENCODER
    pinMode(ROTARY_ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_SW_PIN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_CLK_PIN), readEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_SW_PIN), encoderButtonPressed, FALLING);

    // Read the initial state of CLK
    lastCLK = digitalRead(ROTARY_ENCODER_CLK_PIN);
#endif
}

// Interrupt service routine for reading the encoder
void ARDUINO_ISR_ATTR readEncoder()
{
#if defined(USE_ROTARY_ENCODER) && USE_ROTARY_ENCODER
    currentCLK = digitalRead(ROTARY_ENCODER_CLK_PIN);
    // If the current state of CLK is different from the last state
    // then a pulse occurred
    if (currentCLK != lastCLK)
    {
        // If the DT state is different from the CLK state
        // then the encoder is rotating clockwise
        if (digitalRead(ROTARY_ENCODER_DT_PIN) != currentCLK)
        {
            encoderPos++;
#if defined(ROTARY_ENCODER_CW_CALLBACK)
            ROTARY_ENCODER_CW_CALLBACK();
#endif
        }
        else
        {
            // Otherwise, it's rotating counterclockwise
            encoderPos--;
#if defined(ROTARY_ENCODER_CCW_CALLBACK)
            ROTARY_ENCODER_CCW_CALLBACK();
#endif
        }
    }

    // Update lastCLK with the current state for the next pulse detection
    lastCLK = currentCLK;
#endif
}

void ARDUINO_ISR_ATTR encoderButtonPressed()
{
#if defined(USE_ROTARY_ENCODER) && USE_ROTARY_ENCODER
    const unsigned long nowMs = millis();
    if ((nowMs - lastButtonInterruptMs) < BUTTON_DEBOUNCE_MS)
    {
        return;
    }

    lastButtonInterruptMs = nowMs;
#if defined(ROTARY_ENCODER_SW_CALLBACK)
    ROTARY_ENCODER_SW_CALLBACK();
#endif
#endif
}
