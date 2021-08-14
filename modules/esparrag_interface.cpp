#include "esparrag_interface.h"
#include "esparrag_manager.h"
#include "esparrag_log.h"

#define IMPLEMENT_HANDLER(NAME) \
    void EsparragInterface::NAME(Request &req, Response &res)

#define ON_HTTP(URI, METHOD, HANDLER) \
    m_http.On(URI,                    \
              eMethod::METHOD,        \
              http_handler_callback::create<HANDLER>())

#define ON_MQTT(TOPIC, HANDLER) \
    m_mqtt.On(TOPIC,            \
              mqtt_handler_callback::create<HANDLER>())

void EsparragInterface::RegisterHandlers()
{
    ON_HTTP("/credentials", POST, credentials_post);
    ON_MQTT("/hello", mqtt_hello);
}

IMPLEMENT_HANDLER(credentials_post)
{
    if (!req.m_content)
    {
        res.m_code = Response::CODE(404);
        res.m_format = Response::FORMAT::JSON;
        cJSON_AddStringToObject(res.m_json, "message", "invalid request");
        return;
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
        return;
    }

    Settings::Config.Set<eConfig::STA_SSID>(ssid);
    Settings::Config.Set<eConfig::STA_PASSWORD>(pass);
    res.m_code = Response::CODE(200);
    res.m_format = Response::FORMAT::JSON;
    cJSON_AddStringToObject(res.m_json, "message", "Got it, thanks");
    EsparragManager::CommitConfig();
    return;
}

IMPLEMENT_HANDLER(mqtt_hello)
{
    ESPARRAG_LOG_INFO("hello to you too");
    if (req.m_content != nullptr)
    {
        char *r = cJSON_Print(req.m_content);
        ESPARRAG_LOG_INFO("payload:");
        ESPARRAG_LOG_INFO("%s", r);
        cJSON_free(r);
    }
}
