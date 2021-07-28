#ifndef GPIO_I_H__
#define GPIO_I_H__

#include "driver/gpio.h"
#include "esparrag_common.h"

class GPIO_BASE
{
protected:
    GPIO_BASE(int pin,
              gpio_int_type_t isrType,
              gpio_mode_t mode,
              bool active_state,
              bool pullup,
              bool pulldown);

    gpio_config_t m_config;
    gpio_num_t m_pin;
    bool m_activeState;

    GPIO_BASE(const GPIO_BASE &) = delete;
    GPIO_BASE &operator=(const GPIO_BASE &) = delete;
};

class GPO : GPIO_BASE
{
public:
    GPO(int pin,
        bool active_state = true,
        bool pullup = false,
        bool pulldown = false);

    void Set(bool state);
    void Toggle();
    void operator++(int);
    void operator--(int);
    explicit operator bool() const;

private:
    bool m_state = false;
};

class GPI : GPIO_BASE
{
public:
    using isr_func_t = void (*)(void *arg);

    GPI(int pin,
        gpio_int_type_t isrType,
        bool active_state = true,
        bool pullup = false,
        bool pulldown = false);

    explicit operator bool() const;
    bool IsActive() const;
    eResult DisableInterrupt();
    eResult EnableInterrupt(isr_func_t isrEventHandler, void *arg);

private:
    static bool m_isr_driver_installed;
};

#endif