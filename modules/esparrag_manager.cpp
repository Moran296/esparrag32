#include "esparrag_manager.h"

static EsparragManager *s_this = nullptr;

EsparragManager::EsparragManager()
{
    ESPARRAG_ASSERT(s_this == nullptr);
    s_this = this;
}

void EsparragManager::entryFunction(void *arg)
{
    EsparragManager *manager = reinterpret_cast<EsparragManager *>(arg);
    manager->handleEvents();
}

void EsparragManager::Run()
{
    BaseType_t res = xTaskCreatePinnedToCore(entryFunction, "esparrag-manager", 8192, this, 3, &m_task, tskNO_AFFINITY);
    ESPARRAG_ASSERT(res == pdPASS);
}

void EsparragManager::initComponents()
{
    ESPARRAG_ASSERT(m_wifi.Init() == eResult::SUCCESS);
    ESPARRAG_ASSERT(m_server.Init() == eResult::SUCCESS);
    m_eventGroup = xEventGroupCreate();
    ESPARRAG_LOG_INFO("this %p", this);
    handleCredentials();
    ESPARRAG_ASSERT(m_eventGroup);
}

void EsparragManager::handleEvents()
{
    initComponents();

    ESPARRAG_LOG_INFO("running manager");
    for (;;)
    {
        EventBits_t bits = xEventGroupWaitBits(m_eventGroup, 0xffffff, pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & eEsparragEvents::CONFIG_COMMIT)
        {
            vTaskDelay(MilliSeconds(500).toTicks());
            ESPARRAG_LOG_INFO("event config commit");
            Settings::Config.Commit();
        }

        if (bits & eEsparragEvents::STATUS_COMMIT)
        {
            vTaskDelay(MilliSeconds(500).toTicks());
            ESPARRAG_LOG_INFO("event status commit");
            Settings::Status.Commit();
        }
    }
}

void EsparragManager::setEvent(eEsparragEvents event)
{
    ESPARRAG_LOG_INFO("this %p", this);
    xEventGroupSetBits(m_eventGroup, event);
}

void EsparragManager::CommitConfig()
{
    s_this->setEvent(eEsparragEvents::CONFIG_COMMIT);
}
void EsparragManager::CommitStatus()
{
    s_this->setEvent(eEsparragEvents::STATUS_COMMIT);
}

void EsparragManager::handleCredentials()
{
    m_server.On("/credentials", METHOD::POST,
                [](Request &req, Response &res)
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

                    Settings::Config.Set<CONFIG_ID::STA_SSID>(ssid);
                    Settings::Config.Set<CONFIG_ID::STA_PASSWORD>(pass);
                    res.m_code = Response::CODE(200);
                    res.m_format = Response::FORMAT::JSON;
                    cJSON_AddStringToObject(res.m_json, "message", "Got it, thanks");
                    s_this->setEvent(eEsparragEvents::CONFIG_COMMIT);
                    return eResult::SUCCESS;
                });
}
