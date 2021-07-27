#ifndef ESPARRAG_TIME_UNITS_H__
#define ESPARRAG_TIME_UNITS_H__

#include "freertos/FreeRTOS.h" //for TickType_t

#define OVERLOAD_ALL_COMPARISON_OPERATORS(CLASS, MEMBER)                                 \
    bool operator==(const CLASS &rhs) const { return MEMBER == rhs.MEMBER; }             \
    bool operator!=(const CLASS &rhs) const { return MEMBER != rhs.MEMBER; }             \
    bool operator<(const CLASS &rhs) const { return MEMBER < rhs.MEMBER; }               \
    bool operator>(const CLASS &rhs) const { return MEMBER > rhs.MEMBER; }               \
    bool operator<=(const CLASS &rhs) const { return MEMBER <= rhs.MEMBER; }             \
    bool operator>=(const CLASS &rhs) const { return MEMBER >= rhs.MEMBER; }             \
    const CLASS operator-(const CLASS &rhs) const { return CLASS(MEMBER - rhs.MEMBER); } \
    const CLASS operator+(const CLASS &rhs) const { return CLASS(MEMBER + rhs.MEMBER); } \
    CLASS &operator+=(const CLASS &rhs)                                                  \
    {                                                                                    \
        MEMBER += rhs.MEMBER;                                                            \
        return *this;                                                                    \
    }                                                                                    \
    CLASS &operator-=(const CLASS &rhs)                                                  \
    {                                                                                    \
        MEMBER -= rhs.MEMBER;                                                            \
        return *this;                                                                    \
    }

class Seconds;
class MilliSeconds;
class MicroSeconds;

class Minutes
{
public:
    Minutes(uint32_t minutes) : m_minutes(minutes) {}
    Minutes() : m_minutes(0) {}
    operator Seconds();
    operator MilliSeconds();
    operator MicroSeconds();
    operator int() const { return m_minutes; }
    uint32_t value() const { return m_minutes; }
    size_t size() const { return sizeof(m_minutes); }
    void *data() { return &m_minutes; }

    OVERLOAD_ALL_COMPARISON_OPERATORS(Minutes, m_minutes)
    Minutes &operator=(Minutes const &) = default;
    Minutes(Minutes const &) = default;

private:
    uint32_t m_minutes;
};

class Seconds
{
public:
    Seconds(uint32_t seconds) : m_seconds(seconds) {}
    Seconds() : m_seconds(0) {}
    operator MilliSeconds();
    operator Minutes();
    operator MicroSeconds();
    operator int() const { return m_seconds; }
    uint32_t value() const { return m_seconds; }
    size_t size() const { return sizeof(m_seconds); }
    void *data() { return &m_seconds; }
    TickType_t toTicks() const { return pdMS_TO_TICKS(m_seconds * 1000); }

    OVERLOAD_ALL_COMPARISON_OPERATORS(Seconds, m_seconds)
    Seconds &operator=(Seconds const &) = default;
    Seconds(Seconds const &) = default;

private:
    uint32_t m_seconds;
};

class MilliSeconds
{
public:
    MilliSeconds(uint32_t ms) : m_ms(ms) {}
    MilliSeconds() : m_ms(0) {}
    operator Minutes();
    operator Seconds();
    operator MicroSeconds();
    operator int() const { return m_ms; }
    TickType_t toTicks() const { return pdMS_TO_TICKS(m_ms); }

    uint64_t value() const { return m_ms; }
    size_t size() const { return sizeof(m_ms); }
    void *data() { return &m_ms; }

    OVERLOAD_ALL_COMPARISON_OPERATORS(MilliSeconds, m_ms)
    MilliSeconds &operator=(MilliSeconds const &) = default;
    MilliSeconds(MilliSeconds const &) = default;

private:
    uint64_t m_ms;
};

class MicroSeconds
{
public:
    MicroSeconds(uint64_t us) : m_us(us) {}
    MicroSeconds() : m_us(0) {}
    operator Minutes();
    operator Seconds();
    operator MilliSeconds();
    operator int() const { return m_us; }
    uint64_t value() const { return m_us; }
    size_t size() const { return sizeof(m_us); }
    void *data() { return &m_us; }

    OVERLOAD_ALL_COMPARISON_OPERATORS(MicroSeconds, m_us)
    MicroSeconds &operator=(MicroSeconds const &) = default;
    MicroSeconds(MicroSeconds const &) = default;

private:
    uint64_t m_us;
};

#endif