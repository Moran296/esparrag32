#include "blinker.h"
#include "esparrag_log.h"

Blinker::Blinker(GPO &gpo, MilliSeconds on, MilliSeconds off, int times) : m_gpo(gpo),
                                                                           m_on(on),
                                                                           m_off(off),
                                                                           m_blinkTimes(times)
{
    m_timer = xTimerCreate("blinker", m_on.toTicks(), pdFALSE, this, timerCB);
    ESPARRAG_ASSERT(m_timer);
}

void Blinker::Start(bool end_level)
{
    m_end_level = end_level;
    BaseType_t success = xTimerStart(m_timer, DEFAULT_FREERTOS_TIMEOUT);
    ESPARRAG_ASSERT(success == pdPASS);
}

void Blinker::Stop(bool end_level)
{
    BaseType_t success = xTimerStop(m_timer, DEFAULT_FREERTOS_TIMEOUT);
    ESPARRAG_ASSERT(success == pdPASS);

    m_gpo.Set(end_level);
}

void Blinker::timerCB(TimerHandle_t xTimer)
{
    Blinker *blinker = reinterpret_cast<Blinker *>(pvTimerGetTimerID(xTimer));
    blinker->blink();
}

void Blinker::blink()
{
    m_state = !m_state;
    m_gpo.Set(m_state);
    if (!m_state)
        m_alreadyBlinkedTimes++;

    //Finished blinking
    if (m_blinkTimes != FOREVER && m_alreadyBlinkedTimes >= m_blinkTimes)
    {
        m_alreadyBlinkedTimes = 0;
        m_gpo.Set(m_end_level);
        return;
    }

    // Set next blink
    if (m_on != m_off)
    {
        TickType_t newPeriod = m_state ? m_off.toTicks() : m_on.toTicks();
        xTimerChangePeriod(m_timer, newPeriod, 0);
    }

    xTimerReset(m_timer, 0);
}