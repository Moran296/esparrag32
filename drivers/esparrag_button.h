#ifndef ESPARRAG_BUTTON_H__
#define ESPARRAG_BUTTON_H__

#include "esparrag_common.h"
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

    //ETL_DECLARE_ENUM_TYPE()
};

//find solution for this
const etl::message_router_id_t BUTTON_ROUTER = 0;
const etl::message_router_id_t TWO_BUTTONS_ROUTER = 1;

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
        PRESSED_SHORT,
        PRESSED_LONG,
        NUM
    };
};

class BUTTON : public etl::fsm
{
public:
    struct buttonCB
    {
        void (*m_cb)(void *, uint32_t) = nullptr;
        void *arg = nullptr;
        uint32_t arg2{};
        MilliSeconds m_ms = 0;
    };

    static constexpr int MAX_CALLBACKS = eStateId::NUM * 2;
    using callback_list_t = etl::array<buttonCB, eStateId::NUM>;

    BUTTON(GPI &gpi);
    eResult RegisterPress(buttonCB &&cb);
    eResult RegisterRelease(buttonCB &&cb);

private:
    GPI &m_gpi;
    callback_list_t m_pressCallbacks{};
    callback_list_t m_releaseCallbacks{};
    xTimerHandle m_timer{};
    bool m_ignoreNextRelease = false;
    MicroSeconds m_pressTime{};

    void runPressCallback(uint8_t index);
    void runReleaseCallback();
    void stopTimer(bool fromISR = false);
    void startTimer(int index, bool fromISR = false);
    eResult registerEvent(buttonCB &&cb, callback_list_t& list);

    static void buttonISR(void *arg);
    static void timerCB(TimerHandle_t timer);
    etl::ifsm_state *stateList[eStateId::NUM];

    friend class IdleState;
    friend class PressedState;
    friend class PressedShortState;
    friend class PressedLongState;
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