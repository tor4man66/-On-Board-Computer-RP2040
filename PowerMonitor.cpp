// =====================================================================================================================
// --- Power Monitor Implementation (PowerMonitor.cpp) ---
// =====================================================================================================================
#include "PowerMonitor.h"
#include "Config.h"
#include "DebugUtils.h"

PowerMonitor::PowerMonitor(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance)
: _state(state_manager_instance), _hardware(hardware_manager_instance) {
    log_init_debug("Ініціалізація PowerMonitor...");
}

float PowerMonitor::_get_raw_voltage_value() {
    if (!ENABLE_PIN_BOARD_VOLTAGE_ADC) {
        return -1.0f;
    }
    if (_hardware.get_voltage_adc_pin_num() == -1) {
        return -1.0f;
    }
    return (float)_hardware.get_voltage_adc_raw();
}

void PowerMonitor::update(float interval_sec) {
    unsigned long now = _state.current_time_ms;

    if (ENABLE_PIN_BOARD_VOLTAGE_ADC && _hardware.get_voltage_adc_pin_num() != -1) {
        float raw_voltage_value = _get_raw_voltage_value();
        if (raw_voltage_value >= 0) {
            float actual_voltage = raw_voltage_value * VOLTAGE_CALIBRATION_FACTOR;
            _state.current_battery_voltage = actual_voltage;
            _state._last_voltage_update_time_ms = now;

            float v_for_corr = actual_voltage;
            if (v_for_corr <= MIN_VOLTAGE_FOR_DEADTIME_CORR) {
                v_for_corr = REFERENCE_VOLTAGE_FOR_DEADTIME_CORR;
            }
            float voltage_diff = REFERENCE_VOLTAGE_FOR_DEADTIME_CORR - v_for_corr;
            float new_dt = INJ_DEAD_TIME_US + (voltage_diff * INJ_VOLT_SENSITIVITY);
            _state.dynamic_dead_time_us = (int)max(0.0f, min((float)MAX_DYNAMIC_DEAD_TIME_US, new_dt));
        } else {
            _state.current_battery_voltage = 0.0f;
            _state.dynamic_dead_time_us = 0;
        }
    } else {
        _state.current_battery_voltage = 0.0f;
        _state.dynamic_dead_time_us = 0;
    }

    if (!ENABLE_PIN_ACC) {
        _state.acc_present = true;
        _state.is_dormant_mode = false;
        _state.acc_just_lost = false;
        _state.acc_loss_initiated = false;
        _state.display_ready_for_dormant = false;
        return;
    }

    bool acc_is_on = _hardware.get_acc_status();
    if (acc_is_on) {
        if (!_state.acc_present) {
            log_debug("ACC ON: Живлення повернулося.");
        }
        _state.acc_present = true;
        _state.acc_lost_time_ms = 0;
        _state.is_dormant_mode = false;
        _state.acc_just_lost = false;
        _state.acc_loss_initiated = false;
        _state.display_ready_for_dormant = false;
    } else {
        if (_state.acc_present) {
            _state.acc_present = false;
            _state.acc_lost_time_ms = now;
            _state.acc_just_lost = true;
            _state.acc_loss_initiated = true;
            _state.display_ready_for_dormant = false;
            log_debug_str(String("ACC OFF: Запущено відлік до сну (таймаут ") + (float)DORMANT_WAIT_TIMEOUT_MS / 1000.0f + "S).");
        }

        if (!_state.is_dormant_mode && _state.acc_lost_time_ms > 0) {
            unsigned long time_since_acc_lost = now - _state.acc_lost_time_ms;
            bool should_enter_dormant = false;

            if (_state.display_ready_for_dormant && time_since_acc_lost >= DORMANT_MIN_GRACE_PERIOD_MS) {
                log_debug_str(String("Дисплей готовий до сну (") + (float)time_since_acc_lost / 1000.0f + "S). Перехід до сну.");
                should_enter_dormant = true;
            } else if (time_since_acc_lost >= DORMANT_WAIT_TIMEOUT_MS) {
                log_debug_str(String("Таймаут Dormant Mode (") + (float)DORMANT_WAIT_TIMEOUT_MS / 1000.0f + "S) вичерпано. Примусовий перехід до сну.");
                should_enter_dormant = true;
            }

            if (should_enter_dormant) {
                _state.is_dormant_mode = true;
                _state.acc_just_lost = false;
                _state.acc_loss_initiated = false;
                _state.display_ready_for_dormant = false;
            }
        }
    }
}
