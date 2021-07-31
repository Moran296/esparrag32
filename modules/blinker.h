#ifndef ESPARRAG_BLINKER_H__
#define ESPARRAG_BLINKER_H__

#include "esparrag_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esparrag_common.h"
#include "esparrag_time_units.h"

class Blinker
{
public:
    static constexpr int FOREVER = -1;
    Blinker(GPO &gpo, MilliSeconds on, MilliSeconds off, int times = FOREVER);
    eResult Start(bool fromISR = false);
    eResult Stop(bool fromISR = false);

private:
    GPO &m_gpo;
    xTimerHandle m_timer = nullptr;

    MilliSeconds m_on{};
    MilliSeconds m_off{};
    int m_blinkTimes{};
    int m_alreadyBlinkedTimes{};
    bool m_state{};

    void init();
    static void timerCB(TimerHandle_t xTimer);
    void blink();
};

#endif