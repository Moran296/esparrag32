#include "esparrag_common.h"
#include "etl/enum_type.h"
#include "etl/string.h"
#include "nvs_flash.h"
#include "esparrag_log.h"
#include "esparrag_database.h"
#include "esparrag_gpio.h"
#include "esparrag_http.h"
#include "esparrag_wifi.h"
#include "esparrag_manager.h"
#include "esparrag_button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>
#include "esparrag_time_units.h"
#include "esparrag_settings.h"
#include "esparrag_two_buttons.h"

#define FAST_PRESS 1
#define SHORT_PRESS 2
#define LONG_PRESS 3
#define FAST_RELEASE 4
#define SHORT_RELEASE 5
#define LONG_RELEASE 6

void goo(void *arg, uint32_t hello)
{
    switch (hello)
    {
    case FAST_PRESS:
        ESPARRAG_LOG_INFO("fast press");
        break;
    case SHORT_PRESS:
        ESPARRAG_LOG_INFO("short press");
        break;
    case 34:
        ESPARRAG_LOG_INFO("fast press b2");
        break;
    case 35:
        ESPARRAG_LOG_INFO("short press b3");
        break;
    case LONG_PRESS:
        ESPARRAG_LOG_INFO("long press");
        break;
    case FAST_RELEASE:
        ESPARRAG_LOG_INFO("fast release");
        break;
    case SHORT_RELEASE:
        ESPARRAG_LOG_INFO("short release");
        break;
    case LONG_RELEASE:
        ESPARRAG_LOG_INFO("long release");
        break;
    case 12:
        ESPARRAG_LOG_INFO("fast press two buttons");
        break;
    case 13:
        ESPARRAG_LOG_INFO("fast release two buttons");
        break;
    case 55:
        ESPARRAG_LOG_INFO("2b short timed callback press");
        break;
    case 66:
        ESPARRAG_LOG_INFO("2b short timed callback release");
        break;
    default:
        ESPARRAG_LOG_INFO("whaaaaattt");
    }
}

#include "etl/vector.h"
#include "etl/algorithm.h"

extern "C" void app_main()
{

    EsparragManager manager;
    manager.Run();

    vTaskSuspend(nullptr);
}