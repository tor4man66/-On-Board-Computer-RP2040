// =====================================================================================================================
// --- IRQ Handlers (IRQHandlers.h) ---
// =====================================================================================================================
#ifndef IRQ_HANDLERS_H
#define IRQ_HANDLERS_H

#include "Config.h"
#include "GlobalIRQState.h"
#include "DebugUtils.h"

void injector_irq_handler();
void ignition_rpm_irq_handler();
void vss_irq_handler();
void setup_irq_handlers(int inj_pin, int vss_pin, int ignition_rpm_pin);
void read_irq_counters_atomic(int* vss_pulses, unsigned long* inj_total_raw_time_us, int* last_inj_raw_duration,
                              int* last_ignition_period, unsigned long* last_vss_activity_ms);

#endif // IRQ_HANDLERS_H
