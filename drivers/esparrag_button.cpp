#include "esparrag_button.h"
#include "debouncer.h"

// ========IDLE STATE============
etl::fsm_state_id_t
IdleState::on_enter_state()
{
    return STATE_ID;
}

etl::fsm_state_id_t
IdleState::on_event(const PressEvent &event)
{
    return eStateId::PRESSED;
}

etl::fsm_state_id_t
IdleState::on_event_unknown(const etl::imessage &event)
{
    ets_printf("pressed unknown %d\n", (uint8_t)get_state_id());
    return STATE_ID;
}

// ========PRESSED STATE============
etl::fsm_state_id_t
PressedState::on_enter_state()
{
    auto &button = get_fsm_context();
    button.runPressCallback(BUTTON::ePressTypes::FAST_PRESS);

    m_timeouts = BUTTON::ePressTypes::PRESS_TIMEOUT_1;
    button.m_lastPressTime = esp_timer_get_time();
    button.startTimer(m_timeouts);
    return STATE_ID;
}

etl::fsm_state_id_t
PressedState::on_event(const ReleaseEvent &event)
{
    auto &button = get_fsm_context();
    button.stopTimer(true);
    button.runReleaseCallback();
    return eStateId::IDLE;
}

etl::fsm_state_id_t
PressedState::on_event(const TimerEvent &event)
{
    auto &button = get_fsm_context();
    button.runPressCallback(m_timeouts);

    m_timeouts++;
    button.startTimer(m_timeouts);
    return STATE_ID;
}

etl::fsm_state_id_t
PressedState::on_event_unknown(const etl::imessage &event)
{
    m_timeouts = 0;
    ets_printf("pressed unknown %d\n", (uint8_t)get_state_id());
    return STATE_ID;
}

// ==========BUTTON==============

BUTTON::BUTTON(GPI &gpi) : fsm(get_instance_count()), m_gpi(gpi)
{
    eResult res = m_gpi.EnableInterrupt(buttonISR, this);
    m_buttonState = m_gpi.IsActive();
    if (m_buttonState)
    {
        ESPARRAG_LOG_ERROR("button state on, on startup");
        ESPARRAG_ASSERT(m_buttonState == false);
    }

    ESPARRAG_ASSERT(res == eResult::SUCCESS);
    m_timer = xTimerCreate("button", 100, pdFALSE, this, timerCB);
    ESPARRAG_ASSERT(m_timer);
    set_states(stateList, etl::size(stateList));
    start();
}

eResult BUTTON::registerEvent(buttonCB &&cb, callback_list_t &list)
{
    ESPARRAG_ASSERT(cb.m_cb != nullptr);
    if (cb.m_ms.value() == 0)
    {
        list[ePressTypes::FAST_PRESS] = cb;
        ESPARRAG_LOG_ERROR("registered as fast");
        return eResult::SUCCESS;
    }
    else if (list[ePressTypes::PRESS_TIMEOUT_1].m_cb == nullptr)
    {
        list[ePressTypes::PRESS_TIMEOUT_1] = cb;
        ESPARRAG_LOG_ERROR("registered as short");
        return eResult::SUCCESS;
    }
    else if (list[ePressTypes::PRESS_TIMEOUT_2].m_cb == nullptr)
    {
        if (list[ePressTypes::PRESS_TIMEOUT_1].m_ms > cb.m_ms)
        {
            ESPARRAG_LOG_ERROR("registered as short");
            list[ePressTypes::PRESS_TIMEOUT_2] = list[ePressTypes::PRESS_TIMEOUT_1];
            list[ePressTypes::PRESS_TIMEOUT_1] = cb;
        }

        else
        {

            ESPARRAG_LOG_ERROR("registered as long");
            list[ePressTypes::PRESS_TIMEOUT_2] = cb;
        }

        return eResult::SUCCESS;
    }

    ESPARRAG_LOG_ERROR("couldnt find place for callback");
    return eResult::ERROR_INVALID_STATE;
}

eResult BUTTON::RegisterPress(buttonCB &&cb)
{
    return registerEvent(std::move(cb), m_pressCallbacks);
}
eResult BUTTON::RegisterRelease(buttonCB &&cb)
{
    return registerEvent(std::move(cb), m_releaseCallbacks);
}

void BUTTON::runPressCallback(uint8_t index)
{
    if (index >= MAX_CALLBACKS)
        return;

    auto &callback = m_pressCallbacks[index];
    if (callback.m_cb != nullptr)
    {
        BaseType_t higherPriorityExists;
        xTimerPendFunctionCallFromISR(callback.m_cb, callback.arg, callback.arg2, &higherPriorityExists);
        YIELD_FROM_ISR_IF(higherPriorityExists);
    }
}

void BUTTON::runReleaseCallback()
{
    ESPARRAG_ASSERT(esp_timer_get_time() > m_lastPressTime.value())
    MicroSeconds timePassedSincePress = MicroSeconds(esp_timer_get_time()) - m_lastPressTime;

    for (int i = ePressTypes::PRESS_TIMEOUT_2; i >= ePressTypes::FAST_PRESS; i--)
    {
        auto &callback = m_releaseCallbacks[i];
        if (callback.m_cb != nullptr)
        {
            if (timePassedSincePress >= callback.m_ms)
            {
                BaseType_t higherPriorityExists;
                xTimerPendFunctionCallFromISR(callback.m_cb, callback.arg, callback.arg2, &higherPriorityExists);
                YIELD_FROM_ISR_IF(higherPriorityExists);
                return;
            }
        }
    }
}

void BUTTON::stopTimer(bool fromISR)
{
    if (fromISR)
        xTimerStopFromISR(m_timer, NULL);
    else
        xTimerStop(m_timer, DEFAULT_FREERTOS_TIMEOUT);
}

void BUTTON::startTimer(int index)
{
    ESPARRAG_ASSERT(index >= ePressTypes::PRESS_TIMEOUT_1);
    if (index >= MAX_CALLBACKS)
        return;

    auto &callback = m_pressCallbacks[index];
    bool callbackValid = callback.m_cb != nullptr && callback.m_ms.value() != 0;
    if (!callbackValid)
        return;

    MilliSeconds timeout = callback.m_ms - m_pressCallbacks[index - 1].m_ms;
    BaseType_t higherPriorityExists = pdFALSE;
    xTimerChangePeriodFromISR(m_timer, timeout.toTicks(), &higherPriorityExists);
    xTimerStartFromISR(m_timer, &higherPriorityExists);
    YIELD_FROM_ISR_IF(higherPriorityExists);
}

void IRAM_ATTR BUTTON::buttonISR(void *arg)
{
    static Debouncer sampler(10000);
    if (!sampler.IsValidNow())
        return;

    BUTTON *button = reinterpret_cast<BUTTON *>(arg);
    button->m_buttonState = !button->m_buttonState;
    if (button->m_buttonState)
        button->receive(PressEvent());
    else
        button->receive(ReleaseEvent());
}

void BUTTON::timerCB(xTimerHandle timer)
{
    BUTTON *button = reinterpret_cast<BUTTON *>(pvTimerGetTimerID(timer));
    button->receive(TimerEvent());
}
