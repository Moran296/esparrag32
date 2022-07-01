#include "esparrag_manager.h"
#include "esparrag_mdns.h"
#include "esparrag_log.h"

static EsparragManager *s_this = nullptr;

MqttClient &EsparragManager::GetMqtt()
{
    ESPARRAG_ASSERT(s_this);
    return s_this->m_mqtt;
}

HttpServer &EsparragManager::GetHttp()
{
    ESPARRAG_ASSERT(s_this);
    return s_this->m_http;
}

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
    m_mqtt.Init();

    ESPARRAG_ASSERT(m_http.Init() == eResult::SUCCESS);

    m_wifi.Init();
}

void EsparragManager::handleEvents()
{
    initComponents();

    m_wifi.Connect("*****", "*****");
    while(!m_wifi.IsInState<STATE_Connected>()) {

        vTaskDelay(Seconds(10).toTicks());
        ESPARRAG_LOG_INFO("connecting to wifi.......");
    }

    if (Mdns::Init() != true) {
        ESPARRAG_LOG_ERROR("mdns init failed");
    } else {
        EsparragResult brokerIP = Mdns::FindBroker();
        if (brokerIP.IsError()) {
            ESPARRAG_LOG_ERROR("couldn't find broker");
            PRINT_ERROR_RES(brokerIP);
        } else {
            m_mqtt.TryConnect(brokerIP.ok_or_assert());
        }
    }


    ESPARRAG_LOG_INFO("running manager");
    for (;;)
    {
        vTaskDelay(Seconds(10).toTicks());
    }
}
