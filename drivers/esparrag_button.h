#ifndef ESPARRAG_BUTTON_H__
#define ESPARRAG_BUTTON_H__

#include "esparrag_common.h"
#include "esparrag_time_units.h"
#include "esparrag_log.h"
#include "esparrag_gpio.h"
#include "etl/instance_count.h"
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

class BUTTON;

class IdleState : public etl::fsm_state<BUTTON, IdleState, eStateId::IDLE,
                                        PressEvent>
{
public:
    etl::fsm_state_id_t on_enter_state();
    etl::fsm_state_id_t on_event(const PressEvent &event);
    etl::fsm_state_id_t on_event_unknown(const etl::imessage &event);
};

class PressedState : public etl::fsm_state<BUTTON, PressedState, eStateId::PRESSED,
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

class BUTTON : public etl::fsm, public etl::instance_count<BUTTON>
{
public:
    struct buttonCB
    {
        void (*m_cb)(void *, uint32_t) = nullptr;
        void *arg = nullptr;
        uint32_t arg2{};
        MilliSeconds m_ms = 0;
    };

    enum ePressTypes
    {
        FAST_PRESS,
        PRESS_TIMEOUT_1,
        PRESS_TIMEOUT_2,
        PRESS_NUM
    };

    static constexpr int MAX_CALLBACKS = PRESS_NUM;
    using callback_list_t = etl::array<buttonCB, MAX_CALLBACKS>;

    BUTTON(GPI &gpi);
    eResult RegisterPress(buttonCB &&cb);
    eResult RegisterRelease(buttonCB &&cb);

private:
    GPI &m_gpi;
    callback_list_t m_pressCallbacks{};
    callback_list_t m_releaseCallbacks{};
    xTimerHandle m_timer{};
    bool m_buttonState = false;
    MicroSeconds m_lastPressTime{};

    void runPressCallback(uint8_t index);
    void runReleaseCallback();
    void stopTimer(bool fromISR = false);
    void startTimer(int index);
    eResult registerEvent(buttonCB &&cb, callback_list_t &list);

    static void buttonISR(void *arg);
    static void timerCB(TimerHandle_t timer);

    IdleState m_idle;
    PressedState m_pressed;
    etl::ifsm_state *stateList[eStateId::NUM]{&m_idle, &m_pressed};

    friend class IdleState;
    friend class PressedState;
    friend class TWO_BUTTONS;
};

// class TWO_BUTTONS : public etl::fsm
// {
// public:
//     TWO_BUTTONS(BUTTON &b1, BUTTON &b2);

// private:
//     BUTTON &m_b1;
//     BUTTON &m_b2;
//     BUTTON::callback_list_t m_callbacks{};
//     bool ignoreNextRelease = false;

//     void runCallback(int index);
//     void stopTimer();
//     void startShortPressTimer();
//     void startLongPressTimer();

//     static void buttonISR(void *arg);
//     friend class IdleState2B;
//     friend class PressedState2B;
//     friend class PressedShortState2B;
//     friend class PressedLongState2B;
// };

#endif