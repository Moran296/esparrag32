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
    GPI gpi1(17, GPIO_INTR_ANYEDGE);
    Button a(gpi1);
    Button::buttonCB press_fast = {.cb_function = goo, .cb_arg2 = FAST_PRESS};
    Button::buttonCB release_fast = {.cb_function = goo, .cb_arg2 = FAST_RELEASE};
    Button::buttonCB press_short = {.cb_function = goo, .cb_arg2 = SHORT_PRESS, .cb_time = Seconds(3)};
    Button::buttonCB release_short = {.cb_function = goo, .cb_arg2 = SHORT_RELEASE, .cb_time = Seconds(3)};
    Button::buttonCB press_long = {.cb_function = goo, .cb_arg2 = LONG_PRESS, .cb_time = Seconds(5)};
    Button::buttonCB release_long = {.cb_function = goo, .cb_arg2 = LONG_RELEASE, .cb_time = Seconds(5)};

    a.RegisterPress(std::move(press_fast));
    a.RegisterRelease(std::move(release_fast));
    a.RegisterPress(std::move(press_short));
    a.RegisterRelease(std::move(release_short));
    a.RegisterPress(std::move(press_long));
    a.RegisterRelease(std::move(release_long));

    GPI gpi2(18, GPIO_INTR_ANYEDGE);
    Button b(gpi2);
    Button::buttonCB press_fast2 = {.cb_function = goo, .cb_arg2 = 34};
    Button::buttonCB release_fast2 = {.cb_function = goo, .cb_arg2 = 35};
    b.RegisterPress(std::move(press_fast2));
    b.RegisterRelease(std::move(release_fast2));
    Button::buttonCB b2press = {.cb_function = goo, .cb_arg2 = 55, .cb_time = Seconds(3)};
    Button::buttonCB b2release = {.cb_function = goo, .cb_arg2 = 66, .cb_time = Seconds(3)};
    TwoButtons butt(a, b);
    butt.RegisterPress(std::move(b2press));
    butt.RegisterRelease(std::move(b2release));

    EsparragManager manager;
    manager.Run();

    vTaskSuspend(nullptr);
}