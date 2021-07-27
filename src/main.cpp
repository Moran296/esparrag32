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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>
#include "esparrag_time_units.h"
#include "esparrag_settings.h"
#include "executioner.h"

extern "C" void app_main()
{

    EsparragManager manager;
    manager.Run();

    vTaskSuspend(nullptr);
}