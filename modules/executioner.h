#ifndef EXECUTIONER_H__
#define EXECUTIONER_H__

#include "etl/delegate.h"
#include "etl/priority_queue.h"
#include "esparrag_time_units.h"
#include "esparrag_common.h"
#include "esparrag_log.h"
#include "freertos/timers.h"

template <size_t MAX, class FUNC>
class Executioner
{
public:
    struct executable
    {
        executable(FUNC f, MilliSeconds ms, const char *name) : m_ms(ms), m_function(f), m_name(name), entryTime(esp_timer_get_time()) {}
        executable() {}
        MilliSeconds m_ms;
        FUNC m_function;
        const char *m_name;
        MicroSeconds entryTime;
        bool operator<(const executable &rhs) const { return m_ms < rhs.m_ms; }
        bool operator>(const executable &rhs) const { return m_ms > rhs.m_ms; }
    };

    using execute_queue = etl::priority_queue<executable,
                                              MAX,
                                              etl::vector<executable, MAX>,
                                              etl::greater<executable>>;

    void Add(const char *name, MilliSeconds ms, FUNC func);

    xTimerHandle m_timer;
    execute_queue m_queue;
    MicroSeconds m_actionTime{};

    void schedualer();
    static void timerCb(TimerHandle_t timer)
    {
        Executioner *exec = reinterpret_cast<Executioner *>(pvTimerGetTimerID(timer));
        exec->schedualer();
    }
};

template <size_t MAX, class FUNC>
void Executioner<MAX, FUNC>::Add(const char *name, MilliSeconds ms, FUNC func)
{
    if (m_queue.size() == m_queue.max_size())
        return;

    if (!m_timer)
    {
        m_timer = xTimerCreate("executioner", 0, pdFALSE, this, timerCb);
    }

    m_queue.push(executable(func, ms, name));

    if (!xTimerIsTimerActive(m_timer))
    {
        xTimerChangePeriod(m_timer, m_queue.top().m_ms.toTicks(), DEFAULT_FREERTOS_TIMEOUT);
        xTimerStart(m_timer, portMAX_DELAY);
    }
    else
    {
        schedualer();
    }
}

template <size_t MAX, class FUNC>
void Executioner<MAX, FUNC>::schedualer()
{
    if (m_queue.empty())
        return;

    while (MicroSeconds(esp_timer_get_time()) - m_queue.top().entryTime >= m_queue.top().m_ms)
    {
        auto &exec = m_queue.top();
        ESPARRAG_LOG_INFO("executing function %s", exec.m_name);
        exec.m_function();
        m_queue.pop();
    }

    if (!m_queue.empty())
    {
        xTimerChangePeriod(m_timer, m_queue.top().m_ms.toTicks(), DEFAULT_FREERTOS_TIMEOUT);
        xTimerStart(m_timer, DEFAULT_FREERTOS_TIMEOUT);
    }
}

#endif