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
    on_event(const PressEvent &event)
    {
        ets_printf("idle got press\n");
        auto &button = get_fsm_context();
        button.runCallback(eStateId::PRESSED);
        button.startTimer(eStateId::PRESSED_SHORT, true);
        return eStateId::PRESSED;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        ets_printf("idle got release");
        //ESPARRAG_LOG_ERROR("unknown event in idle state");
        return STATE_ID;
    }
};
class PressedState : public etl::fsm_state<BUTTON, PressedState, eStateId::PRESSED,
                                           ReleaseEvent,
                                           TimerEvent>
{
public:
    etl::fsm_state_id_t
    on_event(const ReleaseEvent &event)
    {
        //Release after fast duration press
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
        auto &button = get_fsm_context();
        button.runCallback(eStateId::PRESSED_SHORT);
        button.startTimer(eStateId::PRESSED_LONG, true);
        return eStateId::PRESSED_SHORT;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        //ESPARRAG_LOG_ERROR("unknown event in pressed state");
        return STATE_ID;
    }
};
class PressedShortState : public etl::fsm_state<BUTTON, PressedShortState, eStateId::PRESSED_SHORT,
                                                ReleaseEvent,
                                                TimerEvent>
{
public:
    etl::fsm_state_id_t
    on_event(const ReleaseEvent &event)
    {
        //Release after short duration press
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
        auto &button = get_fsm_context();
        button.runCallback(eStateId::PRESSED_LONG);
        return eStateId::PRESSED_LONG;
    }

    etl::fsm_state_id_t
    on_event_unknown(const etl::imessage &event)
    {
        //ESPARRAG_LOG_ERROR("unknown event in pressedShortState state");
        return STATE_ID;
    }
};

class PressedLongState : public etl::fsm_state<BUTTON, PressedLongState, eStateId::PRESSED_LONG,
                                               ReleaseEvent>
{
public:
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
    etl::ifsm_state *stateList[eStateId::NUM] = {&m_idle, &m_pressed, &m_pressedShort, &m_pressedLong};
    set_states(stateList, etl::size(stateList));
    start();
}

eResult BUTTON::registerEvent(buttonCB &&cb, int mod)
{
    ESPARRAG_ASSERT(cb.m_cb != nullptr);
    if (cb.m_ms.value() == 0)
    {
        m_callbacks[eStateId::PRESSED * mod] = cb;
        ESPARRAG_LOG_ERROR("registered as fast %s", mod == PRESS_MODIFIER ? "press" : "release");
        return eResult::SUCCESS;
    }
    else if (m_callbacks[eStateId::PRESSED_SHORT * mod].m_cb == nullptr)
    {
        m_callbacks[eStateId::PRESSED_SHORT * mod] = cb;
        ESPARRAG_LOG_ERROR("registered as short %s", mod == PRESS_MODIFIER ? "press" : "release");
        return eResult::SUCCESS;
    }
    else if (m_callbacks[eStateId::PRESSED_LONG * mod].m_cb == nullptr)
    {
        if (m_callbacks[eStateId::PRESSED_SHORT * mod].m_ms > cb.m_ms)
        {
            ESPARRAG_LOG_ERROR("registered as short %s", mod == PRESS_MODIFIER ? "press" : "release");
            m_callbacks[eStateId::PRESSED_LONG * mod] = m_callbacks[eStateId::PRESSED_SHORT];
            m_callbacks[eStateId::PRESSED_SHORT * mod] = cb;
        }

        else
        {

            ESPARRAG_LOG_ERROR("registered as long %s", mod == PRESS_MODIFIER ? "press" : "release");
            m_callbacks[eStateId::PRESSED_LONG * mod] = cb;
        }

        return eResult::SUCCESS;
    }

    ESPARRAG_LOG_ERROR("couldnt find place for callback");
    return eResult::ERROR_INVALID_STATE;
}

eResult BUTTON::RegisterPress(buttonCB &&cb)
{
    return registerEvent(std::move(cb), PRESS_MODIFIER);
}
eResult BUTTON::RegisterRelease(buttonCB &&cb)
{
    return registerEvent(std::move(cb), RELEASE_MODIFIER);
}

void BUTTON::runCallback(int index)
{
    auto &callback = m_callbacks[index];
    if (callback.m_cb != nullptr)
    {
        ets_printf("lets run func\n");
        BaseType_t higher;
        xTimerPendFunctionCallFromISR(callback.m_cb, callback.arg, callback.arg2, &higher);
        portYIELD_FROM_ISR(higher);
        ets_printf("run it!\n");
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
    static Debouncer sampler(1000000);
    if (!sampler.IsValidNow())
        return;

    ets_printf("in isr");
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