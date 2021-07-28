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
    enum enum_type {
    PRESS,
    RELEASE,
    TIMER_EVENT,
    NUM
    };

    //ETL_DECLARE_ENUM_TYPE()
};

const etl::message_router_id_t BUTTON_ROUTER = 0;
const etl::message_router_id_t TWO_BUTTONS_ROUTER = 1;

class PressEvent : public etl::message<ButtonEvent::PRESS> {};
class ReleaseEvent : public etl::message<ButtonEvent::RELEASE> {};
class TimerEvent : public etl::message<ButtonEvent::TIMER_EVENT> {};
class InvalidEvent : public etl::message<ButtonEvent::NUM> {};

struct eStateId{
    enum  enum_type{
        IDLE,
        PRESSED,
        PRESSED_SHORT,
        PRESSED_LONG,
        NUM
    };
};

#define RELEASED_AFTER(PRESSED_STATE) (eStateId::PRESSED_STATE * 2) //for indexing

struct buttonCB {
    void(*m_cb)(void*) = nullptr;
    void* arg = nullptr;
    MilliSeconds m_ms = 0;
};

static constexpr int MAX_CALLBACKS = eStateId::NUM * 2;
using callback_list_t = etl::array<buttonCB, MAX_CALLBACKS>;

class BUTTON : public etl::fsm
{
    public:
    BUTTON(GPI& gpi);


    private:
    GPI& m_gpi;
    callback_list_t m_callbacks{};
    xTimerHandle m_timer{};
    bool ignoreNextRelease = false;


    void runCallback(int index);
    void stopTimer();
    void startShortPressTimer();
    void startLongPressTimer();
    void ignoreNextRelease();

    static void buttonISR(void* arg);
    friend class IdleState;
    friend class PressedState;
    friend class PressedShortState;
    friend class PressedLongState;
};

class TWO_BUTTONS : public etl::fsm
{
    public:
    TWO_BUTTONS(BUTTON& b1, BUTTON& b2);

    private:
    BUTTON& m_b1;
    BUTTON& m_b2;
    callback_list_t m_callbacks{};
    bool ignoreNextRelease = false;

    void runCallback(int index);
    void stopTimer();
    void startShortPressTimer();
    void startLongPressTimer();
    void ignoreNextRelease();

    static void buttonISR(void* arg);
    friend class IdleState2B;
    friend class PressedState2B;
    friend class PressedShortState2B;
    friend class PressedLongState2B;
};







#endif