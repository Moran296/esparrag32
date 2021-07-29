#include "esparrag_two_buttons.h"
#include "debouncer.h"

class IdleState : public etl::fsm_state<TWO_BUTTONS, IdleState, eStateId::IDLE,
                                        A_PressEvent, A_ReleaseEvent, B_PressEvent, B_ReleaseEvent>
{
public:
    etl::fsm_state_id_t
    on_event(const A_PressEvent &event)
    {
        auto &two_button = get_fsm_context();
        if (two_button.m_b2.m_buttonState)
        {
            two_button.m_b2.m_buttonState = false;
            two_button.m_b2.reset(false); //might be a problem
            two_button.m_b2.start(false);
            two_button.runPressCallback(eStateId::PRESSED);
            return eStateId::PRESSED;
        }

        two_button.m_b1.receive(PressEvent());
        return STATE_ID;
    }
    etl::fsm_state_id_t
    on_event(const B_PressEvent &event)
    {
        auto &two_button = get_fsm_context();
        if (two_button.m_b1.m_buttonState)
        {
            two_button.m_b2.m_buttonState = false;
            two_button.m_b2.reset(false);
            two_button.m_b2.start(false);
            two_button.runPressCallback(eStateId::PRESSED);
            return eStateId::PRESSED;
        }

        two_button.m_b2.receive(PressEvent());
        return STATE_ID;
    }
    etl::fsm_state_id_t
    on_event(const A_ReleaseEvent &event)
    {
        auto &two_button = get_fsm_context();
        two_button.m_b1.receive(ReleaseEvent());
        return STATE_ID;
    }
    etl::fsm_state_id_t
    on_event(const B_ReleaseEvent &event)
    {
        auto &two_button = get_fsm_context();
        two_button.m_b2.receive(ReleaseEvent());
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        return STATE_ID;
    }
};

class PressedState : public etl::fsm_state<BUTTON, PressedState, eStateId::PRESSED,
                                           A_ReleaseEvent, B_ReleaseEvent,
                                           Two_TimerEvent>
{
public:
    etl::fsm_state_id_t
    on_enter_state()
    {
        auto &two_button = get_fsm_context();
        two_button.startTimer(eStateId::PRESSED_SHORT);
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const A_ReleaseEvent &event)
    {
        auto &two_button = get_fsm_context();
        two_button.stopTimer(true);
        two_button.runReleaseCallback();
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event(const B_ReleaseEvent &event)
    {
        auto &two_button = get_fsm_context();
        two_button.stopTimer(true);
        two_button.runReleaseCallback();
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event(const Two_TimerEvent &event)
    {
        auto &two_button = get_fsm_context();
        two_button.runPressCallback(eStateId::PRESSED_SHORT);
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
                                                A_ReleaseEvent, B_ReleaseEvent,
                                                Two_TimerEvent>
{
public:
    etl::fsm_state_id_t
    on_enter_state()
    {
        auto &two_button = get_fsm_context();
        two_button.startTimer(eStateId::PRESSED_LONG);
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const A_ReleaseEvent &event)
    {
        auto &two_button = get_fsm_context();
        two_button.stopTimer(true);
        two_button.runReleaseCallback();
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event(const B_ReleaseEvent &event)
    {
        auto &two_button = get_fsm_context();
        two_button.stopTimer(true);
        two_button.runReleaseCallback();
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event(const Two_TimerEvent &event)
    {
        auto &two_button = get_fsm_context();
        two_button.runPressCallback(eStateId::PRESSED_LONG);
        return eStateId::PRESSED_SHORT;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        ets_printf("pressed unknown %d\n", (uint8_t)get_state_id());
        return STATE_ID;
    }
};

class PressedLongState : public etl::fsm_state<BUTTON, PressedLongState, eStateId::PRESSED_LONG,
                                               A_ReleaseEvent, B_ReleaseEvent>
{
public:
    etl::fsm_state_id_t
    on_enter_state()
    {
        auto &two_button = get_fsm_context();
        two_button.stopTimer();
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const A_ReleaseEvent &event)
    {
        //Release after long duration press
        auto &two_button = get_fsm_context();
        two_button.runReleaseCallback();
        return eStateId::IDLE;
    }
    etl::fsm_state_id_t
    on_event(const B_ReleaseEvent &event)
    {
        //Release after long duration press
        auto &two_button = get_fsm_context();
        two_button.runReleaseCallback();
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

TWO_BUTTONS::TWO_BUTTONS(BUTTON &a, BUTTON &b) : fsm(get_instance_count()), m_b1(a), m_b2(b)
{
    eResult res = m_b1.m_gpi.EnableInterrupt(button1ISR, this);
    ESPARRAG_ASSERT(res == eResult::SUCCESS);
    res = m_b2.m_gpi.EnableInterrupt(button2ISR, this);
    ESPARRAG_ASSERT(res == eResult::SUCCESS);

    m_timer = xTimerCreate("two_buttons", 100, pdFALSE, this, timerCB);
    ESPARRAG_ASSERT(m_timer);
    stateList[eStateId::IDLE] = &m_idle;
    stateList[eStateId::PRESSED] = &m_pressed;
    stateList[eStateId::PRESSED_SHORT] = &m_pressedShort;
    stateList[eStateId::PRESSED_LONG] = &m_pressedLong;
    set_states(stateList, etl::size(stateList));
    start();
}

void TWO_BUTTONS::button1ISR(void *arg)
{
    static Debouncer sampler(3000);
    if (!sampler.IsValidNow())
        return;

    TWO_BUTTONS *two_buttons = reinterpret_cast<TWO_BUTTONS *>(arg);
    if (two_buttons->m_b1.m_buttonState)
        two_buttons->receive(A_ReleaseEvent());
    else
        two_buttons->receive(A_PressEvent());
}

void TWO_BUTTONS::button2ISR(void *arg)
{
    static Debouncer sampler(3000);
    if (!sampler.IsValidNow())
        return;

    TWO_BUTTONS *two_buttons = reinterpret_cast<TWO_BUTTONS *>(arg);
    if (two_buttons->m_b1.m_buttonState)
        two_buttons->receive(B_ReleaseEvent());
    else
        two_buttons->receive(B_PressEvent());
}

void TWO_BUTTONS::timerCB(xTimerHandle timer)
{
    TWO_BUTTONS *two_buttons = reinterpret_cast<TWO_BUTTONS *>(pvTimerGetTimerID(timer));
    two_buttons->receive(Two_TimerEvent());
}

eResult TWO_BUTTONS::registerEvent(BUTTON::buttonCB &&cb, BUTTON::callback_list_t &list)
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

eResult TWO_BUTTONS::RegisterPress(BUTTON::buttonCB &&cb)
{
    return registerEvent(std::move(cb), m_pressCallbacks);
}
eResult TWO_BUTTONS::RegisterRelease(BUTTON::buttonCB &&cb)
{
    return registerEvent(std::move(cb), m_releaseCallbacks);
}

void TWO_BUTTONS::runPressCallback(uint8_t index)
{
    auto &callback = m_pressCallbacks[index];
    if (callback.m_cb != nullptr)
    {
        BaseType_t higherPriorityExists;
        xTimerPendFunctionCallFromISR(callback.m_cb, callback.arg, callback.arg2, &higherPriorityExists);
        YIELD_FROM_ISR_IF(higherPriorityExists);
    }
}

void TWO_BUTTONS::runReleaseCallback()
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

void TWO_BUTTONS::stopTimer(bool fromISR)
{
    if (fromISR)
        xTimerStopFromISR(m_timer, NULL);
    else
        xTimerStop(m_timer, DEFAULT_FREERTOS_TIMEOUT);
}

void TWO_BUTTONS::startTimer(int index)
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