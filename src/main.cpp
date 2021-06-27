#include "esparrag_common.h"
#include "etl/enum_type.h"
#include "nvs_flash.h"
#include "esparrag_log.h"
#include "esparrag_database.h"
#include "esparrag_http.h"
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
    HttpServer server(db);
    server.Init();
    server.On("/hi", METHOD::GET,
              [](Request &req, Response &res)
              {
                  if (req.m_content)
                  {
                      char *cont = cJSON_Print(req.m_content);
                      ESPARRAG_LOG_INFO("%s", cont);
                      cJSON_free(cont);
                  }

                  res.m_code = CODE(201);
                  cJSON_AddStringToObject(res.m_response, "message", "Got it, thanks");
                  return eResult::SUCCESS;
              });

    vTaskDelay(15_sec);

    db.Set(CONFIG_ID::STA_SSID, "Rozen_2");
    db.Set(CONFIG_ID::STA_PASSWORD, "0545525855");
    db.Commit();

    vTaskDelay(100000000);
}