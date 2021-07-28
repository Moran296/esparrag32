#include "esparrag_button.h"

class IdleState : public etl::fsm_state<BUTTON, IdleState, eStateId::IDLE,
                                                                PressEvent>
{



};
class PressedState : public etl::fsm_state<BUTTON, PressedState, eStateId::PRESSED,
                                                                    ReleaseEvent,
                                                                    TimerEvent>
{



};
class PressedShortState : public etl::fsm_state<BUTTON, PressedShortState, eStateId::PRESSED_SHORT,
                                                                        ReleaseEvent,
                                                                        TimerEvent>
{



};

class PressedLongState : public etl::fsm_state<BUTTON, PressedLongState, eStateId::PRESSED_LONG,
                                                                        ReleaseEvent,
                                                                        TimerEvent>
{



};

class IdleState2B : public etl::fsm_state<TWO_BUTTONS, IdleState2B, eStateId::IDLE,
                                                                PressEvent>
{



};
class PressedState2B : public etl::fsm_state<TWO_BUTTONS, PressedState2B, eStateId::PRESSED,
                                                                    ReleaseEvent,
                                                                    TimerEvent>
{



};
class PressedShortState2B : public etl::fsm_state<TWO_BUTTONS, PressedShortState2B, eStateId::PRESSED_SHORT,
                                                                        ReleaseEvent,
                                                                        TimerEvent>
{



};

class PressedLongState2B : public etl::fsm_state<TWO_BUTTONS, PressedLongState2B, eStateId::PRESSED_LONG,
                                                                        ReleaseEvent,
                                                                        TimerEvent>
{



};

//========================== BUTTON =============================

BUTTON::BUTTON(GPI& gpi) : fsm(BUTTON_ROUTER), m_gpi(gpi) {}

void BUTTON::runCallback(int index)
{
    if (m_callbacks[index].m_cb != nullptr)
    {
        ESPARRAG_LOG_INFO("running %s event", index >= eStateId::NUM ? "relese" : "press");
        m_callbacks[index].m_cb(m_callbacks[index].arg);
    }
}




//========================== TWO BUTTONS =============================


TWO_BUTTONS::TWO_BUTTONS(BUTTON& b1, BUTTON& b2) : fsm(TWO_BUTTONS_ROUTER), m_b1(b1), m_b2(b2) {}

void BUTTON::runCallback(int index)
{
    if (m_callbacks[index].m_cb != nullptr)
    {
        ESPARRAG_LOG_INFO("running %s event", index >= eStateId::NUM ? "relese" : "press");
        m_callbacks[index].m_cb(m_callbacks[index].arg);
    }
}