#ifndef __FSM_TASK_H__
#define __FSM_TASK_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <variant>
#include <optional>

/*  Finite state machine running over a freertos task.
    This implementation of the fsm class is based on Mateusz Pusz mpusz/fsm-variant repository presented in his cppCon talk.
    The difference is this implemantation runs under a freertos task.

    In order to use this class, one must:
    1. first create the structs/classes used as events and states.
    2. ordered states/events in a variant, the first index in the state variant is the entry state.
    3. create a class which inherits the fsm task variants and itself (CRTP) as template arguments, and task info as ctor args
    Example for a button fsm:
    //EVENTS:
    struct event_press {};
    struct event_release {};
    struct event_timer {int seconds}
    using events = std::variant<event_press, event_release, event_timer>

    //STATES:
    struct state_idle {};
    struct state_pressed {};
    using states = std::variant<state_idle, state_pressed>

    //STATE_MACHINE
    class ButtonFSM : public FsmTask<ButtonFSM, states, events>
    {
    public:
        ButtonFSM() : FsmTask(2048, 3, "button_fsm") {}

        //DEFAULT_HANDLER, will be invoked for undefined state/events
        template <typename State, typename Event>
        auto on_event(State &, const Event &)
        {
            printf("got an unknown event!");
            return std::nullopt; //return null option because state havn't changed
        }

        //handler for idle state, press event
        auto on_event(state_idle &, const event_press)
        {
            printf("state idle got press event!");
            return state_pressed{}; //state have changed
        }

        //handler for pressed state, timer event
        auto on_event(state_pressed &, const event_timer &event)
        {
            printf("state pressed got timer event after %d seconds!", event.seconds);
            return std::nullopt;
        }

        //handler for pressed state, release event
        auto on_event(state_pressed &, const event_release &)
        {
            printf("state pressed got release event");
            return state_idle{};
        }

        // We can also define state entry and exit handlers if we defined the requested macros (CALL_ON_STATE_EXIT / CALL_ON_STATE_ENTRY)

        //default handler for state entry
        template <class STATE>
        void on_entry(STATE& ) {}

        void on_entry(state_pressed &) {
            //do state entry logic
        }

        //default handler for state exit
        template <class STATE>
        void on_exit(STATE& ) {}

        void on_exit(state_idle &) {
            //do state exit logic
        }
    };

    ButtonFSM button;
    button.Start();

    bool dispatchedSuccessfully = button.Dispatch(press_event{});
    configASSERT(dispatchedSuccessfully); // Check if dispatched successfully, otherwise might be better to enlarge event queue
    configASSERT(button.IsInState<state_pressed>());

    button.Dispatch(timer_event{3_sec});
    configASSERT(button.IsInState<state_pressed>());

    //We can do something with the state too
    auto& p_state = button.Get<state_pressed>();
    printf("time pressed = %d", p_state.something());


    button.Dispatch(release_event{});
    configASSERT(button.IsInState<state_idle>());


*/

// define as 1 if on_entry(state) functions required. (see example)
#ifndef CALL_ON_STATE_ENTRY
#define CALL_ON_STATE_ENTRY 1
#endif

// define as 1 if on_exit(state) functions required. (see example)
#ifndef CALL_ON_STATE_EXIT
#define CALL_ON_STATE_EXIT 0
#endif

template <typename Derived, typename StateVariant, typename EventVariant>
class FsmTask
{
    static constexpr uint8_t EVENT_QUEUE_DEFAULT_SIZE{3};

public:
    // Create the FSM Task
    FsmTask(uint32_t taskSize, uint8_t priority, const char *name, uint8_t eventQueueSize = EVENT_QUEUE_DEFAULT_SIZE, BaseType_t xCoreID = tskNO_AFFINITY);

    // Start the FSM Task
    void Start();
    void Start(StateVariant &&state);

    // Dispatch an event to state machine
    template <typename Event>
    bool Dispatch(Event &&event, TickType_t timeout = 0);

    template <typename Event>
    bool DispatchFromISR(Event &&event, BaseType_t *const xHigherPriorityTaskWoken);

    // Whether the fsm is currently in a certain state
    template <class State>
    bool IsInState() const { return std::holds_alternative<State>(m_states); }

protected:
    // Get the state if the state is the requested otherwise asserts
    template <class State>
    State &Get()
    {
        configASSERT(IsInState<State>());
        return std::get<State>(m_states);
    }

    // Get the states variant
    const StateVariant &GetStates() const { return m_states; }
    StateVariant &GetStates() { return m_states; }

private:
    static void s_mainTaskFunc(void *arg);

    void mainTaskFunc();
    void dispatch();
    void handleNewState(std::optional<StateVariant> &&newState);

    bool m_isRunning{false};
    StateVariant m_states{};
    EventVariant m_events{};
    TaskHandle_t m_task{};
    QueueHandle_t m_eventQueue{};
};

//----------------------- PUBLIC FUNTIONS IMPLEMENTATION ------------------------

// CONSTRUCTOR
template <typename Derived, typename StateVariant, typename EventVariant>
FsmTask<Derived, StateVariant, EventVariant>::FsmTask(uint32_t taskSize, uint8_t priority, const char *name, uint8_t eventQueueSize, BaseType_t xCoreID)
{
    m_eventQueue = xQueueCreate(eventQueueSize, sizeof(EventVariant));
    configASSERT(pdPASS == xTaskCreatePinnedToCore(s_mainTaskFunc, name, taskSize, this, priority, &m_task, xCoreID));
    configASSERT(m_eventQueue != nullptr);
    configASSERT(m_task != nullptr);
}

template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTask<Derived, StateVariant, EventVariant>::Start()
{
    configASSERT(!m_isRunning);

    m_isRunning = true;
    xTaskNotifyGive(m_task);
}

template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTask<Derived, StateVariant, EventVariant>::Start(StateVariant &&state)
{
    configASSERT(!m_isRunning);

    m_isRunning = true;
    m_states = std::move(state);
    xTaskNotifyGive(m_task);
}

// DISPATCH AN EVENT
template <typename Derived, typename StateVariant, typename EventVariant>
template <typename Event>
bool FsmTask<Derived, StateVariant, EventVariant>::Dispatch(Event &&event, TickType_t timeout)
{
    if (!m_isRunning)
        return false;

    EventVariant var{event};
    return xQueueSend(m_eventQueue, &var, timeout) == pdTRUE;
}

// DISPATCH AN EVENT FROM ISR
template <typename Derived, typename StateVariant, typename EventVariant>
template <typename Event>
bool FsmTask<Derived, StateVariant, EventVariant>::DispatchFromISR(Event &&event, BaseType_t *const xHigherPriorityTaskWoken)
{
    EventVariant var{event};
    return xQueueSendFromISR(m_eventQueue, &var, xHigherPriorityTaskWoken) == pdTRUE;
}

//--------------------- TASK MANAGEMENT FUNCTIONS IMPLEMENTATION ----------------

// MAIN TASK ENTRY FUNCTION
template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTask<Derived, StateVariant, EventVariant>::s_mainTaskFunc(void *arg)
{
    FsmTask *This = reinterpret_cast<FsmTask *>(arg);
    This->mainTaskFunc();
}

// MAIN TASK LOOP
template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTask<Derived, StateVariant, EventVariant>::mainTaskFunc()
{
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    if constexpr (CALL_ON_STATE_ENTRY)
    {
        Derived &child = static_cast<Derived &>(*this);
        std::visit([&](auto &stateVar)
                   { child.on_entry(stateVar); },
                   m_states);
    }

    for (;;)
    {
        xQueueReceive(m_eventQueue, &m_events, portMAX_DELAY);
        dispatch();
    }
}

//------------------------ PRIVATE FUNTIONS IMPLEMENTATION ----------------------

// PRIVATE DISPATCH HANDLING
template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTask<Derived, StateVariant, EventVariant>::dispatch()
{
    Derived &child = static_cast<Derived &>(*this);
    auto newState = std::visit(
        [&](auto &stateVar, auto &eventVar) -> std::optional<StateVariant>
        { return child.on_event(stateVar, eventVar); },
        m_states, m_events);

    handleNewState(std::move(newState));
}

// HANDLE NEW STATE TRANSITION
template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTask<Derived, StateVariant, EventVariant>::handleNewState(std::optional<StateVariant> &&newState)
{
    Derived &child = static_cast<Derived &>(*this);
    if (!newState)
        return;

    if constexpr (CALL_ON_STATE_EXIT)
    {
        std::visit([&](auto &stateVar)
                   { child.on_exit(stateVar); },
                   m_states);
    }

    m_states = *std::move(newState);

    if constexpr (CALL_ON_STATE_ENTRY)
    {
        std::visit([&](auto &stateVar)
                   { child.on_entry(stateVar); },
                   m_states);
    }
}

#endif // __FSM_TASK_H__
