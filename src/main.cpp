#include "esparrag_common.h"
#include "etl/enum_type.h"
#include "esparrag_log.h"
#include "database.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main()
{
    ConfigDB db;
    db.Init();

    const char *ap_ssid = nullptr;
    db.Get(CONFIG_ID::AP_SSID, ap_ssid);
    if (ap_ssid == nullptr)
        ESPARRAG_LOG_INFO("huston we have a problem");
    else
        ESPARRAG_LOG_INFO("ap ssid %s", ap_ssid);
    db.Get(CONFIG_ID::AP_PASSWORD, ap_ssid);
    if (ap_ssid == nullptr)
        ESPARRAG_LOG_INFO("huston we have a problem");
    else
        ESPARRAG_LOG_INFO("ap pass %s", ap_ssid);

    const char g[] = "whhop whhop";
    eResult res = db.Set(CONFIG_ID::AP_SSID, g);
    if (res != eResult::SUCCESS)
        ESPARRAG_LOG_INFO("couldnt write");
    res = db.Set(CONFIG_ID::AP_PASSWORD, g);
    if (res != eResult::SUCCESS)
        ESPARRAG_LOG_INFO("couldnt write");
    db.Commit();
    db.Get(CONFIG_ID::AP_SSID, ap_ssid);
    if (ap_ssid == nullptr)
        ESPARRAG_LOG_INFO("huston we have a problem");
    else
        ESPARRAG_LOG_INFO("ap ssid %s", ap_ssid);

    vTaskDelay(100000000);
}