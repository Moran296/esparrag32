#ifndef ESPARRAG_DATA_H__
#define ESPARRAG_DATA_H__

#include <cstring>
#include <type_traits>
#include "esparrag_common.h"
#include "esparrag_nvs.h"
#include "esparrag_log.h"
#include "etl/string.h"

template <size_t ID, class T>
struct Data
{
    Data(T min, T max, T defult, bool persistent = true) : m_max(max),
                                                           m_min(min),
                                                           m_default(defult),
                                                           m_val(defult),
                                                           m_isPersistent(persistent)
    {
        ESPARRAG_ASSERT(isValid(defult));
        *this = defult;
    }

    Data(T defult, bool persistent = true) : m_max(),
                                             m_min(),
                                             m_default(defult),
                                             m_val(defult),
                                             m_isPersistent(persistent)
    {
        *this = defult;
    }

    size_t m_id = ID;
    const T m_max;
    const T m_min;
    const T m_default;
    T m_val;
    bool m_isPersistent;

    void operator=(T newVal) { m_val = newVal; }
    bool operator==(T newVal) { return newVal == m_val; }
    bool operator!=(T newVal) { return !(newVal == m_val); }
    void Write(NVS &nvs, const char *key)
    {
        if (!m_isPersistent)
            return;

        eResult res = nvs.SetBlob(key, &m_val, sizeof(T));
        if (res != eResult::SUCCESS)
            ESPARRAG_LOG_ERROR("write flash failed. data %d", m_id);
    }

    void Read(NVS &nvs, const char *key)
    {
        if (!m_isPersistent)
            return;

        size_t actualLen = 0;
        eResult res = nvs.GetBlob(key, &m_val, sizeof(T), actualLen);
        if (res != eResult::SUCCESS && res != eResult::ERROR_FLASH_NOT_FOUND)
            ESPARRAG_LOG_ERROR("problem reading flash");
        else if (actualLen > 0)
            ESPARRAG_LOG_INFO("data %d read from flash - %d bytes", m_id, actualLen);
    }

    bool isValid(const T &newVal)
    {
        if (m_max == m_min)
            return true;

        return (m_min < newVal || m_min == newVal) && (newVal < m_max || newVal == m_max);
    }
};

template <size_t ID>
struct Data<ID, const char *>
{
    Data(const char *defult, bool persistent = true) : m_default(defult),
                                                       m_isPersistent(persistent)
    {
        ESPARRAG_ASSERT(isValid(defult));
        *this = defult;
    }

    static constexpr size_t m_size = 33;
    size_t m_id = ID;
    char m_val[m_size]{};
    const char *m_default;
    bool m_isPersistent;
    void Write(NVS &nvs, const char *key)
    {
        if (!m_isPersistent)
            return;

        eResult res = nvs.SetBlob(key, m_val, strlen(m_val) + 1);
        if (res != eResult::SUCCESS)
            ESPARRAG_LOG_ERROR("write flash failed. data %d", m_id);
    }

    void Read(NVS &nvs, const char *key)
    {
        if (!m_isPersistent)
            return;

        size_t actualLen = 0;
        eResult res = nvs.GetBlob(key, m_val, m_size, actualLen);
        if (res != eResult::SUCCESS && res != eResult::ERROR_FLASH_NOT_FOUND)
            ESPARRAG_LOG_ERROR("problem reading flash");
        else if (actualLen > 0)
            ESPARRAG_LOG_INFO("data %d read from flash - %d bytes", m_id, actualLen);
    }

    void operator=(const char *newVal) { strlcpy(m_val, newVal, m_size); }
    bool operator==(const char *newVal) { return strcmp(m_val, newVal) == 0; }
    bool operator!=(const char *newVal) { return strcmp(m_val, newVal) != 0; }
    bool isValid(const char *newVal) { return strlen(newVal) < m_size; }
};

template <size_t ID, size_t SIZE>
struct Data<ID, etl::string<SIZE>>
{
    Data(const char *defult, bool persistent = true) : m_default(defult),
                                                       m_val(defult),
                                                       m_isPersistent(persistent)
    {
    }

    size_t m_id = ID;
    const etl::string<SIZE> m_default;
    etl::string<SIZE> m_val;
    bool m_isPersistent;

    //if data is a class, it must overload all operators <, ==, data(T*), capacity and size
    void operator=(etl::string<SIZE> newVal) { m_val = newVal; }
    bool operator==(etl::string<SIZE> newVal) { return newVal == m_val; }
    bool operator!=(etl::string<SIZE> newVal) { return !(newVal == m_val); }
    void Write(NVS &nvs, const char *key)
    {
        if (!m_isPersistent)
            return;

        eResult res = nvs.SetBlob(key, m_val.begin(), m_val.size());
        if (res != eResult::SUCCESS)
            ESPARRAG_LOG_ERROR("write flash failed. data %d", m_id);
    }

    void Read(NVS &nvs, const char *key)
    {
        if (!m_isPersistent)
            return;

        size_t actualLen = 0;
        char buffer[SIZE]{};
        eResult res = nvs.GetBlob(key, buffer, SIZE, actualLen);
        if (res != eResult::SUCCESS && res != eResult::ERROR_FLASH_NOT_FOUND)
            ESPARRAG_LOG_ERROR("problem reading flash");
        else if (actualLen > 0)
        {
            m_val = buffer;
            ESPARRAG_LOG_INFO("data %d read from flash - %d bytes", m_id, actualLen);
        }
    }

    bool isValid(const etl::string<SIZE> &newVal)
    {
        return true;
    }
};

#endif