#include "esparrag_pwm.h"
#include "freertos/FreeRTOS.h"
#include "esparrag_log.h"


bool Pwm::s_isInit = false;

Pwm::Pwm(gpio_num_t pin, ledc_mode_t mode, ledc_timer_bit_t dutyCycleResolution, ledc_timer_t timerNumber,
         uint32_t frequency, ledc_channel_t ledChannel) : m_pin(pin) {

    m_pwmTimerConfig.speed_mode = mode;
    m_pwmTimerConfig.duty_resolution = dutyCycleResolution;
    m_pwmTimerConfig.timer_num = timerNumber;
    m_pwmTimerConfig.freq_hz = frequency;
    m_pwmTimerConfig.clk_cfg = LEDC_AUTO_CLK;

    m_pwmChannelConfig.gpio_num = pin;
    m_pwmChannelConfig.speed_mode = mode;
    m_pwmChannelConfig.channel = ledChannel;
    m_pwmChannelConfig.intr_type = LEDC_INTR_DISABLE;
    m_pwmChannelConfig.timer_sel = timerNumber;
    m_pwmChannelConfig.duty = 0;
    m_pwmChannelConfig.hpoint = 0;

    configPwm();
}

void Pwm::SetDutyCycle(uint32_t dutyCycle) {
    m_pwmChannelConfig.duty = dutyCycle;
    esp_err_t res = ledc_set_duty(m_pwmTimerConfig.speed_mode, m_pwmChannelConfig.channel, dutyCycle);


    if (res != ESP_OK) {
        ESPARRAG_LOG_ERROR("ledc_set_duty failed: %d", res);
    }

    assert(res == ESP_OK);

    res = ledc_update_duty(m_pwmTimerConfig.speed_mode, m_pwmChannelConfig.channel);
    assert(res == ESP_OK);
}

uint32_t Pwm::GetDutyCycle() {
    int ledcRes = ledc_get_duty(m_pwmChannelConfig.speed_mode, m_pwmChannelConfig.channel);
    configASSERT(ledcRes != LEDC_ERR_DUTY);

    return (uint32_t)ledcRes;
}

void Pwm::SetFrequency(uint32_t frequency) {
    m_pwmTimerConfig.freq_hz = frequency;
    configPwm();
}

uint32_t Pwm::GetFrequency() {
    uint32_t ledcRes = ledc_get_freq(m_pwmTimerConfig.speed_mode, m_pwmTimerConfig.timer_num);
    configASSERT(ledcRes != 0);

    return ledcRes;
}

void Pwm::SetResolution(uint8_t resolution) {
    m_pwmTimerConfig.duty_resolution = (ledc_timer_bit_t)resolution;
    configPwm();
}

uint8_t Pwm::GetResolution() {
    return (uint8_t)m_pwmTimerConfig.duty_resolution;
}

uint32_t Pwm::getMaxFrequency() {
    uint32_t resolution = m_pwmTimerConfig.duty_resolution;
    uint64_t apbFrequency = APB_CLK_FREQ;

    return apbFrequency / (1 << resolution);
}

uint32_t Pwm::getCorrectedFrequency(uint32_t frequency) {
    uint32_t maxFrequency = getMaxFrequency();
    if (frequency > maxFrequency)
        return maxFrequency;

    return frequency;
}

void Pwm::configPwm() {
    uint32_t correctedFrequency = getCorrectedFrequency(m_pwmTimerConfig.freq_hz);
    m_pwmTimerConfig.freq_hz = correctedFrequency;
    ESPARRAG_LOG_WARNING("corrected freq %d", correctedFrequency);

    esp_err_t res = ledc_timer_config(&m_pwmTimerConfig);
    if (res != ESP_OK) {
        ESPARRAG_LOG_ERROR("Error config pwm timer %d", res);
        return;
    }

    res = ledc_channel_config(&m_pwmChannelConfig);
    if (res != ESP_OK) {
        ESPARRAG_LOG_ERROR("Error config pwm channel %d", res);
    }
}

