#ifndef __REAL_ANY_EDGE_H__
#define __REAL_ANY_EDGE_H__

#include "debouncer.h"
#include "esparrag_gpio.h"
#include <functional>
#include "esparrag_time_units.h"

class RealAnyEdge
{
public:
    RealAnyEdge(GPI& gpi, MicroSeconds debounce);
    void RegisterCallback(std::function<void(bool state)> cb);

private:
    static void isr_func(void* arg);
    void toggleInterrupt();

    GPI& m_gpi;
    bool m_state;
    std::function<void(bool state)> m_callback;
    Debouncer m_debouncer;
};




#endif