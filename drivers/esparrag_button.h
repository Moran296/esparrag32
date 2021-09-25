#ifndef ESPARRAG_BUTTON_H__
#define ESPARRAG_BUTTON_H__

#include "esparrag_common.h"
#include "debouncer.h"
#include "esparrag_time_units.h"
#include "esparrag_log.h"
#include "esparrag_gpio.h"
#include "etl/array.h"
#include "etl/enum_type.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#define CALL_ON_STATE_ENTRY 1
#include "fsm_task.h"

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

class PressEvent
{
};
class ReleaseEvent
{
};
class TimerEvent
{
};

using ButtonEventVariant = std::variant<PressEvent, ReleaseEvent, TimerEvent>;

struct IdleState
{
};
struct PressedState
{
    int m_timeouts = 0;
};

using ButtonStateVariant = std::variant<IdleState, PressedState>;

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

class Button : public FsmTask<Button, ButtonStateVariant, ButtonEventVariant>
{
public:
    //friend class TwoButtons;

    struct buttonCB
    {
        void (*cb_function)(void *, uint32_t) = nullptr;
        void *cb_arg1 = nullptr;
        uint32_t cb_arg2{};
        MilliSeconds cb_time = 0;
        bool operator<(const buttonCB &rhs) const { return cb_time < rhs.cb_time; }
    };

    static constexpr int MAX_CALLBACKS = ePressType::PRESS_NUM;
    static constexpr int BUTTON_DEBOUNCE_TIME_uS = 3500;
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

    template <class State>
    void on_entry(const State &) const {}
    //default on event handler
    template <class State, class Event>
    auto on_event(const State &, const Event &) const
    {
        ets_printf("pressed unknown \n");
        return std::nullopt;
    }

    //idle state events
    auto on_event(IdleState &, PressEvent &);
    //pressed state events
    void on_entry(PressedState &);
    auto on_event(PressedState &, ReleaseEvent &);
    auto on_event(PressedState &, TimerEvent &);

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

    //Button states
};

#endif