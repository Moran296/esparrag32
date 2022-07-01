#include "real_any_edge.h"
#include "esparrag_log.h"


RealAnyEdge::RealAnyEdge(GPI& gpi, MicroSeconds debounce) : m_gpi(gpi),
                                                            m_debouncer(debounce) 
{
    m_state = m_gpi.IsActive();

    m_gpi.SetInterruptType(m_gpi.IsHigh()  ? 
                           GPIO_INTR_LOW_LEVEL : 
                           GPIO_INTR_HIGH_LEVEL);

    m_gpi.RegisterISR(isr_func, this);
    m_gpi.DisableInterrupt();
}

void RealAnyEdge::RegisterCallback(std::function<void(bool state)> cb) {
    m_callback = cb;
    m_gpi.EnableInterrupt();
}


void RealAnyEdge::isr_func(void* arg) {
    RealAnyEdge* self = static_cast<RealAnyEdge*>(arg);

    if(!self->m_debouncer.IsValidNow()) {
        return;
    }

    //disable interrupts
    self->m_gpi.DisableInterrupt();

    //toggle state and call callback
    self->m_state = !self->m_state;
    if (self->m_callback) {
        self->m_callback(self->m_state);
    }

    //toggle the next interrupt
    self->toggleInterrupt();

    //re enable interrupts
    self->m_gpi.EnableInterrupt();
}

void RealAnyEdge::toggleInterrupt() {
    m_gpi.SetInterruptType(m_gpi.GetInterruptType() == GPIO_INTR_LOW_LEVEL ? 
                                                       GPIO_INTR_HIGH_LEVEL : 
                                                       GPIO_INTR_LOW_LEVEL);
}