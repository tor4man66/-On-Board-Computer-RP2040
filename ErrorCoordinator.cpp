// =====================================================================================================================
// --- Error Coordinator Implementation (ErrorCoordinator.cpp) ---
// =====================================================================================================================
#include "ErrorCoordinator.h"
#include "Config.h"
#include "Icons.h"
#include "DebugUtils.h"

ErrorCoordinator::ErrorCoordinator(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance,
                                   FuelMonitor& fuel_monitor_instance, EngineMonitor& engine_monitor_instance)
: _state(state_manager_instance), _hardware(hardware_manager_instance),
_fuel_monitor(fuel_monitor_instance), _engine_monitor(engine_monitor_instance) {
    log_init_debug("Ініціалізація ErrorCoordinator...");
}

uint8_t ErrorCoordinator::get_error_severity_level(ErrorIconType* error_list, int count) {
    if (count == 0 || (count == 1 && error_list[0] == ICON_NONE)) {
        return ERROR_SEVERITY_NONE;
    }

    bool has_critical = false;
    bool has_non_critical = false;

    for (int i = 0; i < count; ++i) {
        ErrorIconType type = error_list[i];
        if (GET_ERROR_ICON(type).is_critical) {
            has_critical = true;
        }
        if (GET_ERROR_ICON(type).severity == ERROR_SEVERITY_NON_CRITICAL) {
            has_non_critical = true;
        }
    }

    if (has_critical) {
        return CRITICAL_ERROR_SEVERITY;
    }
    if (has_non_critical) {
        return ERROR_SEVERITY_NON_CRITICAL;
    }
    return ERROR_SEVERITY_NONE;
}

void ErrorCoordinator::check_for_errors(unsigned long current_time_ms, ErrorIconType* found_errors, int* found_count_ptr) {
    *found_count_ptr = 0;
    int current_idx = 0;

    float current_rpm_safe = _engine_monitor.get_current_rpm();

    if (!_state.is_towing_mode_active) {
        ErrorIconType err_brake = check_brake_fluid_error(_hardware);
        if (err_brake != ICON_NONE) {
            found_errors[current_idx++] = err_brake;
        }

        ErrorIconType err_coolant = check_coolant_error(_hardware);
        if (err_coolant != ICON_NONE) {
            found_errors[current_idx++] = err_coolant;
        }

        ErrorIconType err_oil_pressure = check_oil_pressure_error(
            _hardware,
            _state.is_engine_running_stable,
            _state.engine_start_time_ms,
            current_time_ms,
            current_rpm_safe
        );
        if (err_oil_pressure != ICON_NONE) {
            found_errors[current_idx++] = err_oil_pressure;
        }

        ErrorIconType err_charging = check_charging_error(
            _state.is_engine_running_stable,
            _state.current_battery_voltage
        );
        if (err_charging != ICON_NONE) {
            found_errors[current_idx++] = err_charging;
        }
    }

    float smoothed_fuel_percent = _fuel_monitor.get_smoothed_fuel_percent();
    if (smoothed_fuel_percent < 0 || !ENABLE_PIN_FUEL_LEVEL_ADC) {
        _state.is_low_fuel_active_by_hysteresis = false;
    } else {
        if (_state.is_low_fuel_active_by_hysteresis) {
            if (smoothed_fuel_percent >= FUEL_HIGH_THRESHOLD_PERCENT) {
                _state.is_low_fuel_active_by_hysteresis = false;
            }
        } else {
            if (smoothed_fuel_percent <= FUEL_LOW_THRESHOLD_PERCENT) {
                _state.is_low_fuel_active_by_hysteresis = true;
            }
        }
        if (_state.is_low_fuel_active_by_hysteresis) {
            found_errors[current_idx++] = ICON_LOW_FUEL;
        }
    }

    if (current_idx == 0) {
        found_errors[current_idx++] = ICON_NONE;
    }
    *found_count_ptr = current_idx;
}
