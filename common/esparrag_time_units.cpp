#include "esparrag_time_units.h"

Minutes::operator Seconds() { return Seconds(m_minutes * 60); }
Minutes::operator MilliSeconds() { return MilliSeconds(m_minutes * 60 * 1000); }
Minutes::operator MicroSeconds() { return MicroSeconds(m_minutes * 60 * 1000 * 1000); }
Seconds::operator Minutes() { return Minutes(m_seconds / 60); }
Seconds::operator MilliSeconds() { return MilliSeconds(m_seconds * 1000); }
Seconds::operator MicroSeconds() { return MicroSeconds(m_seconds * 1000 * 1000); }
MilliSeconds::operator Minutes() { return Minutes(m_ms / 1000 / 60); }
MilliSeconds::operator Seconds() { return Seconds(m_ms / 1000); }
MilliSeconds::operator MicroSeconds() { return MicroSeconds(m_ms * 1000); }
MicroSeconds::operator Seconds() { return Seconds(m_us / 1000 / 1000); }
MicroSeconds::operator MilliSeconds() { return MilliSeconds(m_us / 1000); }
MicroSeconds::operator Minutes() { return Minutes(m_us / 1000 / 1000 / 60); }
