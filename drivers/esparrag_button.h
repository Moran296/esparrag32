#ifndef ESPARRAG_BUTTON_H__
#define ESPARRAG_BUTTON_H__

#include "esparrag_common.h"
#include "esparrag_time_units.h"
#include "esparrag_gpio.h"
#include "etl/delegate"
#include "etl/vector"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

class Button
{
public:
    static constexpr MAX_CALLBACKS = 3;
    using button_function_t = etl::delegate<void(void)>;
    friend class TwoButtons;

    Button(GPI &gpi) : m_gpi(gpi), m_state((m_gpi)) {}
    ~Button() { ESPARRAG_ASSERT(false); }

    void OnPress(button_function_t func);
    void OnPress(MS time, button_function_t func);
    void OnRelease(button_function_t func);
    void OnRelease(MS time, button_function_t func);

private:
    using ImmediatePress = MS(0);

    GPI &m_gpi;
    etl::vector<std::pair<MS, button_function_t>, MAX_CALLBACKS> m_pressCB;
    etl::vector<std::pair<MS, button_function_t>, MAX_CALLBACKS> m_releaseCB;
    bool m_state;
    uS m_pressedTime(0);
    xTimerHandle m_timer = nullptr;

    Button(Button &b) = delete;
    Button &operator==(Button &b) = delete;
};

class TwoButtons
{
public:
    TwoButtons(Button &b1, Button &b2) : m_b1(b1), m_b2(b2) {}
    void OnPress(button_function_t func);
    void OnPress(MS time, button_function_t func);

private:
    etl::vector<std::pair<int, button_function_t>, MAX_CALLBACKS> m_callbacks;
    Button &m_b1;
    Button &m_b2;
};

#endif