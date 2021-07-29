#include "esparrag_button.h"
#include "debouncer.h"

class IdleState : public etl::fsm_state<BUTTON, IdleState, eStateId::IDLE,
                                        PressEvent>
{
public:
    etl::fsm_state_id_t
    on_enter_state()
    {
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const PressEvent &event)
    {
        auto &button = get_fsm_context();
        button.m_lastPressTime = esp_timer_get_time();
        button.runPressCallback(eStateId::PRESSED);
        return eStateId::PRESSED;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        return STATE_ID;
    }
};
class PressedState : public etl::fsm_state<BUTTON, PressedState, eStateId::PRESSED,
                                           ReleaseEvent,
                                           TimerEvent>
{
public:
    etl::fsm_state_id_t
    on_enter_state()
    {
        auto &button = get_fsm_context();
        button.startTimer(eStateId::PRESSED_SHORT);
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const ReleaseEvent &event)
    {
        auto &button = get_fsm_context();
        button.stopTimer(true);
        button.runReleaseCallback();
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event(const TimerEvent &event)
    {
        auto &button = get_fsm_context();
        button.runPressCallback(eStateId::PRESSED_SHORT);
        return eStateId::PRESSED_SHORT;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        ets_printf("pressed unknown %d\n", (uint8_t)get_state_id());
        return STATE_ID;
    }
};
class PressedShortState : public etl::fsm_state<BUTTON, PressedShortState, eStateId::PRESSED_SHORT,
                                                ReleaseEvent,
                                                TimerEvent>
{
public:
    etl::fsm_state_id_t
    on_enter_state()
    {
        auto &button = get_fsm_context();
        button.startTimer(eStateId::PRESSED_LONG);
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const ReleaseEvent &event)
    {
        auto &button = get_fsm_context();
        button.stopTimer();
        button.runReleaseCallback();
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event(const TimerEvent &event)
    {
        auto &button = get_fsm_context();
        button.runPressCallback(eStateId::PRESSED_LONG);
        return eStateId::PRESSED_LONG;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        ets_printf("short unknown %d\n", (uint8_t)get_state_id());
        return STATE_ID;
    }
};

class PressedLongState : public etl::fsm_state<BUTTON, PressedLongState, eStateId::PRESSED_LONG,
                                               ReleaseEvent>
{
public:
    etl::fsm_state_id_t
    on_enter_state()
    {
        auto &button = get_fsm_context();
        button.stopTimer();
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const ReleaseEvent &event)
    {
        //Release after long duration press
        auto &button = get_fsm_context();
        button.runReleaseCallback();
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        ets_printf("unknown long pressed\n");
        return STATE_ID;
    }
};

static IdleState m_idle;
static PressedState m_pressed;
static PressedShortState m_pressedShort;
static PressedLongState m_pressedLong;

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
    stateList[eStateId::IDLE] = &m_idle;
    stateList[eStateId::PRESSED] = &m_pressed;
    stateList[eStateId::PRESSED_SHORT] = &m_pressedShort;
    stateList[eStateId::PRESSED_LONG] = &m_pressedLong;
    set_states(stateList, etl::size(stateList));
    start();
}

eResult BUTTON::registerEvent(buttonCB &&cb, callback_list_t &list)
{
    ESPARRAG_ASSERT(cb.m_cb != nullptr);
    if (cb.m_ms.value() == 0)
    {
        list[eStateId::PRESSED] = cb;
        ESPARRAG_LOG_ERROR("registered as fast");
        return eResult::SUCCESS;
    }
    else if (list[eStateId::PRESSED_SHORT].m_cb == nullptr)
    {
        list[eStateId::PRESSED_SHORT] = cb;
        ESPARRAG_LOG_ERROR("registered as short");
        return eResult::SUCCESS;
    }
    else if (list[eStateId::PRESSED_LONG].m_cb == nullptr)
    {
        if (list[eStateId::PRESSED_SHORT].m_ms > cb.m_ms)
        {
            ESPARRAG_LOG_ERROR("registered as short");
            list[eStateId::PRESSED_LONG] = list[eStateId::PRESSED_SHORT];
            list[eStateId::PRESSED_SHORT] = cb;
        }

        else
        {

            ESPARRAG_LOG_ERROR("registered as long");
            list[eStateId::PRESSED_LONG] = cb;
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

    for (size_t i = eStateId::PRESSED_LONG; i > eStateId::IDLE; i--)
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
    ESPARRAG_ASSERT(index >= eStateId::PRESSED_SHORT);
    ESPARRAG_ASSERT(index <= eStateId::PRESSED_LONG);

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
    static Debouncer sampler(3000);
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
