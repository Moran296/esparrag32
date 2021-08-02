#include "esparrag_button.h"
#include "etl/algorithm.h"

// ========IDLE STATE============
etl::fsm_state_id_t
Button::IdleState::on_event(const PressEvent &event)
{
    return eStateId::PRESSED;
}

etl::fsm_state_id_t
Button::IdleState::on_event_unknown(const etl::imessage &event)
{
    return STATE_ID;
}

// ========PRESSED STATE============
etl::fsm_state_id_t
Button::PressedState::on_enter_state()
{
    auto &button = get_fsm_context();
    button.runPressCallback(button.m_pressCallbacks, ePressType::FAST_PRESS);

    m_timeouts = ePressType::PRESS_TIMEOUT_1;
    button.m_lastPressTime = esp_timer_get_time();
    button.startTimer(ePressType(m_timeouts));
    return STATE_ID;
}

etl::fsm_state_id_t
Button::PressedState::on_event(const ReleaseEvent &event)
{
    auto &button = get_fsm_context();
    button.stopTimer(true);
    button.runReleaseCallback(button.m_releaseCallbacks);
    return eStateId::IDLE;
}

etl::fsm_state_id_t
Button::PressedState::on_event(const TimerEvent &event)
{
    auto &button = get_fsm_context();
    button.runPressCallback(button.m_pressCallbacks, ePressType(m_timeouts));

    m_timeouts++;
    button.startTimer(ePressType(m_timeouts));
    return STATE_ID;
}

etl::fsm_state_id_t
Button::PressedState::on_event_unknown(const etl::imessage &event)
{
    m_timeouts = 0;
    ets_printf("pressed unknown %d\n", (uint8_t)get_state_id());
    return STATE_ID;
}

// ==========Button==============

Button::Button(GPI &gpi) : fsm(get_instance_count()), m_gpi(gpi)
{
    eResult res = m_gpi.SetInterruptType(GPIO_INTR_HIGH_LEVEL);
    ESPARRAG_ASSERT(res == eResult::SUCCESS);

    m_buttonState = m_gpi.IsActive();
    if (m_buttonState)
    {
        ESPARRAG_LOG_ERROR("button state on, on startup");
        ESPARRAG_ASSERT(m_buttonState == false);
    }

    m_timer = xTimerCreate("button", 100, pdFALSE, this, timerCB);
    ESPARRAG_ASSERT(m_timer);
    set_states(m_stateList, etl::size(m_stateList));

    res = m_gpi.EnableInterrupt(buttonISR, this);
    ESPARRAG_ASSERT(res == eResult::SUCCESS);
    start();
}

eResult Button::RegisterPress(const buttonCB &cb)
{
    return registerEvent(cb, m_pressCallbacks);
}

eResult Button::RegisterRelease(const buttonCB &cb)
{
    return registerEvent(cb, m_releaseCallbacks);
}

eResult Button::registerEvent(const buttonCB &cb, callback_list_t &cb_list)
{
    ESPARRAG_ASSERT(cb.cb_function != nullptr);
    MilliSeconds cbTime = cb.cb_time;

    //utility for inserting and reordering
    auto cb_sorted_inserter = [&cb_list, &cb](int i)
    {
        cb_list[i] = cb;
        ESPARRAG_LOG_INFO("i = %d", i);
        for (size_t j = 0; j <= i; j++)
        {
            ESPARRAG_LOG_INFO("j %d. time %d", i, cb_list[j].cb_time.value());
        }
        etl::sort(cb_list.begin(), cb_list.begin() + i + 1);
        ESPARRAG_LOG_INFO("event written as %s", ePressType(i).c_str());
        for (size_t j = 0; j <= i; j++)
        {
            ESPARRAG_LOG_INFO("j %d. time %d", j, cb_list[j].cb_time.value());
        }

        return eResult::SUCCESS;
    };

    // if timeout is 0, it should be registered as the first callback
    if (cbTime.value() == 0)
    {
        if (cb_list[0].cb_function != nullptr)
            ESPARRAG_LOG_WARNING("button event is registered instead of an existing event");

        cb_list[ePressType::FAST_PRESS] = cb;
        ESPARRAG_LOG_INFO("event written as %s", ePressType(0).c_str());
        return eResult::SUCCESS;
    }

    //otherwise find a place in the timeout section
    for (size_t i = ePressType::PRESS_TIMEOUT_1; i < ePressType::PRESS_NUM; i++)
    {
        if (cb_list[i].cb_time.value() == 0)
        {
            //we found an empty place
            return cb_sorted_inserter(i);
        }

        if (cb_list[i].cb_time == cbTime)
        {
            //we found a callback with an equal time, we will replace it
            ESPARRAG_LOG_WARNING("button event is registered instead of an existing event");
            return cb_sorted_inserter(i);
        }
    }

    ESPARRAG_LOG_ERROR("couldnt find place for callback");
    return eResult::ERROR_INVALID_PARAMETER;
}

void Button::runPressCallback(callback_list_t &cb_list, ePressType press)
{
    if (press.get_value() >= MAX_CALLBACKS)
        return;

    auto &callback = cb_list[press];
    if (callback.cb_function != nullptr)
    {
        BaseType_t higherPriorityExists;
        xTimerPendFunctionCallFromISR(callback.cb_function, callback.cb_arg1, callback.cb_arg2, &higherPriorityExists);
        YIELD_FROM_ISR_IF(higherPriorityExists);
    }
}

void Button::runReleaseCallback(callback_list_t &cb_list)
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
        if (callback.cb_function != nullptr)
        {
            if (timePassedSincePress >= callback.cb_time)
            {
                BaseType_t higherPriorityExists;
                xTimerPendFunctionCallFromISR(callback.cb_function, callback.cb_arg1, callback.cb_arg2, &higherPriorityExists);
                YIELD_FROM_ISR_IF(higherPriorityExists);
                return;
            }
        }
    }
}

void Button::stopTimer(bool fromISR)
{
    if (fromISR)
        xTimerStopFromISR(m_timer, NULL);
    else
        xTimerStop(m_timer, DEFAULT_FREERTOS_TIMEOUT);
}

void Button::startTimer(ePressType timeout)
{
    ESPARRAG_ASSERT(timeout.get_value() >= ePressType::PRESS_TIMEOUT_1);
    if (timeout.get_value() >= MAX_CALLBACKS)
        return;

    auto &callback = m_pressCallbacks[timeout.get_value()];
    bool callbackValid = callback.cb_function != nullptr && callback.cb_time.value() != 0;
    if (!callbackValid)
        return;

    //callback timeout is the delta between requested time and last callback timeout
    MilliSeconds ms = callback.cb_time - m_pressCallbacks[timeout.get_value() - 1].cb_time;
    BaseType_t higherPriorityExists = pdFALSE;
    xTimerChangePeriodFromISR(m_timer, ms.toTicks(), &higherPriorityExists);
    xTimerStartFromISR(m_timer, &higherPriorityExists);
    YIELD_FROM_ISR_IF(higherPriorityExists);
}

void IRAM_ATTR Button::buttonISR(void *arg)
{
    Button *button = reinterpret_cast<Button *>(arg);
    if (!button->m_sampler.IsValidNow())
        return;

    button->m_buttonState = !button->m_buttonState;
    if (button->m_buttonState)
    {
        button->m_gpi.SetInterruptType(GPIO_INTR_LOW_LEVEL);
        button->receive(PressEvent());
    }
    else
    {
        button->m_gpi.SetInterruptType(GPIO_INTR_HIGH_LEVEL);
        button->receive(ReleaseEvent());
    }
}

void Button::timerCB(xTimerHandle timer)
{
    Button *button = reinterpret_cast<Button *>(pvTimerGetTimerID(timer));
    button->receive(TimerEvent());
}
