#include "esparrag_button.h"
#include "debouncer.h"
#define RELEASED_AFTER(PRESSED_STATE) (PRESSED_STATE * 2) //for indexing
#define PRESS_MODIFIER 1
#define RELEASE_MODIFIER 2

//========================== BUTTON =============================
class IdleState : public etl::fsm_state<BUTTON, IdleState, eStateId::IDLE,
                                        PressEvent>
{
public:
    etl::fsm_state_id_t
    on_enter_state()
    {
        ets_printf("in idle state\n");
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const PressEvent &event)
    {
        ets_printf("idle got press\n");
        auto &button = get_fsm_context();
        button.m_pressTime = esp_timer_get_time();
        button.runCallback(eStateId::PRESSED);
        return eStateId::PRESSED;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        ets_printf("idle got release\n");
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
        ets_printf("in pressed state\n");
        auto &button = get_fsm_context();
        button.startTimer(eStateId::PRESSED_SHORT, true);
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const ReleaseEvent &event)
    {
        //Release after fast duration press
        ets_printf("pressed got release\n");
        auto &button = get_fsm_context();
        button.stopTimer(true);
        if (!button.m_ignoreNextRelease)
            button.runCallback(RELEASED_AFTER(eStateId::PRESSED));
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event(const TimerEvent &event)
    {
        //I am in pressed state and timer called - button was pressed short
        ets_printf("pressed got timer\n");
        auto &button = get_fsm_context();
        button.runCallback(eStateId::PRESSED_SHORT);
        return eStateId::PRESSED_SHORT;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        //ESPARRAG_LOG_ERROR("unknown event in pressed state");
        ets_printf("pressed got unknown\n");
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
        ets_printf("in pressed short state\n");
        auto &button = get_fsm_context();
        button.startTimer(eStateId::PRESSED_LONG, true);
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const ReleaseEvent &event)
    {
        //Release after short duration press
        ets_printf("release timer\n");
        auto &button = get_fsm_context();
        button.stopTimer();
        if (!button.m_ignoreNextRelease)
            button.runCallback(RELEASED_AFTER(eStateId::PRESSED_SHORT));
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event(const TimerEvent &event)
    {
        //I am in pressed short state and timer called - button was pressed long
        ets_printf("short timer\n");
        auto &button = get_fsm_context();
        button.runCallback(eStateId::PRESSED_LONG);
        return eStateId::PRESSED_LONG;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        //ESPARRAG_LOG_ERROR("unknown event in pressedShortState state");
        ets_printf("short unknown\n");
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
        ets_printf("in long pressed state\n");
        auto &button = get_fsm_context();
        button.stopTimer();
        return STATE_ID;
    }

    etl::fsm_state_id_t
    on_event(const ReleaseEvent &event)
    {
        //Release after long duration press
        auto &button = get_fsm_context();
        button.stopTimer();
        if (!button.m_ignoreNextRelease)
            button.runCallback(RELEASED_AFTER(eStateId::PRESSED_LONG));
        return eStateId::IDLE;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        //ESPARRAG_LOG_ERROR("unknown event in pressedLongState state");
        return STATE_ID;
    }
};

static IdleState m_idle;
static PressedState m_pressed;
static PressedShortState m_pressedShort;
static PressedLongState m_pressedLong;

BUTTON::BUTTON(GPI &gpi) : fsm(BUTTON_ROUTER), m_gpi(gpi)
{
    eResult res = m_gpi.EnableInterrupt(buttonISR, this);
    ESPARRAG_ASSERT(res == eResult::SUCCESS);
    m_timer = xTimerCreate("button", 100, pdFALSE, this, timerCB);
    ESPARRAG_ASSERT(m_timer);
    stateList[0] = &m_idle;
    stateList[1] = &m_pressed;
    stateList[2] = &m_pressedShort;
    stateList[3] = &m_pressedLong;
    set_states(stateList, etl::size(stateList));
    start();
}

eResult BUTTON::registerEvent(buttonCB &&cb, callback_list_t& list)
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
        ets_printf("lets run func\n");
        BaseType_t higher;
        xTimerPendFunctionCallFromISR(callback.m_cb, callback.arg, callback.arg2, &higher);
        portYIELD_FROM_ISR(higher);
    }
}

void BUTTON::runReleaseCallback()
{
    MicroSeconds timeNow = esp_timer_get_time();
    for (size_t i = eStateId::PRESSED; i < eStateId::NUM; i++)
    {
        if ()
    }

}

void BUTTON::stopTimer(bool fromISR)
{
    if (fromISR)
        xTimerStopFromISR(m_timer, NULL);
    else
        xTimerStop(m_timer, DEFAULT_FREERTOS_TIMEOUT);
}

void BUTTON::startTimer(int index, bool fromISR)
{
    ets_printf("start timer\n");
    auto &callback = m_callbacks[index];
    bool callbackValid = callback.m_cb != nullptr && callback.m_ms.value() != 0;
    if (!callbackValid)
        return;

    MilliSeconds timeout;
    switch (index)
    {
    case eStateId::PRESSED_SHORT:
        timeout = callback.m_ms;
        ets_printf("1\n");
        break;
    case eStateId::PRESSED_LONG:
        timeout = callback.m_ms - m_callbacks[eStateId::PRESSED_SHORT].m_ms;
        ets_printf("2\n");
        break;
    default:
        ets_printf("3\n");
        ESPARRAG_LOG_ERROR("unidentified callback for timer %d", index);
        return;
    }

    if (fromISR)
    {
        BaseType_t higherPriority = pdFALSE;
        ets_printf("change time\n");
        xTimerChangePeriodFromISR(m_timer, timeout.toTicks(), &higherPriority);
        ets_printf("start time\n");
        xTimerStartFromISR(m_timer, &higherPriority);
        ets_printf("yield\n");
        portYIELD_FROM_ISR(higherPriority);
    }
    else
    {
        xTimerChangePeriod(m_timer, timeout.toTicks(), DEFAULT_FREERTOS_TIMEOUT);
        xTimerStart(m_timer, DEFAULT_FREERTOS_TIMEOUT);
    }
}

void BUTTON::buttonISR(void *arg)
{
    static Debouncer sampler;
    if (!sampler.IsValidNow())
        return;

    ets_printf("in isr\n");
    BUTTON *button = reinterpret_cast<BUTTON *>(arg);
    bool isPressed = button->m_gpi.IsActive();
    if (isPressed)
        button->receive(PressEvent());
    else
        button->receive(ReleaseEvent());
}

void BUTTON::timerCB(xTimerHandle timer)
{
    BUTTON *button = reinterpret_cast<BUTTON *>(pvTimerGetTimerID(timer));
    button->receive(TimerEvent());
}

// //========================== TWO BUTTONS =============================

// class IdleState2B : public etl::fsm_state<TWO_BUTTONS, IdleState2B, eStateId::IDLE,
//                                           PressEvent>
// {
// };
// class PressedState2B : public etl::fsm_state<TWO_BUTTONS, PressedState2B, eStateId::PRESSED,
//                                              ReleaseEvent,
//                                              TimerEvent>
// {
// };
// class PressedShortState2B : public etl::fsm_state<TWO_BUTTONS, PressedShortState2B, eStateId::PRESSED_SHORT,
//                                                   ReleaseEvent,
//                                                   TimerEvent>
// {
// };

// class PressedLongState2B : public etl::fsm_state<TWO_BUTTONS, PressedLongState2B, eStateId::PRESSED_LONG,
//                                                  ReleaseEvent,
//                                                  TimerEvent>
// {
// };

// TWO_BUTTONS::TWO_BUTTONS(BUTTON &b1, BUTTON &b2) : fsm(TWO_BUTTONS_ROUTER), m_b1(b1), m_b2(b2) {}

// void BUTTON::runCallback(int index)
// {
//     if (m_callbacks[index].m_cb != nullptr)
//     {
//         ESPARRAG_LOG_INFO("running %s event", index >= eStateId::NUM ? "relese" : "press");
//         m_callbacks[index].m_cb(m_callbacks[index].arg, m_callbacks[index].arg2);
//     }
// }