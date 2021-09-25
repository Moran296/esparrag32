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
#include "executioner.h"

#define FAST_PRESS 1
#define SHORT_PRESS 2
#define FAST_PRESS_2 34
#define SHORT_PRESS_2 35
#define LONG_PRESS 3
#define FAST_RELEASE 4
#define SHORT_RELEASE 5
#define LONG_RELEASE 6
#define LONGER_PRESS 59
#define LONGER_RELEASE 69

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
    case LONGER_PRESS:
        ESPARRAG_LOG_INFO("longer press");
        break;
    case LONGER_RELEASE:
        ESPARRAG_LOG_INFO("longer release");
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

extern "C" void app_main()
{

    // EsparragManager manager;
    // manager.Run();

    GPI gpi1{27, GPIO_INTR_DISABLE, true, false, true};
    Button b1{gpi1};

    b1.RegisterPress({.cb_function = goo, .cb_arg1 = nullptr, .cb_arg2 = SHORT_PRESS, .cb_time = 0});
    b1.RegisterRelease({.cb_function = goo, .cb_arg1 = nullptr, .cb_arg2 = SHORT_RELEASE, .cb_time = 0});
    b1.RegisterPress({.cb_function = goo, .cb_arg1 = nullptr, .cb_arg2 = LONG_PRESS, .cb_time = 2500});
    b1.RegisterRelease({.cb_function = goo, .cb_arg1 = nullptr, .cb_arg2 = LONG_RELEASE, .cb_time = 2500});
    b1.RegisterPress({.cb_function = goo, .cb_arg1 = nullptr, .cb_arg2 = LONGER_PRESS, .cb_time = 5000});
    b1.RegisterRelease({.cb_function = goo, .cb_arg1 = nullptr, .cb_arg2 = LONGER_RELEASE, .cb_time = 5000});

    vTaskSuspend(nullptr);
}