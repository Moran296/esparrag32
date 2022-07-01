#include "esparrag_button.h"
#include "etl/algorithm.h"

// ================== STATE MACHINE =================

using return_state_t = std::optional<ButtonStates>;

//============================BUTTON IDLE ======================================

void Button::on_entry(STATE_IDLE &state) {
}

return_state_t Button::on_event(STATE_IDLE& state, EVENT_PRESS& event) {
    return STATE_PRESSED{};
}

//============================BUTTON PRESSEED ======================================

void Button::on_entry(STATE_PRESSED &state) {
    runPressCallback(m_pressCallbacks, ePressType::FAST_PRESS);

    state.m_timeouts = ePressType::PRESS_TIMEOUT_1;
    m_lastPressTime = esp_timer_get_time();
    startTimer(state.m_timeouts);
}

return_state_t Button::on_event(STATE_PRESSED& state, EVENT_RELEASE& event) {
    stopTimer(true);
    runReleaseCallback(m_releaseCallbacks);
    return STATE_IDLE{};
}

return_state_t Button::on_event(STATE_PRESSED& state, EVENT_TIMER& event) {
    runPressCallback(m_pressCallbacks, state.m_timeouts);

    state.m_timeouts = ePressType(state.m_timeouts.get_value() + 1);
    startTimer(state.m_timeouts);
    return std::nullopt;
}


// ==========Button==============

Button::Button(GPI &gpi) : m_gpi(gpi),
                           m_realAnyEdge(gpi, MilliSeconds(30)) 
{
    m_timer = xTimerCreate("button", 100, pdFALSE, this, timerCB);
    ESPARRAG_ASSERT(m_timer);

    Start(gpi.IsActive() ? ButtonStates{STATE_PRESSED{}} : ButtonStates{STATE_IDLE{}});

    m_realAnyEdge.RegisterCallback([this](bool isActive) {
        if (isActive) {
            this->Dispatch(EVENT_PRESS{});
        } else {
            this->Dispatch(EVENT_RELEASE{});
        }
    });
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
        etl::sort(cb_list.begin(), cb_list.begin() + i + 1);
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
            ESPARRAG_LOG_INFO("event written as long press");
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

void Button::timerCB(xTimerHandle timer)
{
    Button *button = reinterpret_cast<Button *>(pvTimerGetTimerID(timer));
    button->Dispatch(EVENT_TIMER{});
}
