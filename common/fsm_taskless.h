#ifndef __FSM_TASKLESS_H__
#define __FSM_TASKLESS_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <variant>
#include <optional>

/*
    Equivalent to FSMTask but without a task (run in the same conetxt as the caller)
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
class FsmTaskless
{
public:
    // Start the FSM Task
    void Start();
    void Start(StateVariant &&state);

    // Dispatch an event to state machine
    template <typename Event>
    void Dispatch(Event &&event);

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
    void handleNewState(std::optional<StateVariant> &&newState);

    bool m_isRunning{false};
    StateVariant m_states{};
    EventVariant m_events{};
};

//----------------------- PUBLIC FUNTIONS IMPLEMENTATION ------------------------

template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTaskless<Derived, StateVariant, EventVariant>::Start()
{
    configASSERT(!m_isRunning);

    m_isRunning = true;

    if constexpr (CALL_ON_STATE_ENTRY)
    {
        Derived &child = static_cast<Derived &>(*this);
        std::visit([&](auto &stateVar)
                   { child.on_entry(stateVar); },
                   m_states);
    }
}

template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTaskless<Derived, StateVariant, EventVariant>::Start(StateVariant &&state)
{
    configASSERT(!m_isRunning);

    m_states = std::move(state);
    m_isRunning = true;

    if constexpr (CALL_ON_STATE_ENTRY)
    {
        Derived &child = static_cast<Derived &>(*this);
        std::visit([&](auto &stateVar)
                   { child.on_entry(stateVar); },
                   m_states);
    }
}

// DISPATCH AN EVENT
template <typename Derived, typename StateVariant, typename EventVariant>
template <typename Event>
void FsmTaskless<Derived, StateVariant, EventVariant>::Dispatch(Event &&event)
{
    configASSERT(m_isRunning);

    m_events = std::move(event);
    Derived &child = static_cast<Derived &>(*this);
    auto newState = std::visit(
        [&](auto &stateVar, auto &eventVar) -> std::optional<StateVariant>
        { return child.on_event(stateVar, eventVar); },
        m_states, m_events);

    handleNewState(std::move(newState));
}

//------------------------ PRIVATE FUNTIONS IMPLEMENTATION ----------------------

// HANDLE NEW STATE TRANSITION
template <typename Derived, typename StateVariant, typename EventVariant>
void FsmTaskless<Derived, StateVariant, EventVariant>::handleNewState(std::optional<StateVariant> &&newState)
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
