// =====================================================================================================================
// --- Engine Monitor Implementation (EngineMonitor.cpp) ---
// =====================================================================================================================
#include "EngineMonitor.h"
#include "Config.h"
#include "DebugUtils.h"

EngineMonitor::EngineMonitor(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance)
: _state(state_manager_instance), _hardware(hardware_manager_instance) {
    log_init_debug("Ініціалізація EngineMonitor...");
}

void EngineMonitor::update(float interval_sec, int last_inj_raw_duration, int last_ignition_period, unsigned long last_vss_activity_ms) {
    _state.last_vss_activity_time_ms = last_vss_activity_ms;
    _check_engine_status(_state.current_time_ms);
    _update_rpm_from_raw_data(last_ignition_period);
    _update_injector_pulse_data(last_inj_raw_duration);

    unsigned long now = _state.current_time_ms;
    if ((now - _last_log_time_ms) > 2000UL) {
        if (_state.rpm > 0 || _state.current_speed > 0) {
            float speed = _state.current_speed;
            String log_message = String("Оберти: ") + (int)_state.rpm + " | " +
            "Швидкість: " + (int)speed + " km/h | " +
            "Час впорскування форсунки: " + _state.current_inj_period_us + "us | " +
            "Напруга: " + String(_state.current_battery_voltage, 2) + "V";
            log_debug_str(log_message);
        }
        _last_log_time_ms = now;
    }
}

void EngineMonitor::_check_engine_status(unsigned long current_time_ms) {
    if (!ENABLE_PIN_INJ) return;

    if ((current_time_ms - _state.last_inj_activity_time_ms) > ENGINE_STOP_TIMEOUT_MS) {
        _state.rpm = 0.0f;
        _state.is_engine_running = false;
        _state.is_engine_running_stable = false;
        _state.engine_start_time_ms = 0;
        _state.current_inj_period_us = 0;
        _state.avg_rpm_for_display = 0.0f;
        _state.avg_inj_ms = 0.0f;
    } else {
        _state.is_engine_running = true;
    }
}

void EngineMonitor::_update_rpm_from_raw_data(int period_between_pulses_us) {
    if (!ENABLE_PIN_IGNITION_RPM) {
        _state.rpm = 0.0f;
        return;
    }

    if (period_between_pulses_us > 0) {
        if (MIN_IGNITION_PULSE_PERIOD_US < period_between_pulses_us && period_between_pulses_us < MAX_IGNITION_PULSE_PERIOD_US) {
            float rpm_float = (float)RPM_BASE_FACTOR / (float)period_between_pulses_us / (float)RPM_PULSES_PER_ENGINE_REVOLUTION;
            _state.rpm = max((float)MIN_DISPLAY_RPM, min((float)MAX_DISPLAY_RPM, rpm_float));
        } else {
            _state.rpm = 0.0f;
        }
    } else {
        _state.rpm = 0.0f;
    }

    unsigned long current_time_ms = _state.current_time_ms;
    if (_state.rpm > MIN_RPM_FOR_STABLE_RUNNING) {
        _state.is_engine_running_stable = true;
        _state.last_stable_rpm_time_ms = current_time_ms;
        if (_state.engine_start_time_ms == 0) {
            _state.engine_start_time_ms = current_time_ms;
        }
    } else {
        if ((current_time_ms - _state.last_stable_rpm_time_ms) > ENGINE_RPM_STABILITY_HYSTERESIS_MS) {
            _state.is_engine_running_stable = false;
            _state.engine_start_time_ms = 0;
        }
    }
}

void EngineMonitor::_update_injector_pulse_data(int raw_duration) {
    if (!ENABLE_PIN_INJ) {
        _state.current_inj_period_us = 0;
        return;
    }

    if (raw_duration > 0) {
        int actual_duration = max(0, raw_duration - _state.dynamic_dead_time_us);
        if (actual_duration < MIN_INJ_PULSE_WIDTH_FILTER_US) {
            _state.current_inj_period_us = 0;
            return;
        }
        _state.current_inj_period_us = actual_duration;
        _state.last_inj_activity_time_ms = _state.current_time_ms;
    } else {
        _state.current_inj_period_us = 0;
    }
}

int EngineMonitor::get_current_inj_period() {
    return _state.current_inj_period_us;
}

float EngineMonitor::get_current_rpm() {
    return _state.rpm;
}
