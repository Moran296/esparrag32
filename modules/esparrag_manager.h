#ifndef ESPARRAG_MANAGER
#define ESPARRAG_MANAGER

#include "esparrag_wifi.h"
#include "esparrag_mqtt.h"
#include "esparrag_database.h"
#include "esparrag_http.h"
#include "esparrag_common.h"
#include "esparrag_time_units.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_bit_defs.h"

class EsparragManager
{
public:
    enum eEsparragEvents
    {
        CONFIG_COMMIT = BIT(0),
        STATUS_COMMIT = BIT(1)
    };

    EsparragManager();
    void Run();
    static void CommitConfig();
    static void CommitStatus();
    static MqttClient &GetMqtt();
    static HttpServer &GetHttp();

private:
    static void entryFunction(void *arg);
    void setEvent(eEsparragEvents event);
    void initComponents();
    void initName();

    void handleEvents();
    void handleCredentials();

    TaskHandle_t m_task;
    EventGroupHandle_t m_eventGroup;
    Wifi m_wifi;
    MqttClient m_mqtt;
    HttpServer m_http;

    EsparragManager(EsparragManager const &) = delete;
    EsparragManager &operator=(EsparragManager const &) = delete;
};

#endif