#include "ftimers.h"

static hw_timer_t* COUNTER_TIMER = nullptr;
static volatile uint32_t COUNTER = 0;

static void IRAM_ATTR onCounterTimer() { COUNTER++; }

void initCounter() {
    COUNTER_TIMER = timerBegin(1, 80, true);
    timerAttachInterrupt(COUNTER_TIMER, &onCounterTimer, true);
    timerAlarmWrite(COUNTER_TIMER, 500000, true);
    timerAlarmEnable(COUNTER_TIMER);
}

void resetCounterTimer() { timerWrite(COUNTER_TIMER, 0); }

uint32_t readCounter() { return COUNTER; }

void setCounter(uint32_t value) { COUNTER = value; }
