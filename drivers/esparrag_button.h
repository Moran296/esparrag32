#ifndef ESPARRAG_BUTTON_H__
#define ESPARRAG_BUTTON_H__

#include "esparrag_common.h"
#include "debouncer.h"
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

class BUTTON : public etl::fsm,
               public etl::instance_count<BUTTON>
{
public:
    struct buttonCB
    {
        void (*m_cb)(void *, uint32_t) = nullptr;
        void *arg = nullptr;
        uint32_t arg2{};
        MilliSeconds m_ms = 0;
        bool operator<(const buttonCB &rhs) const { return m_ms < rhs.m_ms; }
    };

    static constexpr int MAX_CALLBACKS = ePressType::PRESS_NUM;
    static constexpr int BUTTON_DEBOUNCE_TIME_uS = 10000;
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
    bool m_ignoreReleaseCallback = false;
    MicroSeconds m_lastPressTime{};
    Debouncer m_sampler{BUTTON_DEBOUNCE_TIME_uS};

    void runPressCallback(callback_list_t &cb_list, ePressType press);
    void runReleaseCallback(callback_list_t &cb_list);
    void stopTimer(bool fromISR = false);
    void startTimer(ePressType timeout);
    eResult registerEvent(buttonCB &&cb, callback_list_t &cb_list);

    static void buttonISR(void *arg);
    static void timerCB(TimerHandle_t timer);

    IdleState m_idle;
    PressedState m_pressed;
    etl::ifsm_state *stateList[eStateId::NUM]{&m_idle, &m_pressed};

    friend class IdleState;
    friend class PressedState;

    friend class TWO_BUTTONS;
    friend class IdleState_2B;
    friend class PressedState_2B;
};

#endif