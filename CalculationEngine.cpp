// =====================================================================================================================
// --- Calculation Engine Implementation (CalculationEngine.cpp) ---
// =====================================================================================================================
#include "CalculationEngine.h"
#include "Config.h"
#include "DebugUtils.h"

CalculationEngine::CalculationEngine(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance,
                                     FuelMonitor& fuel_monitor_instance, DataPersistence& data_persistence_instance)
: _state(state_manager_instance),
_fuel_monitor(fuel_monitor_instance), _data_persistence(data_persistence_instance) {
    log_init_debug("Ініціалізація CalculationEngine...");
}

void CalculationEngine::process_periodic_updates(float interval_sec, int vss_pulses, unsigned long inj_total_raw_time_us) {
    _check_towing_mode();

    float distance_km_current_interval, volume_L_current_interval, current_speed_kmh, raw_volume_l_per_h;
    _calculate_sensor_data(vss_pulses, inj_total_raw_time_us, interval_sec,
                           &distance_km_current_interval, &volume_L_current_interval, &current_speed_kmh, &raw_volume_l_per_h);

    float smoothed_val = _update_main_display_value(raw_volume_l_per_h, current_speed_kmh, distance_km_current_interval, volume_L_current_interval);
    _update_trip_and_persistence(distance_km_current_interval, volume_L_current_interval, current_speed_kmh);

    _state.current_speed = current_speed_kmh;
    _state.smoothed_val = smoothed_val;
}

void CalculationEngine::_check_towing_mode() {
    bool towing_mode_detected = check_towing_mode_status(
        _state.last_vss_activity_time_ms,
        _state.rpm,
        _state.is_engine_running,
        _state.current_time_ms,
        _state.current_speed
    );
    if (towing_mode_detected && !_state.is_towing_mode_active) {
        _state.is_towing_mode_active = true;
        log_debug("Детектовано режим буксирування.");
    } else if (!towing_mode_detected && _state.is_towing_mode_active) {
        _state.is_towing_mode_active = false;
        log_debug("Режим буксирування деактивовано.");
    }
}

void CalculationEngine::_calculate_sensor_data(int pulses_to_process, unsigned long pulse_time_to_process_us, float interval_sec,
                                               float* distance_km_current_interval_ptr, float* volume_L_current_interval_ptr,
                                               float* current_speed_kmh_ptr, float* raw_volume_l_per_h_ptr) {

    if (!ENABLE_PIN_VSS) {
        *distance_km_current_interval_ptr = 0.0f;
        *current_speed_kmh_ptr = 0.0f;
    } else {
        *distance_km_current_interval_ptr = (float)pulses_to_process / VSS_IMPULSES_PER_KM;

        *current_speed_kmh_ptr = (interval_sec > MIN_INTERVAL_FOR_SPEED_CALC) ?
        (*distance_km_current_interval_ptr / (interval_sec / SECONDS_IN_HOUR)) : 0.0f;
    }

    if (!ENABLE_PIN_INJ) {
        *volume_L_current_interval_ptr = 0.0f;
        *raw_volume_l_per_h_ptr = 0.0f;
    } else {
        float FUEL_RATE_L_PER_US = 0.0f;
        if (ML_PER_MIN_TO_L_PER_US_DENOMINATOR > 0.001f) {
            FUEL_RATE_L_PER_US = INJ_FLOW_RATE_ML_PER_MIN / ML_PER_MIN_TO_L_PER_US_DENOMINATOR;
        }

        *volume_L_current_interval_ptr = (_state.is_engine_running_stable) ? ((float)pulse_time_to_process_us * FUEL_RATE_L_PER_US) : 0.0f;

        *raw_volume_l_per_h_ptr = (interval_sec > MIN_INTERVAL_FOR_VOLUME_CALC) ?
        (*volume_L_current_interval_ptr / (interval_sec / SECONDS_IN_HOUR)) : 0.0f;
    }
}

float CalculationEngine::_update_main_display_value(float raw_volume_l_per_h, float current_speed_kmh, float distance_km_current_interval, float volume_L_current_interval) {
    bool can_show_l100km = (current_speed_kmh >= MIN_SPEED_FOR_L100KM_KMH) &&
                           (_state.get_trip_distance_travelled_km() >= MIN_DISTANCE_FOR_L100KM_KM);

    float temp_raw_val = 0.0f;
    if (can_show_l100km) {
        if (max(distance_km_current_interval, MIN_DISTANCE_FOR_L100KM_CALC) > 0.001f) {
            temp_raw_val = (volume_L_current_interval / max(distance_km_current_interval, MIN_DISTANCE_FOR_L100KM_CALC)) * 100.0f;
        } else {
            temp_raw_val = 0.0f;
        }
    } else {
        temp_raw_val = raw_volume_l_per_h;
    }

    String current_unit = can_show_l100km ? UNIT_TEXT_L100KM : UNIT_TEXT_LH;
    if (current_unit != _state.last_display_unit) {
        for (int i = 0; i < MAIN_VAL_BUFFER_SIZE; ++i) {
            _state.main_val_buffer[i] = temp_raw_val;
        }
        _state.last_display_unit = current_unit;
        log_debug_str(String("Перемикання одиниць відображення на ") + current_unit + ".");
    }

    for (int i = 0; i < MAIN_VAL_BUFFER_SIZE - 1; ++i) {
        _state.main_val_buffer[i] = _state.main_val_buffer[i + 1];
    }
    _state.main_val_buffer[MAIN_VAL_BUFFER_SIZE - 1] = temp_raw_val;

    float sum_val = 0.0f;
    for (int i = 0; i < MAIN_VAL_BUFFER_SIZE; ++i) {
        sum_val += _state.main_val_buffer[i];
    }
    float smoothed_val = sum_val / (float)MAIN_VAL_BUFFER_SIZE;
    return smoothed_val;
}

void CalculationEngine::_update_trip_and_persistence(float distance_km_current_interval, float volume_L_current_interval, float current_speed_kmh) {
    if (_state.is_towing_mode_active) {
        return;
    }

    if (_state.get_trip_fuel_consumed_L() > MAX_TRIP_LITERS || _state.get_trip_distance_travelled_km() > MAX_TRIP_DISTANCE) {
        _state.set_trip_fuel_consumed_L(0.0f);
        _state.set_trip_distance_travelled_km(0.0f);
        log_debug("Лічильники TRIP скинуто через перевищення максимуму.");
    }

    _state.set_trip_fuel_consumed_L(_state.get_trip_fuel_consumed_L() + volume_L_current_interval);
    _state.set_trip_distance_travelled_km(_state.get_trip_distance_travelled_km() + distance_km_current_interval);

    if (current_speed_kmh >= MIN_SPEED_FOR_PERS_COUNT_KMH) {
        _state.set_persistent_trip_fuel_L(_state.get_persistent_trip_fuel_L() + volume_L_current_interval);
        _state.set_persistent_trip_distance_km(_state.get_persistent_trip_distance_km() + distance_km_current_interval);
    }
    _data_persistence.reset_persistent_trip();
}