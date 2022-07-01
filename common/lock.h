#ifndef ESPARRAG_LOCK__
#define ESPARRAG_LOCK__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esparrag_common.h"
#include <shared_mutex>
#include <type_traits>

template <class T>
struct Lock
{
    Lock(T &mutex) : m_mutex(mutex)
    {
        if constexpr (std::is_same_v<T, SemaphoreHandle_t>)
        {
            ESPARRAG_ASSERT(m_mutex != nullptr);
            ESPARRAG_ASSERT(pdTRUE == xSemaphoreTake(m_mutex, DEFAULT_FREERTOS_TIMEOUT));
        }
        else
        {
            m_mutex.lock();
        }
    }

    ~Lock()
    {
        if constexpr (std::is_same_v<T, SemaphoreHandle_t>)
        {
            ESPARRAG_ASSERT(pdTRUE == xSemaphoreGive(m_mutex));
        }
        else
        {
            m_mutex.unlock();
        }
    }

    T &m_mutex;
};

class SharedLock
{
public:
    SharedLock(std::shared_mutex &mutex) : m_mutex(mutex)
    {
        m_mutex.lock_shared();
    }

    ~SharedLock()
    {
        m_mutex.unlock_shared();
    }

    std::shared_mutex &m_mutex;
};

#endif