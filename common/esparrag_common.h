#ifndef ESPARRAGOS_COMMON__
#define ESPARRAGOS_COMMON__
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "etl/enum_type.h"
#include "esp_log.h"

#define ESPARRAG_ASSERT(X)                                                                    \
    if (!(X))                                                                                 \
    {                                                                                         \
        ESP_LOGE(__FILE__, "%s:%d (%s)- assert failed!\n", __FILE__, __LINE__, __FUNCTION__); \
        while (1)                                                                             \
            ;                                                                                 \
    }

#define ETL_ENUM_DEFAULT(ID) ETL_ENUM_TYPE(ID, #ID)
#define DEFAULT_FREERTOS_TIMEOUT pdMS_TO_TICKS(5000) // 5 seconds

inline TickType_t operator"" _ms(unsigned long long milli)
{
    return pdMS_TO_TICKS(milli);
}
inline TickType_t operator"" _sec(unsigned long long sec)
{
    return sec * 1000_ms;
}
inline TickType_t operator"" _min(unsigned long long min)
{
    return min * 60_sec;
}

enum class eResult : uint8_t
{
    SUCCESS,
    ERROR_NOT_INITIALIZED,
    ERROR_INVALID_PARAMETER,
    ERROR_INVALID_STATE,
    ERROR_FLASH,
    ERROR_FLASH_NOT_FOUND,
    ERROR_WIFI,
    ERROR_CONFIG_LIMITS,
    ERROR_NOT_FOUND,
    ERROR_GENERAL,

    NUM

};

#endif