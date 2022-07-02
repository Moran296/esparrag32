#ifndef ESPARRAG_MANAGER
#define ESPARRAG_MANAGER

#include "esparrag_common.h"
#include "esparrag_time_units.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_bit_defs.h"

class EsparragManager
{
public:
    EsparragManager();
    void Run();

private:
    static void entryFunction(void *arg);
    void initComponents();

    void handleEvents();

    TaskHandle_t m_task;

    EsparragManager(EsparragManager const &) = delete;
    EsparragManager &operator=(EsparragManager const &) = delete;
};

#endif