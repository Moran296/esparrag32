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

struct TwoButtonsEvent
{
    enum enum_type
    {
        A_PRESS,
        B_PRESS,
        A_RELEASE,
        B_RELEASE,
        TIMER_EVENT,
        NUM
    };
};

class A_PressEvent : public etl::message<TwoButtonsEvent::A_PRESS>
{
};
class B_PressEvent : public etl::message<TwoButtonsEvent::B_PRESS>
{
};
class A_ReleaseEvent : public etl::message<TwoButtonsEvent::A_RELEASE>
{
};
class B_ReleaseEvent : public etl::message<TwoButtonsEvent::B_RELEASE>
{
};
class Two_TimerEvent : public etl::message<TwoButtonsEvent::TIMER_EVENT>
{
};

class TWO_BUTTONS : public etl::fsm, public etl::instance_count<BUTTON> //for fsm router num
{
public:
    TWO_BUTTONS(BUTTON &a, BUTTON &b);
    void Register();
    eResult RegisterPress(BUTTON::buttonCB &&cb);
    eResult RegisterRelease(BUTTON::buttonCB &&cb);

private:
    BUTTON &m_b1;
    BUTTON &m_b2;
    bool m_state = false;

    BUTTON::callback_list_t m_pressCallbacks{};
    BUTTON::callback_list_t m_releaseCallbacks{};
    xTimerHandle m_timer{};
    MicroSeconds m_lastPressTime{};

    void runPressCallback(uint8_t index);
    void runReleaseCallback();
    void stopTimer(bool fromISR = false);
    void startTimer(int index);
    eResult registerEvent(BUTTON::buttonCB &&cb, BUTTON::callback_list_t &list);

    static void timerCB(TimerHandle_t timer);
    etl::ifsm_state *stateList[eStateId::NUM];
    static void button1ISR(void *arg);
    static void button2ISR(void *arg);

    friend class IdleState;
    friend class PressedState;
    friend class PressedShortState;
    friend class PressedLongState;
};

#endif