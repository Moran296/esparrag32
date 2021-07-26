#include "esparrag_manager.h"

void EsparragManager::entryFunction(void *arg)
{
    EsparragManager *manager = reinterpret_cast<EsparragManager *>(arg);
    manager->handleEvents();
}

void EsparragManager::Run()
{
    initComponents();
    BaseType_t res = xTaskCreatePinnedToCore(entryFunction, "esparrag-manager", 8192, this, 3, &m_task, tskNO_AFFINITY);
    ESPARRAG_ASSERT(res == pdPASS);
}

void EsparragManager::initComponents()
{
    ESPARRAG_ASSERT(m_wifi.Init() == eResult::SUCCESS);
    ESPARRAG_ASSERT(m_server.Init() == eResult::SUCCESS);
    m_eventGroup = xEventGroupCreate();
    ESPARRAG_ASSERT(m_eventGroup);
}

void EsparragManager::handleEvents()
{
    for (;;)
    {
        EventBits_t bits = xEventGroupWaitBits(m_eventGroup, 0xffffff, pdTRUE, pdFALSE, portMAX_DELAY);
    }
}
