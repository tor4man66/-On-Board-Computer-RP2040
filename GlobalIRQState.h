// =====================================================================================================================
// --- Global IRQ State (GlobalIRQState.h) ---
// =====================================================================================================================
#ifndef GLOBAL_IRQ_STATE_H
#define GLOBAL_IRQ_STATE_H

extern volatile unsigned long _global_total_pulse_time_us;
extern volatile unsigned long _global_current_inj_raw_duration_us;
extern volatile unsigned long _global_last_pulse_edge_us;
extern volatile unsigned long _global_last_inj_irq_time_us;

extern volatile unsigned long _global_last_ignition_period_us;
extern volatile unsigned long _global_last_ignition_start_us;
extern volatile unsigned long _global_last_ignition_irq_time_us;

extern volatile unsigned int _global_vss_pulse_count;
extern volatile unsigned long _global_last_vss_pulse_us;
extern volatile unsigned long _global_last_vss_pulse_ms_irq;

#endif // GLOBAL_IRQ_STATE_H
