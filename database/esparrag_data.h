#ifndef ESPARRAG_DATA_H__
#define ESPARRAG_DATA_H__

#include <cstring>
#include "esparrag_common.h"

#define TERNARY_RETURN_CONSTEXPR(CONDITION, TRUE_TYPE, FALSE_TYPE) \
    if constexpr (CONDITION)                                       \
        return TRUE_TYPE;                                          \
    else                                                           \
        return FALSE_TYPE

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

    //if data is a class, it must overload all operators <, ==, data(T*), capacity and size
    void operator=(T newVal) { m_val = newVal; }
    bool operator==(T newVal) { return newVal == m_val; }
    bool operator!=(T newVal) { return !(newVal == m_val); }
    size_t size() const { TERNARY_RETURN_CONSTEXPR(std::is_class_v<T>, m_val.size(), sizeof(T)); }
    size_t capacity() const { TERNARY_RETURN_CONSTEXPR(std::is_class_v<T>, m_val.capacity(), sizeof(T)); }
    void *operator&() { TERNARY_RETURN_CONSTEXPR(std::is_class_v<T>, m_val.data(), &m_val); }

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

    void operator=(const char *newVal) { strlcpy(m_val, newVal, m_size); }
    bool operator==(const char *newVal) { return strcmp(m_val, newVal) == 0; }
    bool operator!=(const char *newVal) { return strcmp(m_val, newVal) != 0; }
    bool isValid(const char *newVal) { return strlen(newVal) < capacity(); }
    size_t size() const { return strlen(m_val); }
    size_t capacity() const { return m_size; }
    void *pointer() { return m_val; }
};

#endif