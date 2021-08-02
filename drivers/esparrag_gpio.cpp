#include "esparrag_gpio.h"
#include "esparrag_log.h"

GPIO_BASE::GPIO_BASE(int pin,
                     gpio_int_type_t isrType,
                     gpio_mode_t mode,
                     bool active_state,
                     bool pullup,
                     bool pulldown)
{
    m_pin = gpio_num_t(pin);
    m_config.pin_bit_mask = 1ULL << pin;
    m_config.mode = mode;
    m_config.pull_down_en = pulldown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    m_config.pull_up_en = pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    m_config.intr_type = isrType;

    esp_err_t err = gpio_config(&m_config);
    if (err != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("failed gpio config, err %d", err);
        ESPARRAG_ASSERT(false);
    }

    m_activeState = active_state;
}

/*==================
        GPO
===================*/
GPO::GPO(int pin,
         bool active_state,
         bool pullup,
         bool pulldown) : GPIO_BASE(pin,
                                    GPIO_INTR_DISABLE,
                                    GPIO_MODE_OUTPUT,
                                    active_state,
                                    pullup,
                                    pulldown)
{
    Set(false);
}

void GPO::Set(bool state)
{
    uint32_t active = state ? m_activeState : !m_activeState;
    esp_err_t res = gpio_set_level(m_pin, active);
    ESPARRAG_ASSERT(res == ESP_OK);
    m_state = state;
}

void GPO::operator++(int)
{
    Set(true);
}
void GPO::operator--(int)
{
    Set(false);
}

GPO::operator bool() const
{
    return m_state;
}

void GPO::Toggle()
{
    Set(!m_state);
}

/*==================
        GPI
===================*/

bool GPI::m_isr_driver_installed = false;

GPI::GPI(int pin,
         gpio_int_type_t isrType,
         bool active_state,
         bool pullup,
         bool pulldown) : GPIO_BASE(pin,
                                    isrType,
                                    GPIO_MODE_INPUT,
                                    active_state,
                                    pullup,
                                    pulldown)
{
}

GPI::operator bool() const
{
    return IsActive();
}
bool GPI::IsActive() const
{
    return 0 != gpio_get_level(m_pin);
}

#include "esp_intr_alloc.h"

eResult GPI::EnableInterrupt(isr_func_t isrEventHandler, void *arg)
{
    ESPARRAG_ASSERT(m_config.intr_type != GPIO_INTR_DISABLE);
    ESPARRAG_ASSERT(isrEventHandler != nullptr);

    esp_err_t res = ESP_OK;

    if (!m_isr_driver_installed)
    {
        res = gpio_install_isr_service(0);
        if (res != ESP_OK)
        {
            ESPARRAG_LOG_ERROR("set interrupt failed, err %d", res);
            return eResult::ERROR_INVALID_STATE;
        }

        m_isr_driver_installed = true;
    }

    res = gpio_isr_handler_add(m_pin, isrEventHandler, arg);
    if (res != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("set interrupt failed, err %d", res);
        return eResult::ERROR_INVALID_STATE;
    }

    return eResult::SUCCESS;
}

eResult GPI::DisableInterrupt()
{
    esp_err_t res = gpio_isr_handler_remove(m_pin);
    if (res != ESP_OK)
    {
        ESPARRAG_LOG_ERROR("disable interrupt failed, err %d", res);
        return eResult::ERROR_INVALID_STATE;
    }

    return eResult::SUCCESS;
}

eResult GPI::SetInterruptType(gpio_int_type_t type)
{
    esp_err_t res = gpio_set_intr_type(m_pin, type);
    return res == ESP_OK ? eResult::SUCCESS : eResult::ERROR_INVALID_STATE;
}
