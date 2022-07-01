#ifndef ESPARRAG_PWM_H__
#define ESPARRAG_PWM_H__

#include "driver/gpio.h"
#include "driver/ledc.h"


class Pwm
{
    public:
    static void Init() {

    }


    Pwm(gpio_num_t pin, ledc_mode_t mode, ledc_timer_bit_t dutyCycleResolution, ledc_timer_t timerNumber,
        uint32_t frequency, ledc_channel_t ledChannel);

    void SetDutyCycle(uint32_t dutyCycle);
    uint32_t GetDutyCycle();
    void SetFrequency(uint32_t frequency);
    uint32_t GetFrequency();
    void SetResolution(uint8_t resolution);
    uint8_t GetResolution();

    private:

    static bool s_isInit;

    Pwm(const Pwm &) = delete;
    Pwm &operator=(const Pwm &) = delete;

    void configPwm();
    uint32_t getMaxFrequency();
    uint32_t getCorrectedFrequency(uint32_t frequency);

    ledc_timer_config_t m_pwmTimerConfig;
    ledc_channel_config_t m_pwmChannelConfig;
    const gpio_num_t m_pin;
};



#endif