#ifndef ESPARRAG_BUTTON_H__
#define ESPARRAG_BUTTON_H__

#include "esparrag_common.h"
#include "debouncer.h"
#include "esparrag_time_units.h"
#include "esparrag_log.h"
#include "esparrag_gpio.h"
#include "etl/array.h"
#include "etl/fsm.h"
#include "etl/enum_type.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

struct ButtonEvent
{
    enum enum_type
    {
        PRESS,
        RELEASE,
        TIMER_EVENT,
        NUM
    };
};

class PressEvent : public etl::message<ButtonEvent::PRESS>
{
};
class ReleaseEvent : public etl::message<ButtonEvent::RELEASE>
{
};
class TimerEvent : public etl::message<ButtonEvent::TIMER_EVENT>
{
};
class InvalidEvent : public etl::message<ButtonEvent::NUM>
{
};

struct eStateId
{
    enum enum_type
    {
        IDLE,
        PRESSED,
        NUM
    };
};

struct ePressType
{
    enum enum_type
    {
        FAST_PRESS,
        PRESS_TIMEOUT_1,
        PRESS_TIMEOUT_2,
        PRESS_NUM
    };

    ETL_DECLARE_ENUM_TYPE(ePressType, uint8_t)
    ETL_ENUM_DEFAULT(FAST_PRESS)
    ETL_ENUM_DEFAULT(PRESS_TIMEOUT_1)
    ETL_ENUM_DEFAULT(PRESS_TIMEOUT_2)
    ETL_ENUM_DEFAULT(PRESS_NUM)
    ETL_END_ENUM_TYPE
};

class Button : public etl::fsm,
               public FSM_COUNT
{
public:
    friend class TwoButtons;

    struct buttonCB
    {
        void (*cb_function)(void *, uint32_t) = nullptr;
        void *cb_arg1 = nullptr;
        uint32_t cb_arg2{};
        MilliSeconds cb_time = 0;
        bool operator<(const buttonCB &rhs) const { return cb_time < rhs.cb_time; }
    };

    static constexpr int MAX_CALLBACKS = ePressType::PRESS_NUM;
    static constexpr int BUTTON_DEBOUNCE_TIME_uS = 1500;
    using callback_list_t = etl::array<buttonCB, MAX_CALLBACKS>;

    /*
        CTOR-
        Note: gpi must be in inactive state at ctor time (active state on startup is not implemented)
        Note: gpi interrupt type must be GPIO_INTR_ANYEDGE
        Note: above conditions are asserted in ctor
    */
    Button(GPI &gpi);
    /*
        Register to press event-
        @param- button cb.
        example: 
        a quick press -------->
        Button::buttonCB a_press = {.cb_function = function_for_fast_press, .cb_arg1 = nullptr, .cb_arg2 = 1, .cb_time = Seconds(3)};
        button.RegisterPress(a_press);
        a longer press -------->
        a_press.cb_function = function_for_longer_press
        a_press.cb_time = Seconds(3);
        button.RegisterPress(a_press);

        Note: cb_time == 0 means event will fire on press.
        Note: registered event can be overwritten if they are in the same time of an previously registered event
    */
    eResult RegisterPress(const buttonCB &cb);
    /*
        Register to release event-
        Refer to press *RegisterPress* documetation
        Note: cb_time == X means event will fire if button released and X time passed
    */
    eResult RegisterRelease(const buttonCB &cb);

    MicroSeconds lastEventTime{};

private:
    GPI &m_gpi;
    callback_list_t m_pressCallbacks{};
    callback_list_t m_releaseCallbacks{};
    xTimerHandle m_timer{};
    bool m_buttonState = false;
    bool m_ignoreReleaseCallback = false;
    MicroSeconds m_lastPressTime{};
    Debouncer m_sampler{BUTTON_DEBOUNCE_TIME_uS};

    void runPressCallback(callback_list_t &cb_list, ePressType press);
    void runReleaseCallback(callback_list_t &cb_list);
    void stopTimer(bool fromISR = false);
    void startTimer(ePressType timeout);
    eResult registerEvent(const buttonCB &cb, callback_list_t &cb_list);

    static void buttonISR(void *arg);
    static void timerCB(TimerHandle_t timer);

    etl::ifsm_state *m_stateList[eStateId::NUM]{&m_idle, &m_pressed};

    //Button states
    class IdleState : public etl::fsm_state<Button, IdleState, eStateId::IDLE,
                                            PressEvent>
    {
    public:
        etl::fsm_state_id_t on_event(const PressEvent &event);
        etl::fsm_state_id_t on_event_unknown(const etl::imessage &event);
    };

    class PressedState : public etl::fsm_state<Button, PressedState, eStateId::PRESSED,
                                               ReleaseEvent,
                                               TimerEvent>
    {
    public:
        etl::fsm_state_id_t on_enter_state();
        etl::fsm_state_id_t on_event(const ReleaseEvent &event);
        etl::fsm_state_id_t on_event(const TimerEvent &event);
        etl::fsm_state_id_t on_event_unknown(const etl::imessage &event);

    private:
        int m_timeouts = 0;
    };

    IdleState m_idle;
    PressedState m_pressed;
};

#endif