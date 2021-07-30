#ifndef ESPARRAG_TWO_BUTTONS_H__
#define ESPARRAG_TWO_BUTTONS_H__

#include "esparrag_common.h"
#include "esparrag_button.h"
#include "esparrag_time_units.h"
#include "esparrag_log.h"
#include "esparrag_gpio.h"
#include "etl/array.h"
#include "etl/fsm.h"
#include "etl/enum_type.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "etl/instance_count.h"

enum eButtonID
{
    BUTTON_A,
    BUTTON_B,
    BUTTON_MAX
};

class PressEvent_2B : public etl::message<ButtonEvent::PRESS>
{
public:
    PressEvent_2B(eButtonID id) : m_buttonId(id){ESPARRAG_ASSERT(id < BUTTON_MAX)};
    eButtonID m_buttonId;
};

class ReleaseEvent_2B : public etl::message<ButtonEvent::RELEASE>
{
public:
    ReleaseEvent_2B(eButtonID id) : m_buttonId(id){ESPARRAG_ASSERT(id < BUTTON_MAX)};
    eButtonID m_buttonId;
};

class TimerEvent_2B : public etl::message<ButtonEvent::TIMER_EVENT>
{
};

class TwoButtons;

class IdleState_2B : public etl::fsm_state<TwoButtons, IdleState_2B, eStateId::IDLE,
                                           PressEvent_2B, ReleaseEvent_2B>
{
public:
    etl::fsm_state_id_t on_enter_state();
    etl::fsm_state_id_t on_event(const PressEvent_2B &event);
    etl::fsm_state_id_t on_event(const ReleaseEvent_2B &event);
    etl::fsm_state_id_t on_event_unknown(const etl::imessage &event);

private:
    bool m_isButton1Pressed = false;
    bool m_isButton2Pressed = false;
};

class PressedState_2B : public etl::fsm_state<TwoButtons, PressedState_2B, eStateId::PRESSED,
                                              TimerEvent_2B, ReleaseEvent_2B>
{
public:
    etl::fsm_state_id_t on_enter_state();
    etl::fsm_state_id_t on_event(const ReleaseEvent_2B &event);
    etl::fsm_state_id_t on_event(const TimerEvent_2B &event);
    etl::fsm_state_id_t on_event_unknown(const etl::imessage &event);

private:
    bool m_isButton1Released = false;
    bool m_isButton2Released = false;
    int m_timeouts = 0;
};

class TwoButtons : public etl::fsm, public etl::instance_count<Button> //for fsm router num
{
public:
    /*
        Ctor-
        @param- Two buttons in inactive state
    */
    TwoButtons(Button &a, Button &b);
    /*
        Register to press event-
        @param- button cb.
        example: 
        Button::buttonCB press_short_time = {.cb_function = goo, .cb_arg2 = SHORT_PRESS, .cb_time = Seconds(3)};
        two_buttons.RegisterPress(std::move(press_short_time));

        Note: cb_time == 0 means event will fire on press.
        Note: registered event can be overwritten if they are in the same time of an previously registered event
        Note: buttons won't fire release callbacks when released after been in a two buttons event
    */
    eResult RegisterPress(Button::buttonCB &&cb);
    /*
        Register to release event-
        Like press event.
        Note: cb_time == X means event will fire if one of the two buttons released and X time passed
    */
    eResult RegisterRelease(Button::buttonCB &&cb);

private:
    Button &m_b1;
    Button &m_b2;
    bool m_state = false;

    Button::callback_list_t m_pressCallbacks{};
    Button::callback_list_t m_releaseCallbacks{};
    xTimerHandle m_timer{};
    MicroSeconds m_lastPressTime{};
    IdleState_2B m_idle;
    PressedState_2B m_pressed;
    etl::ifsm_state *stateList[eStateId::NUM]{&m_idle, &m_pressed};
    Debouncer m_sampler{Button::BUTTON_DEBOUNCE_TIME_uS};

    void stopTimer(bool fromISR = false);
    void startTimer(ePressType timeout);

    static void timerCB(TimerHandle_t timer);
    static void button1ISR(void *arg);
    static void button2ISR(void *arg);

    friend class IdleState_2B;
    friend class PressedState_2B;
};

#endif