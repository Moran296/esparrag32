#include "esparrag_common.h"
#include "etl/enum_type.h"
#include "nvs_flash.h"
#include "esparrag_log.h"
#include "esparrag_database.h"
#include "esparrag_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main()
{

    nvs_flash_erase();
    ConfigDB db;
    db.Init();
    Wifi wifi(db);
    wifi.Init();

    vTaskDelay(15_sec);

    db.Set(CONFIG_ID::STA_SSID, "Rozen_2");
    db.Set(CONFIG_ID::STA_PASSWORD, "0545525855");
    db.Commit();

    vTaskDelay(100000000);
}