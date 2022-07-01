#ifndef ESPARRAGOS_COMMON__
#define ESPARRAGOS_COMMON__
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "etl/enum_type.h"
#include "etl/instance_count.h"
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
#define YIELD_FROM_ISR_IF(X) \
    if (X)                   \
    portYIELD_FROM_ISR()


struct eResult 
{
    enum enum_type
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
        ERROR_MEMORY,
        ERROR_CONNECTION_FAILURE,
        ERROR_GENERAL,

        NUM
    };

    ETL_DECLARE_ENUM_TYPE(eResult, uint8_t)
    ETL_ENUM_DEFAULT(SUCCESS)
    ETL_ENUM_DEFAULT(ERROR_NOT_INITIALIZED)
    ETL_ENUM_DEFAULT(ERROR_INVALID_PARAMETER)
    ETL_ENUM_DEFAULT(ERROR_INVALID_STATE)
    ETL_ENUM_DEFAULT(ERROR_FLASH)
    ETL_ENUM_DEFAULT(ERROR_FLASH_NOT_FOUND)
    ETL_ENUM_DEFAULT(ERROR_WIFI)
    ETL_ENUM_DEFAULT(ERROR_CONFIG_LIMITS)
    ETL_ENUM_DEFAULT(ERROR_NOT_FOUND)
    ETL_ENUM_DEFAULT(ERROR_MEMORY)
    ETL_ENUM_DEFAULT(ERROR_CONNECTION_FAILURE)
    ETL_ENUM_DEFAULT(ERROR_GENERAL)
    ETL_END_ENUM_TYPE
};

//Every etl fsm must inherit this
class FSM_COUNT : public etl::instance_count<FSM_COUNT>
{
};

#endif