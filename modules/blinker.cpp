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

eResult Blinker::Start(bool fromISR)
{
    if (!fromISR)
    {
        BaseType_t success = xTimerStart(m_timer, DEFAULT_FREERTOS_TIMEOUT);
        if (success != pdPASS)
        {
            ESPARRAG_LOG_ERROR("start blinker timer failed");
            return eResult::ERROR_INVALID_STATE;
        }
    }
    else
    {
        BaseType_t higherPriority = pdFALSE;
        BaseType_t success = xTimerStartFromISR(m_timer, &higherPriority);
        if (success != pdPASS)
            return eResult::ERROR_INVALID_STATE;

        YIELD_FROM_ISR_IF(higherPriority);
    }

    return eResult::SUCCESS;
}

eResult Blinker::Stop(bool fromISR)
{
    if (!fromISR)
    {
        BaseType_t success = xTimerStop(m_timer, DEFAULT_FREERTOS_TIMEOUT);
        if (success != pdPASS)
        {
            ESPARRAG_LOG_ERROR("start blinker timer failed");
            return eResult::ERROR_INVALID_STATE;
        }
    }
    else
    {
        BaseType_t higherPriority = pdFALSE;
        BaseType_t success = xTimerStopFromISR(m_timer, &higherPriority);
        if (success != pdPASS)
            return eResult::ERROR_INVALID_STATE;

        YIELD_FROM_ISR_IF(higherPriority);
    }

    return eResult::SUCCESS;
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

    if (m_blinkTimes != FOREVER && m_alreadyBlinkedTimes >= m_blinkTimes)
    {
        m_alreadyBlinkedTimes = 0;
        return;
    }

    if (m_on != m_off)
    {
        TickType_t newPeriod = m_state ? m_off.toTicks() : m_on.toTicks();
        xTimerChangePeriod(m_timer, newPeriod, 0);
    }

    xTimerReset(m_timer, 0);
}