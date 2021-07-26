#ifndef ESPARRAG_MANAGER
#define ESPARRAG_MANAGER

#include "esparrag_wifi.h"
#include "executioner.h"
#include "esparrag_database.h"
#include "esparrag_http.h"
#include "esparrag_common.h"
#include "esparrag_time_units.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

class EsparragManager
{
public:
    void
    Run() {}

    eResult Execute(etl::delegate<void(void)> func, MilliSeconds delay = 0);

private:
    static void entryFunction(void *arg);
    void initComponents();
    void handleEvents();

    TaskHandle_t m_task;
    EventGroupHandle_t m_eventGroup;
    Wifi m_wifi;
    HttpServer m_server;
    Executioner<etl::delegate<void(void)>> m_executioner;
};

#endif