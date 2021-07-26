#ifndef EXECUTIONER_H__
#define EXECUTIONER_H__

#include "etl/delegate.h"
#include "etl/priority_queue.h"
#include "esparrag_time_units.h"
#include "esparrag_common.h"
#include "freertos/timers.h"

template <class FUNC>
class Executioner
{
public:
    struct executable
    {
        executable(FUNC f, MilliSeconds ms) : m_ms(ms), m_function(f) {}
        executable() {}
        FUNC m_function;
        MilliSeconds m_ms;
        bool operator<(const executable &rhs) { return m_ms < rhs.m_ms; }
    };

    using execute_queue = etl::priority_queue<executable, 10>;

    void Add(FUNC func, MilliSeconds ms);

    xTimerHandle m_timer;
    execute_queue m_queue;
    MicroSeconds m_actionTime{};

    void decrementTimes();
    void schedualer();
    static void timerCb(TimerHandle_t timer)
    {
        Executioner *exec = reinterpret_cast<Executioner *>(pvTimerGetTimerID(timer));
        exec->schedualer();
    }
};

template <class FUNC>
void Executioner<FUNC>::Add(FUNC func, MilliSeconds ms)
{
    if (m_queue.size() == m_queue.max_size())
        return;

    if (!m_timer)
    {
        m_timer = xTimerCreate("executioner", 0, pdFALSE, this, timerCb);
    }
    if (!xTimerIsTimerActive(m_timer))
    {
        xTimerChangePeriod(m_timer, ms.toTicks(), DEFAULT_FREERTOS_TIMEOUT);
        xTimerStart(m_timer, portMAX_DELAY);
        m_actionTime = MicroSeconds(esp_timer_get_time());
    }

    m_queue.push(executable(func, ms));
}

template <class FUNC>
void Executioner<FUNC>::schedualer()
{

    executable exec;
    m_queue.pop_into(exec);
    decrementTimes();

    exec.m_function();

    if (!m_queue.empty)
    {
        xTimerChangePeriod(m_timer, m_queue.begin()->m_ms.toTicks(), DEFAULT_FREERTOS_TIMEOUT);
        xTimerStart(m_timer, DEFAULT_FREERTOS_TIMEOUT);
    }
}

template <class FUNC>
void Executioner<FUNC>::decrementTimes()
{
    MicroSeconds timePassed = MicroSeconds(esp_timer_get_time()) - m_actionTime;
    m_actionTime = MicroSeconds(esp_timer_get_time());

    etl::for_each(m_queue.begin(), m_queue.end(), [&timePassed](auto &exec)
                  {
                      if (exec.m_ms < timePassed)
                          exec.ms = 0;
                      else
                          exec.m_ms -= timePassed;
                  });
}

#endif