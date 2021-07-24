#ifndef ESPARRAG_DATA_H__
#define ESPARRAG_DATA_H__

#include <cstring>
#include "esparrag_common.h"

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

    size_t m_id = ID;
    size_t m_size = sizeof(T);
    const T m_max;
    const T m_min;
    const T m_default;
    T m_val;
    bool m_isPersistent;

    void *operator&() { return &m_val; }
    void operator=(T newVal) { m_val = newVal; }
    bool operator==(T newVal) { return newVal == m_val; }
    bool operator!=(T newVal) { return newVal != m_val; }
    bool isValid(T newVal) { return (newVal <= m_max && newVal >= m_min); }
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

    void *operator&() { return m_val; }
    void operator=(const char *newVal) { strlcpy(m_val, newVal, m_size); }
    bool operator==(const char *newVal) { return strncmp(m_val, newVal, strlen(newVal)) == 0; }
    bool operator!=(const char *newVal) { return strncmp(m_val, newVal, strlen(newVal)) != 0; }
    bool isValid(const char *newVal) { return strlen(newVal) < m_size; }
};

#endif