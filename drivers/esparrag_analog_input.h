#ifndef ESPARRAG_ANALOG_INPUT_H__
#define ESPARRAG_ANALOG_INPUT_H__

#include "esparrag_common.h"
#include "esp_adc_cal.h"
#include <driver/adc.h>
#include "etl/rescale.h"

class AnalogInput
{
public:
    AnalogInput(adc_channel_t channel, adc_atten_t attenuation, uint16_t sampleNum);
    uint16_t Read() const;
    uint16_t ReadLinearized(uint16_t min_in, uint16_t max_in, uint16_t min_out, uint16_t max_out) const;

private:
    const adc_channel_t m_channel;
    const adc_atten_t m_attenuation;
    const uint16_t m_sampleNum;

    static constexpr adc_bits_width_t BIT_WIDTH = ADC_WIDTH_BIT_12;
    static bool is_set_bit_width;
    static void setBitWidth();

    AnalogInput(const AnalogInput &) = delete;
    AnalogInput &operator=(const AnalogInput &) = delete;
};

#endif