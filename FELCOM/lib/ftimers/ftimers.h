#ifndef FTIMERS_H
#define FTIMERS_H

#include <Arduino.h>
#include <stdint.h>

void initCounter();
void resetCounterTimer();
uint32_t readCounter();
void setCounter(uint32_t value);
#endif