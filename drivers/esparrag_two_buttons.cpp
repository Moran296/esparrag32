#include "esparrag_two_buttons.h"
#include "debouncer.h"

// ========IDLE STATE============
etl::fsm_state_id_t
IdleState_2B::on_enter_state()
{
    m_isButton1Pressed = false;
    m_isButton2Pressed = false;
    return STATE_ID;
}

etl::fsm_state_id_t
IdleState_2B::on_event(const PressEvent_2B &event)
{
    auto &two_buttons = get_fsm_context();
    auto &buttonPressed = event.m_buttonId == eButtonID::BUTTON_A ? m_isButton1Pressed : m_isButton2Pressed;
    buttonPressed = true;
    if (m_isButton1Pressed && m_isButton2Pressed)
    {
        //one button previously pressed, should release it (without running its callback)
        auto &previousPressedButton = event.m_buttonId == eButtonID::BUTTON_A ? two_buttons.m_b2 : two_buttons.m_b1;
        previousPressedButton.m_ignoreReleaseCallback = true;
        previousPressedButton.receive(ReleaseEvent());
        return eStateId::PRESSED;
    }

    //only one button was pressed, send him a press event
    auto &button = m_isButton1Pressed ? two_buttons.m_b1 : two_buttons.m_b2;
    button.receive(PressEvent());
    return STATE_ID;
}

etl::fsm_state_id_t
IdleState_2B::on_event(const ReleaseEvent_2B &event)
{
    auto &buttonReleased = event.m_buttonId == eButtonID::BUTTON_A ? m_isButton1Pressed : m_isButton2Pressed;
    buttonReleased = false;

    auto &two_buttons = get_fsm_context();
    auto &button = event.m_buttonId == eButtonID::BUTTON_A ? two_buttons.m_b1 : two_buttons.m_b2;
    button.receive(ReleaseEvent());
    return STATE_ID;
}

etl::fsm_state_id_t
IdleState_2B::on_event_unknown(const etl::imessage &event)
{
    ets_printf("idle unknown 2b %d\n", (uint8_t)get_state_id());
    return STATE_ID;
}

// ========PRESSED STATE============
etl::fsm_state_id_t
PressedState_2B::on_enter_state()
{
    m_isButton1Released = false;
    m_isButton2Released = false;

    auto &two_buttons = get_fsm_context();
    two_buttons.m_b1.runPressCallback(two_buttons.m_pressCallbacks, ePressType::FAST_PRESS);

    m_timeouts = ePressType::PRESS_TIMEOUT_1;
    two_buttons.m_lastPressTime = esp_timer_get_time();
    two_buttons.startTimer(ePressType(m_timeouts));
    return STATE_ID;
}

etl::fsm_state_id_t
PressedState_2B::on_event(const ReleaseEvent_2B &event)
{
    auto &two_buttons = get_fsm_context();
    two_buttons.stopTimer(true);

    auto &buttonReleased = event.m_buttonId == eButtonID::BUTTON_A ? m_isButton1Released : m_isButton2Released;
    buttonReleased = true;
    if (m_isButton1Released && m_isButton2Released)
    {
        two_buttons.m_b1.runReleaseCallback(two_buttons.m_releaseCallbacks);
        return eStateId::IDLE;
    }

    return STATE_ID;
}

etl::fsm_state_id_t
PressedState_2B::on_event(const TimerEvent_2B &event)
{
    auto &two_button = get_fsm_context();
    two_button.m_b1.runPressCallback(two_button.m_pressCallbacks, ePressType(m_timeouts));

    m_timeouts++;
    two_button.startTimer(ePressType(m_timeouts));
    return STATE_ID;
}

etl::fsm_state_id_t
PressedState_2B::on_event_unknown(const etl::imessage &event)
{
    m_timeouts = 0;
    ets_printf("pressed unknown 2b %d\n", (uint8_t)get_state_id());
    return STATE_ID;
}

//========TwoButtons=====

TwoButtons::TwoButtons(Button &a, Button &b) : fsm(get_instance_count()), m_b1(a), m_b2(b)
{
    eResult res = eResult::SUCCESS;
    res = m_b1.m_gpi.DisableInterrupt();
    ESPARRAG_ASSERT(res == eResult::SUCCESS);
    res = m_b2.m_gpi.DisableInterrupt();
    ESPARRAG_ASSERT(res == eResult::SUCCESS);
    res = m_b1.m_gpi.EnableInterrupt(button1ISR, this);
    ESPARRAG_ASSERT(res == eResult::SUCCESS);
    res = m_b2.m_gpi.EnableInterrupt(button2ISR, this);
    ESPARRAG_ASSERT(res == eResult::SUCCESS);

    m_timer = xTimerCreate("two_buttons", 100, pdFALSE, this, timerCB);
    ESPARRAG_ASSERT(m_timer);
    set_states(stateList, etl::size(stateList));
    start();
}

eResult TwoButtons::RegisterPress(const Button::buttonCB &cb)
{
    return m_b1.registerEvent(cb, m_pressCallbacks);
}
eResult TwoButtons::RegisterRelease(const Button::buttonCB &cb)
{
    return m_b1.registerEvent(cb, m_releaseCallbacks);
}

void TwoButtons::button1ISR(void *arg)
{
    TwoButtons *two_buttons = reinterpret_cast<TwoButtons *>(arg);
    if (!two_buttons->m_sampler.IsValidNow())
        return;

    Button &button = two_buttons->m_b1;
    button.m_buttonState = !button.m_buttonState;
    if (button.m_buttonState)
    {
        button.m_gpi.SetInterruptType(GPIO_INTR_LOW_LEVEL);
        two_buttons->receive(PressEvent_2B(eButtonID::BUTTON_A));
    }
    else
    {
        button.m_gpi.SetInterruptType(GPIO_INTR_HIGH_LEVEL);
        two_buttons->receive(ReleaseEvent_2B(eButtonID::BUTTON_A));
    }
}

void TwoButtons::button2ISR(void *arg)
{
    TwoButtons *two_buttons = reinterpret_cast<TwoButtons *>(arg);
    if (!two_buttons->m_sampler.IsValidNow())
        return;

    Button &button = two_buttons->m_b2;
    button.m_buttonState = !button.m_buttonState;
    if (button.m_buttonState)
    {
        button.m_gpi.SetInterruptType(GPIO_INTR_LOW_LEVEL);
        two_buttons->receive(PressEvent_2B(eButtonID::BUTTON_B));
    }
    else
    {
        button.m_gpi.SetInterruptType(GPIO_INTR_HIGH_LEVEL);
        two_buttons->receive(ReleaseEvent_2B(eButtonID::BUTTON_B));
    }
}

void TwoButtons::timerCB(xTimerHandle timer)
{
    TwoButtons *two_buttons = reinterpret_cast<TwoButtons *>(pvTimerGetTimerID(timer));
    two_buttons->receive(TimerEvent_2B());
}

void TwoButtons::stopTimer(bool fromISR)
{
    if (fromISR)
        xTimerStopFromISR(m_timer, NULL);
    else
        xTimerStop(m_timer, DEFAULT_FREERTOS_TIMEOUT);
}

void TwoButtons::startTimer(ePressType timeout)
{
    ESPARRAG_ASSERT(timeout.get_value() >= ePressType::PRESS_TIMEOUT_1);
    if (timeout.get_value() >= Button::MAX_CALLBACKS)
        return;

    auto &callback = m_pressCallbacks[timeout.get_value()];
    bool callbackValid = callback.cb_function != nullptr && callback.cb_time.value() != 0;
    if (!callbackValid)
        return;

    //callback timeout is the delta between requested time and last callback timeout
    MilliSeconds ms = callback.cb_time - m_pressCallbacks[timeout.get_value() - 1].cb_time;
    BaseType_t higherPriorityExists = pdFALSE;
    xTimerChangePeriodFromISR(m_timer, ms.toTicks(), &higherPriorityExists);
    xTimerStartFromISR(m_timer, &higherPriorityExists);
    YIELD_FROM_ISR_IF(higherPriorityExists);
}