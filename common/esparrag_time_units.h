#ifndef ESPARRAG_TIME_UNITS_H__
#define ESPARRAG_TIME_UNITS_H__

#include "esparrag_common.h"

class SECONDS;
class MS;
class uS;

class MINUTES
{
public:
    MINUTES(uint32_t minutes) : m_minutes(minutes) {}
    operator SECONDS();
    operator MS();
    operator uS();
    operator int() { return m_minutes; }
    uint32_t value() { return m_minutes; }

private:
    uint32_t m_minutes;
};

class SECONDS
{
public:
    SECONDS(uint32_t seconds) : m_seconds(seconds) {}
    operator MS();
    operator MINUTES();
    operator uS();
    operator int() { return m_seconds; }
    uint32_t value() { return m_seconds; }
    TickType_t toTicks() { return pdMS_TO_TICKS(m_seconds * 1000); }

private:
    uint32_t m_seconds;
};

class MS
{
public:
    MS(uint32_t ms) : m_ms(ms) {}
    operator MINUTES();
    operator SECONDS();
    operator uS();
    operator int() { return m_ms; }
    uint32_t value() { return m_ms; }
    TickType_t toTicks() { return pdMS_TO_TICKS(m_ms); }

private:
    uint32_t m_ms;
};

class uS
{
public:
    uS(uint64_t us) : m_us(us) {}
    operator MINUTES();
    operator SECONDS();
    operator MS();
    operator int() { return m_us; }
    uint64_t value() { return m_us; }

private:
    uint64_t m_us;
};

MINUTES::operator SECONDS() { return SECONDS(m_minutes * 60); }
MINUTES::operator MS() { return MS(m_minutes * 60 * 1000); }
MINUTES::operator uS() { return uS(m_minutes * 60 * 1000 * 1000); }
SECONDS::operator MINUTES() { return MINUTES(m_seconds / 60); }
SECONDS::operator MS() { return MS(m_seconds * 1000); }
SECONDS::operator uS() { return uS(m_seconds * 1000 * 1000); }
MS::operator MINUTES() { return MINUTES(m_ms / 1000 / 60); }
MS::operator SECONDS() { return SECONDS(m_ms / 1000); }
MS::operator uS() { return uS(m_ms * 1000); }
uS::operator SECONDS() { return SECONDS(m_us / 1000 / 1000); }
uS::operator MS() { return MS(m_us / 1000); }
uS::operator MINUTES() { return MINUTES(m_us / 1000 / 1000 / 60); }

#endif