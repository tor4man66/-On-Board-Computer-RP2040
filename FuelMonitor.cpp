// =====================================================================================================================
// --- Fuel Monitor Implementation (FuelMonitor.cpp) ---
// =====================================================================================================================
#include "FuelMonitor.h"
#include "Config.h"
#include "DebugUtils.h"

FuelMonitor::FuelMonitor(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance)
: _state(state_manager_instance), _hardware(hardware_manager_instance) {
    log_init_debug("Ініціалізація FuelMonitor...");
    for (int i = 0; i < FUEL_BUFFER_SIZE; ++i) {
        _fuel_buffer[i] = 0.0f;
    }
}

void FuelMonitor::initialize_fuel_buffer(float initial_value) {
    for (int i = 0; i < FUEL_BUFFER_SIZE; ++i) {
        _fuel_buffer[i] = initial_value;
    }
    _state.last_smoothed_fuel_percent = initial_value;
}

float FuelMonitor::get_raw_fuel_percent() {
    if (!ENABLE_PIN_FUEL_LEVEL_ADC || _hardware.get_fuel_level_adc_pin_num() == -1) {
        _state.fuel_level_available = false;
        return -1.0f;
    }

    int raw_adc_readings[FUEL_ADC_READINGS_FOR_MEDIAN];
    for (int i = 0; i < FUEL_ADC_READINGS_FOR_MEDIAN; ++i) {
        raw_adc_readings[i] = _hardware.get_fuel_adc_raw();
    }

    for (int i = 0; i < FUEL_ADC_READINGS_FOR_MEDIAN - 1; ++i) {
        for (int j = i + 1; j < FUEL_ADC_READINGS_FOR_MEDIAN; ++j) {
            if (raw_adc_readings[i] > raw_adc_readings[j]) {
                int temp = raw_adc_readings[i];
                raw_adc_readings[i] = raw_adc_readings[j];
                raw_adc_readings[j] = temp;
            }
        }
    }

    _state.fuel_level_available = true;
    float raw_adc_value = (float)raw_adc_readings[FUEL_ADC_READINGS_FOR_MEDIAN / 2];

    float adc_range = FUEL_ADC_MAX_RAW_SCALED - FUEL_ADC_MIN_RAW_SCALED;
    if (adc_range <= 0.001f) {
        return 0.0f;
    }

    float percent = (raw_adc_value - FUEL_ADC_MIN_RAW_SCALED) / adc_range;
    percent = max(0.0f, min(1.0f, percent));
    return percent * 100.0f;
}

void FuelMonitor::process_fuel_smoothing() {
    float new_raw_percent = get_raw_fuel_percent();
    unsigned long current_time_ms = _state.current_time_ms;

    if (new_raw_percent < 0) {
        _state.last_smoothed_fuel_percent = -1.0f;
        _state._last_fuel_update_time_ms = current_time_ms;
        for(int i = 0; i < FUEL_BUFFER_SIZE; ++i) _fuel_buffer[i] = 0.0f;
        return;
    }

    unsigned long time_diff_ms = current_time_ms - _state._last_fuel_update_time_ms;
    float time_diff_sec = (float)time_diff_ms / 1000.0f;
    float effective_time_diff_sec = time_diff_sec;
    if (time_diff_sec == 0 || time_diff_sec < FUEL_SMOOTHING_MIN_EFFECTIVE_INTERVAL_SEC) {
        effective_time_diff_sec = UPDATE_INTERVAL_SEC;
    }

    _fuel_buffer[_fuel_buffer_idx] = new_raw_percent;
    _fuel_buffer_idx = (_fuel_buffer_idx + 1) % FUEL_BUFFER_SIZE;

    float sum_fuel = 0.0f;
    for (int i = 0; i < FUEL_BUFFER_SIZE; ++i) {
        sum_fuel += _fuel_buffer[i];
    }
    float current_smoothed_percent = sum_fuel / (float)FUEL_BUFFER_SIZE;

    if (_state.last_smoothed_fuel_percent >= 0) {
        float max_change = FUEL_MAX_PERCENT_CHANGE_PER_SEC * effective_time_diff_sec;
        if (current_smoothed_percent > _state.last_smoothed_fuel_percent + max_change) {
            current_smoothed_percent = _state.last_smoothed_fuel_percent + max_change;
        } else if (current_smoothed_percent < _state.last_smoothed_fuel_percent - max_change) {
            current_smoothed_percent = _state.last_smoothed_fuel_percent - max_change;
        }
    }

    _state.last_smoothed_fuel_percent = current_smoothed_percent;
    _state._last_fuel_update_time_ms = current_time_ms;
}

float FuelMonitor::get_smoothed_fuel_percent() {
    return _state.last_smoothed_fuel_percent;
}

void FuelMonitor::update(float interval_sec) {
    process_fuel_smoothing();
}
