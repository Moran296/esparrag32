#include "esparrag_common.h"
#include "etl/enum_type.h"
#include "nvs_flash.h"
#include "esparrag_log.h"
#include "esparrag_database.h"
#include "esparrag_http.h"
#include "esparrag_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char indx[] = {
    0x3c, 0x21, 0x44, 0x4f, 0x43, 0x54, 0x59, 0x50, 0x45, 0x20, 0x68, 0x74,
    0x6d, 0x6c, 0x3e, 0x0d, 0x0a, 0x3c, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x0d,
    0x0a, 0x20, 0x20, 0x3c, 0x68, 0x65, 0x61, 0x64, 0x3e, 0x0d, 0x0a, 0x20,
    0x20, 0x20, 0x20, 0x3c, 0x74, 0x69, 0x74, 0x6c, 0x65, 0x3e, 0x54, 0x69,
    0x74, 0x6c, 0x65, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x64,
    0x6f, 0x63, 0x75, 0x6d, 0x65, 0x6e, 0x74, 0x3c, 0x2f, 0x74, 0x69, 0x74,
    0x6c, 0x65, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x2f, 0x68, 0x65, 0x61,
    0x64, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x62, 0x6f, 0x64, 0x79, 0x3e,
    0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x3c, 0x66, 0x6f, 0x72, 0x6d, 0x20,
    0x65, 0x6e, 0x63, 0x74, 0x79, 0x70, 0x65, 0x20, 0x3d, 0x27, 0x61, 0x70,
    0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x6a, 0x73,
    0x6f, 0x6e, 0x27, 0x20, 0x61, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x3d, 0x22,
    0x2f, 0x63, 0x72, 0x65, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x61, 0x6c, 0x73,
    0x22, 0x20, 0x6d, 0x65, 0x74, 0x68, 0x6f, 0x64, 0x3d, 0x22, 0x70, 0x6f,
    0x73, 0x74, 0x22, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x73, 0x73, 0x69, 0x64, 0x3a, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x3c, 0x69, 0x6e, 0x70, 0x75, 0x74, 0x20, 0x74, 0x79, 0x70, 0x65,
    0x3d, 0x22, 0x74, 0x65, 0x78, 0x74, 0x22, 0x20, 0x6e, 0x61, 0x6d, 0x65,
    0x3d, 0x22, 0x73, 0x73, 0x69, 0x64, 0x22, 0x20, 0x70, 0x6c, 0x61, 0x63,
    0x65, 0x68, 0x6f, 0x6c, 0x64, 0x65, 0x72, 0x3d, 0x22, 0x73, 0x73, 0x69,
    0x64, 0x22, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x50,
    0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64, 0x3a, 0x0d, 0x0a, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x3c, 0x69, 0x6e, 0x70, 0x75, 0x74, 0x20, 0x74,
    0x79, 0x70, 0x65, 0x3d, 0x22, 0x74, 0x65, 0x78, 0x74, 0x22, 0x20, 0x6e,
    0x61, 0x6d, 0x65, 0x3d, 0x22, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72,
    0x64, 0x22, 0x20, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x68, 0x6f, 0x6c, 0x64,
    0x65, 0x72, 0x3d, 0x22, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64,
    0x22, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3c, 0x62,
    0x72, 0x2f, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3c,
    0x62, 0x72, 0x2f, 0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x3c, 0x69, 0x6e, 0x70, 0x75, 0x74, 0x20, 0x74, 0x79, 0x70, 0x65, 0x3d,
    0x22, 0x73, 0x75, 0x62, 0x6d, 0x69, 0x74, 0x22, 0x20, 0x76, 0x61, 0x6c,
    0x75, 0x65, 0x3d, 0x22, 0x53, 0x75, 0x62, 0x6d, 0x69, 0x74, 0x22, 0x3e,
    0x0d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x3c, 0x2f, 0x66, 0x6f, 0x72, 0x6d,
    0x3e, 0x0d, 0x0a, 0x20, 0x20, 0x3c, 0x2f, 0x62, 0x6f, 0x64, 0x79, 0x3e,
    0x0d, 0x0a, 0x3c, 0x2f, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0};

extern "C" void app_main()
{

    nvs_flash_erase();
    ConfigDB db;
    db.Init();

    // WIFI_STATE state = WIFI_STATE::STA;
    // db.Set(std::pair(CONFIG_ID::STA_PASSWORD, "tahaha"),
    //        std::pair(CONFIG_ID::WIFI_STATE, state.get_value()));

    // auto newState = db.Get<WIFI_STATE>(CONFIG_ID::WIFI_STATE);
    // auto ss = db.Get<const char *>(CONFIG_ID::STA_PASSWORD);

    // ESPARRAG_LOG_INFO("state %d", newState.get_value());
    // ESPARRAG_LOG_INFO("ss %s", ss);

    Wifi wifi(db);
    wifi.Init();
    HttpServer server(db);
    server.Init();
    server.On("/index", METHOD::GET,
              [](Request &req, Response &res)
              {
                  res.m_code = Response::CODE(200);
                  res.m_format = Response::FORMAT::HTML;
                  res.m_string = indx;
                  return eResult::SUCCESS;
              });

    server.On("/credentials", METHOD::POST,
              [&db](Request &req, Response &res)
              {
                  if (!req.m_content)
                  {
                      res.m_code = Response::CODE(404);
                      res.m_format = Response::FORMAT::JSON;
                      cJSON_AddStringToObject(res.m_json, "message", "invalid request");
                      return eResult::SUCCESS;
                  }

                  char *cont = cJSON_Print(req.m_content);
                  ESPARRAG_LOG_INFO("%s", cont);
                  cJSON_free(cont);

                  cJSON *ssidJ = cJSON_GetObjectItem(req.m_content, "ssid");
                  cJSON *passJ = cJSON_GetObjectItem(req.m_content, "password");
                  const char *ssid = cJSON_GetStringValue(ssidJ);
                  const char *pass = cJSON_GetStringValue(passJ);
                  if (ssid == nullptr || pass == nullptr)
                  {
                      res.m_code = Response::CODE(404);
                      res.m_format = Response::FORMAT::JSON;
                      cJSON_AddStringToObject(res.m_json, "message", "missing parameters");
                      return eResult::SUCCESS;
                  }

                  db.Set(std::pair(CONFIG_ID::STA_PASSWORD, pass),
                         std::pair(CONFIG_ID::STA_SSID, ssid));

                  res.m_code = Response::CODE(200);
                  res.m_format = Response::FORMAT::JSON;
                  cJSON_AddStringToObject(res.m_json, "message", "Got it, thanks");
                  return eResult::SUCCESS;
              });

    vTaskDelay(5_sec);

    // db.Set(std::pair(CONFIG_ID::STA_PASSWORD, "11112222"),
    //        std::pair(CONFIG_ID::STA_SSID, "suannai"));

    vTaskDelay(100000000);
}