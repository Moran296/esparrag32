#ifndef ESPARRAG_TIME_UNITS_H__
#define ESPARRAG_TIME_UNITS_H__

#include "esparrag_common.h"

class Seconds;
class MS;
class uS;

class Minutes
{
public:
    Minutes(uint32_t minutes) : m_minutes(minutes) {}
    Minutes() : m_minutes(0) {}
    operator Seconds();
    operator MS();
    operator uS();
    operator int() const { return m_minutes; }
    uint32_t value() const { return m_minutes; }
    uint32_t size() const { return sizeof(m_minutes); }
    void *data() { return &m_minutes; }

    bool operator==(const Minutes &rhs) const { return m_minutes == rhs.m_minutes; }
    bool operator!=(const Minutes &rhs) const { return m_minutes != rhs.m_minutes; }

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
    operator MS();
    operator Minutes();
    operator uS();
    operator int() const { return m_seconds; }
    uint32_t value() const { return m_seconds; }
    uint32_t size() const { return sizeof(m_seconds); }
    void *data() { return &m_seconds; }
    TickType_t toTicks() const { return pdMS_TO_TICKS(m_seconds * 1000); }

    bool operator==(const Seconds &rhs) const { return m_seconds == rhs.m_seconds; }
    bool operator!=(const Seconds &rhs) const { return m_seconds != rhs.m_seconds; }

    Seconds &operator=(Seconds const &) = default;
    Seconds(Seconds const &) = default;

private:
    uint32_t m_seconds;
};

class MS
{
public:
    MS(uint32_t ms) : m_ms(ms) {}
    MS() : m_ms(0) {}
    operator Minutes();
    operator Seconds();
    operator uS();
    operator int() const { return m_ms; }
    TickType_t toTicks() const { return pdMS_TO_TICKS(m_ms); }

    uint64_t value() const { return m_ms; }
    uint32_t size() const { return sizeof(m_ms); }
    void *data() { return &m_ms; }

    bool operator==(const MS &rhs) const { return m_ms == rhs.m_ms; }
    bool operator!=(const MS &rhs) const { return m_ms != rhs.m_ms; }

    MS &operator=(MS const &) = default;
    MS(MS const &) = default;

private:
    uint64_t m_ms;
};

class uS
{
public:
    uS(uint64_t us) : m_us(us) {}
    uS() : m_us(0) {}
    operator Minutes();
    operator Seconds();
    operator MS();
    operator int() const { return m_us; }
    uint64_t value() const { return m_us; }
    uint32_t size() const { return sizeof(m_us); }
    void *data() { return &m_us; }

    bool operator==(const uS &rhs) const { return m_us == rhs.m_us; }
    bool operator!=(const uS &rhs) const { return m_us != rhs.m_us; }

    uS &operator=(uS const &) = default;
    uS(uS const &) = default;

private:
    uint64_t m_us;
};

Minutes::operator Seconds() { return Seconds(m_minutes * 60); }
Minutes::operator MS() { return MS(m_minutes * 60 * 1000); }
Minutes::operator uS() { return uS(m_minutes * 60 * 1000 * 1000); }
Seconds::operator Minutes() { return Minutes(m_seconds / 60); }
Seconds::operator MS() { return MS(m_seconds * 1000); }
Seconds::operator uS() { return uS(m_seconds * 1000 * 1000); }
MS::operator Minutes() { return Minutes(m_ms / 1000 / 60); }
MS::operator Seconds() { return Seconds(m_ms / 1000); }
MS::operator uS() { return uS(m_ms * 1000); }
uS::operator Seconds() { return Seconds(m_us / 1000 / 1000); }
uS::operator MS() { return MS(m_us / 1000); }
uS::operator Minutes() { return Minutes(m_us / 1000 / 1000 / 60); }

#endif