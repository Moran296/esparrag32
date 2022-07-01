#ifndef ESPARRAG_BUTTON_H__
#define ESPARRAG_BUTTON_H__

#include "esparrag_common.h"
#include "real_any_edge.h"
#include "esparrag_time_units.h"
#include "esparrag_log.h"
#include "esparrag_gpio.h"
#include "etl/array.h"
#include "etl/enum_type.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "fsm_taskless.h"

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
struct STATE_IDLE {
    static constexpr const char *NAME = "STATE_IDLE";
};

struct STATE_PRESSED {
    static constexpr const char *NAME = "STATE_PRESSED";

    ePressType m_timeouts{};
};

using ButtonStates = std::variant<STATE_IDLE, STATE_PRESSED>;

struct EVENT_PRESS {
    static constexpr const char *NAME = "EVENT_PRESS";
}; 
struct EVENT_RELEASE 
{
    static constexpr const char *NAME = "EVENT_RELEASE";
};
struct EVENT_TIMER 
{
    static constexpr const char *NAME = "EVENT_RELEASE";
};

using ButtonEvents = std::variant<EVENT_PRESS, EVENT_RELEASE, EVENT_TIMER>;


class Button : public FsmTaskless<Button, ButtonStates, ButtonEvents>
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


    void on_entry(STATE_IDLE &);
    void on_entry(STATE_PRESSED &);

    using return_state_t = std::optional<ButtonStates>;
    
    return_state_t on_event(STATE_IDLE&, EVENT_PRESS&);
    return_state_t on_event(STATE_PRESSED&, EVENT_RELEASE&);
    return_state_t on_event(STATE_PRESSED&, EVENT_TIMER&);

    
    template <class STATE, class EVENT>
    return_state_t on_event(STATE &, EVENT &)
    {
        ets_printf("invalid event - %s - %s \n", STATE::NAME, EVENT::NAME);
        return std::nullopt;
    }




private:
    GPI &m_gpi;
    RealAnyEdge m_realAnyEdge;
    callback_list_t m_pressCallbacks{};
    callback_list_t m_releaseCallbacks{};
    xTimerHandle m_timer{};
    MicroSeconds m_lastPressTime{};

    void runPressCallback(callback_list_t &cb_list, ePressType press);
    void runReleaseCallback(callback_list_t &cb_list);
    void stopTimer(bool fromISR = false);
    void startTimer(ePressType timeout);
    eResult registerEvent(const buttonCB &cb, callback_list_t &cb_list);

    static void timerCB(TimerHandle_t timer);
};

#endif