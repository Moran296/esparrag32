#ifndef ESPARRAG_DEBOUNCER_H__
#define ESPARRAG_DEBOUNCER_H__

#include "esparrag_time_units.h"
#include "esp_timer.h"

class Debouncer
{
public:
    Debouncer(const MicroSeconds debounce_time = 1000) : debounce_time(debounce_time),
                                                              last_sample(esp_timer_get_time()) {}
    bool IsValidNow()
    {
        if (esp_timer_get_time() - last_sample.value() > debounce_time.value())
        {
            last_sample = esp_timer_get_time();
            return true;
        }

        return false;
    }

private:
    const MicroSeconds debounce_time;
    MicroSeconds last_sample;
};

#endif