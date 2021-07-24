#ifndef ESPARRAG_LOCK__
#define ESPARRAG_LOCK__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esparrag_common.h"

struct Lock
{
    Lock(SemaphoreHandle_t mutex) : m_mutex(mutex)
    {
        ESPARRAG_ASSERT(m_mutex != nullptr);
        ESPARRAG_ASSERT(pdTRUE == xSemaphoreTake(m_mutex, DEFAULT_FREERTOS_TIMEOUT));
    }

    ~Lock()
    {
        ESPARRAG_ASSERT(pdTRUE == xSemaphoreGive(m_mutex));
    }

    SemaphoreHandle_t m_mutex;
};

#endif