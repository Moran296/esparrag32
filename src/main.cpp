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
    default:
        ESPARRAG_LOG_INFO("whaaaaattt");
    }
}

extern "C" void app_main()
{
    GPI gpi(17, GPIO_INTR_ANYEDGE);
    BUTTON b(gpi);
    BUTTON::buttonCB press_fast = {.m_cb = goo, .arg2 = FAST_PRESS};
    BUTTON::buttonCB release_fast = {.m_cb = goo, .arg2 = FAST_RELEASE};
    BUTTON::buttonCB press_short = {.m_cb = goo, .arg2 = SHORT_PRESS, .m_ms = Seconds(3)};
    BUTTON::buttonCB release_short = {.m_cb = goo, .arg2 = SHORT_RELEASE, .m_ms = Seconds(3)};
    BUTTON::buttonCB press_long = {.m_cb = goo, .arg2 = LONG_PRESS, .m_ms = Seconds(5)};
    BUTTON::buttonCB release_long = {.m_cb = goo, .arg2 = LONG_RELEASE, .m_ms = Seconds(5)};
     b.RegisterPress(std::move(press_fast));
    // b.RegisterRelease(std::move(release_fast));
    b.RegisterPress(std::move(press_short));
    // b.RegisterRelease(std::move(release_short));
    // b.RegisterPress(std::move(press_long));
    // b.RegisterRelease(std::move(release_long));

    EsparragManager manager;
    manager.Run();

    vTaskSuspend(nullptr);
}