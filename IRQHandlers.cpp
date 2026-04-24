// =====================================================================================================================
// --- IRQ Handlers Implementation (IRQHandlers.cpp) ---
// =====================================================================================================================
#include "IRQHandlers.h"
#include "Config.h"
#include "GlobalIRQState.h"
#include "DebugUtils.h"

volatile unsigned long _global_total_pulse_time_us = 0;
volatile unsigned long _global_current_inj_raw_duration_us = 0;
volatile unsigned long _global_last_pulse_edge_us = 0;
volatile unsigned long _global_last_inj_irq_time_us = 0;

volatile unsigned long _global_last_ignition_period_us = 0;
volatile unsigned long _global_last_ignition_start_us = 0;
volatile unsigned long _global_last_ignition_irq_time_us = 0;

volatile unsigned int _global_vss_pulse_count = 0;
volatile unsigned long _global_last_vss_pulse_us = 0;
volatile unsigned long _global_last_vss_pulse_ms_irq = 0;

void injector_irq_handler() {
    unsigned long current_time_us = micros();
    if ((current_time_us - _global_last_inj_irq_time_us) < INJ_IRQ_DEBOUNCE_US) {
        return;
    }
    _global_last_inj_irq_time_us = current_time_us;

    if (digitalRead(PIN_INJ) == LOW) {
        _global_last_pulse_edge_us = current_time_us;
    } else {
        if (_global_last_pulse_edge_us > 0) {
            unsigned long raw_duration = current_time_us - _global_last_pulse_edge_us;
            _global_current_inj_raw_duration_us = raw_duration;
            _global_total_pulse_time_us += raw_duration;
            _global_last_pulse_edge_us = 0;
        }
    }
}

void ignition_rpm_irq_handler() {
    unsigned long current_time_us = micros();
    if ((current_time_us - _global_last_ignition_irq_time_us) < IGNITION_RPM_IRQ_DEBOUNCE_US) {
        return;
    }
    _global_last_ignition_irq_time_us = current_time_us;

    if (digitalRead(PIN_IGNITION_RPM) == LOW) {
        if (_global_last_ignition_start_us != 0) {
            unsigned long period_between_pulses_us = current_time_us - _global_last_ignition_start_us;
            _global_last_ignition_period_us = period_between_pulses_us;
        } else {
            _global_last_ignition_period_us = 0;
        }
        _global_last_ignition_start_us = current_time_us;
    }
}

void vss_irq_handler() {
    unsigned long now_us = micros();
    unsigned long now_ms = millis();
    if ((now_us - _global_last_vss_pulse_us) > VSS_DEBOUNCE_US) {
        _global_vss_pulse_count++;
        _global_last_vss_pulse_us = now_us;
        _global_last_vss_pulse_ms_irq = now_ms;
    }
}

void setup_irq_handlers(int inj_pin, int vss_pin, int ignition_rpm_pin) {
    log_init_debug("Реєстрація IRQ обробників...");
    if (inj_pin != -1) {
        attachInterrupt(digitalPinToInterrupt(inj_pin), injector_irq_handler, CHANGE);
        log_init_debug_str(String("  IRQ для форсунки (GPIO ") + inj_pin + ") - УСПІШНО");
    } else {
        log_init_debug_str(String("  IRQ для форсунки (GPIO ") + PIN_INJ + ") - ПРОПУЩЕНО (пін не ініціалізовано)");
    }
    if (vss_pin != -1) {
        attachInterrupt(digitalPinToInterrupt(vss_pin), vss_irq_handler, FALLING);
        log_init_debug_str(String("  IRQ для VSS (GPIO ") + vss_pin + ") - УСПІШНО");
    } else {
        log_init_debug_str(String("  IRQ для VSS (GPIO ") + PIN_VSS + ") - ПРОПУЩЕНО (пін не ініціалізовано)");
    }
    if (ignition_rpm_pin != -1) {
        attachInterrupt(digitalPinToInterrupt(ignition_rpm_pin), ignition_rpm_irq_handler, FALLING);
        log_init_debug_str(String("  IRQ для RPM (GPIO ") + ignition_rpm_pin + ") - УСПІШНО");
    } else {
        log_init_debug_str(String("  IRQ для RPM (GPIO ") + PIN_IGNITION_RPM + ") - ПРОПУЩЕНО (пін не ініціалізовано)");
    }
    log_init_debug("Реєстрація IRQ обробників - ЗАВЕРШЕНО.");
}

void read_irq_counters_atomic(int* vss_pulses, unsigned long* inj_total_raw_time_us, int* last_inj_raw_duration,
                              int* last_ignition_period, unsigned long* last_vss_activity_ms) {
    noInterrupts();
    *vss_pulses = _global_vss_pulse_count;
    *inj_total_raw_time_us = _global_total_pulse_time_us;
    *last_inj_raw_duration = _global_current_inj_raw_duration_us;
    *last_ignition_period = _global_last_ignition_period_us;
    *last_vss_activity_ms = _global_last_vss_pulse_ms_irq;

    _global_vss_pulse_count = 0;
    _global_total_pulse_time_us = 0;
    _global_current_inj_raw_duration_us = 0;
    _global_last_ignition_period_us = 0;
    interrupts();
                              }
