#include "esparrag_button.h"
#include "etl/algorithm.h"

// ========IDLE STATE============
etl::fsm_state_id_t
IdleState::on_event(const PressEvent &event)
{
    return eStateId::PRESSED;
}

etl::fsm_state_id_t
IdleState::on_event_unknown(const etl::imessage &event)
{
    return STATE_ID;
}

// ========PRESSED STATE============
etl::fsm_state_id_t
PressedState::on_enter_state()
{
    auto &button = get_fsm_context();
    button.runPressCallback(button.m_pressCallbacks, ePressType::FAST_PRESS);

    m_timeouts = ePressType::PRESS_TIMEOUT_1;
    button.m_lastPressTime = esp_timer_get_time();
    button.startTimer(ePressType(m_timeouts));
    return STATE_ID;
}

etl::fsm_state_id_t
PressedState::on_event(const ReleaseEvent &event)
{
    auto &button = get_fsm_context();
    button.stopTimer(true);
    button.runReleaseCallback(button.m_releaseCallbacks);
    return eStateId::IDLE;
}

etl::fsm_state_id_t
PressedState::on_event(const TimerEvent &event)
{
    auto &button = get_fsm_context();
    button.runPressCallback(button.m_pressCallbacks, ePressType(m_timeouts));

    m_timeouts++;
    button.startTimer(ePressType(m_timeouts));
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

eResult BUTTON::RegisterPress(buttonCB &&cb)
{
    return registerEvent(std::move(cb), m_pressCallbacks);
}

eResult BUTTON::RegisterRelease(buttonCB &&cb)
{
    return registerEvent(std::move(cb), m_releaseCallbacks);
}

eResult BUTTON::registerEvent(buttonCB &&cb, callback_list_t &cb_list)
{
    ESPARRAG_ASSERT(cb.m_cb != nullptr);
    MilliSeconds cbTime = cb.m_ms;
    // if timeout is 0, it should be registered as the first callback
    if (cbTime.value() == 0)
    {
        if (cb_list[0].m_cb != nullptr)
            ESPARRAG_LOG_WARNING("button event is registered instead of an existing event");

        cb_list[ePressType::FAST_PRESS] = cb;
        ESPARRAG_LOG_INFO("event written as %s", ePressType(0).c_str());
        return eResult::SUCCESS;
    }

    //utility for inserting and reordering
    auto cb_sorted_inserter = [&cb_list, &cb](int i)
    {
        cb_list[i] = cb;
        etl::sort(cb_list.begin(), cb_list.begin() + i);
        ESPARRAG_LOG_INFO("event written as %s", ePressType(i).c_str());
        return eResult::SUCCESS;
    };

    //otherwise find a place in the timeout section
    for (size_t i = ePressType::PRESS_TIMEOUT_1; i < ePressType::PRESS_NUM; i++)
    {
        if (cb_list[i].m_ms.value() == 0)
        {
            //we found an empty place
            return cb_sorted_inserter(i);
        }

        if (cb_list[i].m_ms == cbTime)
        {
            //we found a callback with an equal time, we will replace it
            ESPARRAG_LOG_WARNING("button event is registered instead of an existing event");
            return cb_sorted_inserter(i);
        }
    }

    ESPARRAG_LOG_ERROR("couldnt find place for callback");
    return eResult::ERROR_INVALID_PARAMETER;
}

void BUTTON::runPressCallback(callback_list_t &cb_list, ePressType press)
{
    if (press.get_value() >= MAX_CALLBACKS)
        return;

    auto &callback = cb_list[press];
    if (callback.m_cb != nullptr)
    {
        BaseType_t higherPriorityExists;
        xTimerPendFunctionCallFromISR(callback.m_cb, callback.arg, callback.arg2, &higherPriorityExists);
        YIELD_FROM_ISR_IF(higherPriorityExists);
    }
}

void BUTTON::runReleaseCallback(callback_list_t &cb_list)
{
    if (m_ignoreReleaseCallback)
    {
        m_ignoreReleaseCallback = false;
        return;
    }

    ESPARRAG_ASSERT(esp_timer_get_time() > m_lastPressTime.value())
    MicroSeconds timePassedSincePress = MicroSeconds(esp_timer_get_time()) - m_lastPressTime;

    //find the longest release time that is smaller than time passed since press
    for (int i = ePressType::PRESS_TIMEOUT_2; i >= ePressType::FAST_PRESS; i--)
    {
        auto &callback = cb_list[i];
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

void BUTTON::startTimer(ePressType timeout)
{
    ESPARRAG_ASSERT(timeout.get_value() >= ePressType::PRESS_TIMEOUT_1);
    if (timeout.get_value() >= MAX_CALLBACKS)
        return;

    auto &callback = m_pressCallbacks[timeout.get_value()];
    bool callbackValid = callback.m_cb != nullptr && callback.m_ms.value() != 0;
    if (!callbackValid)
        return;

    //callback timeout is the delta between requested time and last callback timeout
    MilliSeconds ms = callback.m_ms - m_pressCallbacks[timeout.get_value() - 1].m_ms;
    BaseType_t higherPriorityExists = pdFALSE;
    xTimerChangePeriodFromISR(m_timer, ms.toTicks(), &higherPriorityExists);
    xTimerStartFromISR(m_timer, &higherPriorityExists);
    YIELD_FROM_ISR_IF(higherPriorityExists);
}

void IRAM_ATTR BUTTON::buttonISR(void *arg)
{
    ets_printf("button isr");
    BUTTON *button = reinterpret_cast<BUTTON *>(arg);
    if (!button->m_sampler.IsValidNow())
        return;

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
