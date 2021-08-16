
#include "esparrag_analog_input.h"
#include "esparrag_time_units.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esparrag_log.h"
#include "etl/rescale.h"

bool AnalogInput::is_set_bit_width = false;
void AnalogInput::setBitWidth()
{
    esp_err_t err = adc1_config_width(BIT_WIDTH);
    ESPARRAG_ASSERT(err == ESP_OK);
    is_set_bit_width = true;
}

AnalogInput::AnalogInput(adc_channel_t channel, adc_atten_t attenuation, uint16_t sampleNum) : m_channel(channel),
                                                                                               m_attenuation(attenuation),
                                                                                               m_sampleNum(sampleNum)
{
    ESPARRAG_ASSERT(channel < ADC_CHANNEL_8);
    ESPARRAG_ASSERT(attenuation < ADC_ATTEN_MAX);
    esp_err_t err = ESP_OK;

    if (!is_set_bit_width)
        setBitWidth();

    err = adc1_config_channel_atten((adc1_channel_t)m_channel, m_attenuation);
    ESPARRAG_ASSERT(err == ESP_OK);
    err = adc_gpio_init(ADC_UNIT_1, m_channel);
    ESPARRAG_ASSERT(err == ESP_OK);
}

uint16_t AnalogInput::Read() const
{
    double sum = 0.0;

    for (auto i = 0; i < m_sampleNum; i++)
    {
        sum += adc1_get_raw((adc1_channel_t)m_channel);
        vTaskDelay(MilliSeconds(1).toTicks());
    }

    auto retVal = uint16_t(sum / m_sampleNum);
    return retVal;
}

uint16_t AnalogInput::ReadLinearized(uint16_t min_in, uint16_t max_in, uint16_t min_out, uint16_t max_out) const
{
    etl::rescale rescaler(min_in, max_in, min_out, max_out);
    return rescaler(Read());
}
