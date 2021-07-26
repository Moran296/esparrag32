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

extern "C" void app_main()
{

    EsparragManager manager;
    manager.Run();

    // server.On("/credentials", METHOD::POST,
    //           [](Request &req, Response &res)
    //           {
    //               if (!req.m_content)
    //               {
    //                   res.m_code = Response::CODE(404);
    //                   res.m_format = Response::FORMAT::JSON;
    //                   cJSON_AddStringToObject(res.m_json, "message", "invalid request");
    //                   return eResult::SUCCESS;
    //               }

    //               char *cont = cJSON_Print(req.m_content);
    //               ESPARRAG_LOG_INFO("%s", cont);
    //               cJSON_free(cont);

    //               cJSON *ssidJ = cJSON_GetObjectItem(req.m_content, "ssid");
    //               cJSON *passJ = cJSON_GetObjectItem(req.m_content, "password");
    //               const char *ssid = cJSON_GetStringValue(ssidJ);
    //               const char *pass = cJSON_GetStringValue(passJ);
    //               if (ssid == nullptr || pass == nullptr)
    //               {
    //                   res.m_code = Response::CODE(404);
    //                   res.m_format = Response::FORMAT::JSON;
    //                   cJSON_AddStringToObject(res.m_json, "message", "missing parameters");
    //                   return eResult::SUCCESS;
    //               }

    //               Settings::Config.Set<CONFIG_ID::STA_SSID, CONFIG_ID::STA_PASSWORD>(ssid, pass);
    //               res.m_code = Response::CODE(200);
    //               res.m_format = Response::FORMAT::JSON;
    //               cJSON_AddStringToObject(res.m_json, "message", "Got it, thanks");
    //               return eResult::SUCCESS;
    //           });

    vTaskSuspend(nullptr);
}