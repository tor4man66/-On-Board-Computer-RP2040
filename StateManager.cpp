// =====================================================================================================================
// --- State Manager Implementation (StateManager.cpp) ---
// =====================================================================================================================
#include "StateManager.h"
#include "Config.h"
#include "DebugUtils.h"

StateManager& StateManager::getInstance() {
    static StateManager instance;
    return instance;
}

void StateManager::init_state() {
    error_logger = &project_error_logger;
    for (int i = 0; i < MAIN_VAL_BUFFER_SIZE; ++i) {
        main_val_buffer[i] = 0.0f;
    }
    active_errors[0] = ICON_NONE;
    active_errors_count = 1;

    current_time_ms = millis();
    _last_fuel_update_time_ms = current_time_ms;
    _last_voltage_update_time_ms = current_time_ms;
    last_inj_activity_time_ms = current_time_ms;
    last_vss_activity_time_ms = current_time_ms;
    last_persistent_save_time_ms = current_time_ms;
    last_main_display_mode_save_time_ms = current_time_ms;
    last_brightness_read_ms = current_time_ms;
    last_error_cycle_time_ms = current_time_ms;
    last_blink_toggle_time_ms = current_time_ms;
    alarm_phase_start_time_ms = current_time_ms;
    non_critical_last_state_change_time_ms = current_time_ms;
    startup_phase_start_ms = current_time_ms;

    log_init_debug("StateManager ініціалізовано.");
}

void StateManager::set_temp_display_mode(String mode) {
    _temp_display_mode = mode;
    _temp_display_mode_start_time = millis();
}

String StateManager::get_and_reset_temp_display_mode() {
    if (_temp_display_mode == "DTC_CLEANING") {
        if (millis() - _temp_display_mode_start_time >= DTC_CLEANING_DISPLAY_TIME_MS) {
            _temp_display_mode = "";
        }
    }
    return _temp_display_mode;
}

float StateManager::get_trip_fuel_consumed_L() { return _trip_fuel_consumed_L; }
void StateManager::set_trip_fuel_consumed_L(float value) {
    if (_trip_fuel_consumed_L != value) {
        _trip_fuel_consumed_L = value;
        persistent_data_dirty = true;
    }
}

float StateManager::get_trip_distance_travelled_km() { return _trip_distance_travelled_km; }
void StateManager::set_trip_distance_travelled_km(float value) {
    if (_trip_distance_travelled_km != value) {
        _trip_distance_travelled_km = value;
        persistent_data_dirty = true;
    }
}

float StateManager::get_persistent_trip_fuel_L() { return _persistent_trip_fuel_L; }
void StateManager::set_persistent_trip_fuel_L(float value) {
    if (_persistent_trip_fuel_L != value) {
        _persistent_trip_fuel_L = value;
        persistent_data_dirty = true;
    }
}

float StateManager::get_persistent_trip_distance_km() { return _persistent_trip_distance_km; }
void StateManager::set_persistent_trip_distance_km(float value) {
    if (_persistent_trip_distance_km != value) {
        _persistent_trip_distance_km = value;
        persistent_data_dirty = true;
    }
}

int StateManager::get_current_main_display_mode() { return _current_main_display_mode; }
void StateManager::set_current_main_display_mode(int value) {
    if (_current_main_display_mode != value) {
        _current_main_display_mode = value;
        persistent_data_dirty = true;
    }
}
