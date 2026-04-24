// =====================================================================================================================
// --- Sensor Functions Implementation (SensorFunctions.cpp) ---
// =====================================================================================================================
#include "SensorFunctions.h"
#include "Config.h"
#include "Icons.h"
#include "HardwareManager.h"
#include "DebugUtils.h"

ErrorIconType check_brake_fluid_error(HardwareManager& hardware_manager) {
    if (!ENABLE_PIN_SENSOR_BRAKE_FLUID || hardware_manager.get_brake_fluid_sensor_pin_num() == -1) {
        return ICON_NONE;
    }
    if (hardware_manager.get_brake_fluid_status()) {
        return ICON_BRAKE_FLUID;
    }
    return ICON_NONE;
}

ErrorIconType check_charging_error(bool is_engine_running_stable, float current_battery_voltage) {
    if (!ENABLE_PIN_BOARD_VOLTAGE_ADC) return ICON_NONE;
    if (!is_engine_running_stable) {
        return ICON_NONE;
    }
    if (current_battery_voltage < 0.001f) {
        return ICON_NONE;
    }
    if (current_battery_voltage < NO_CHARGE_THRESHOLD_VOLTAGE) {
        return ICON_NO_CHARGE;
    }
    return ICON_NONE;
}

ErrorIconType check_coolant_error(HardwareManager& hardware_manager) {
    if (!ENABLE_PIN_SENSOR_OVERHEAT_AND_LOW_COOLANT || hardware_manager.get_overheat_and_low_coolant_sensor_pin_num() == -1) {
        return ICON_NONE;
    }
    if (hardware_manager.get_overheat_and_low_coolant_status()) {
        return ICON_OVERHEAT_COOLANT;
    }
    return ICON_NONE;
}

ErrorIconType check_oil_pressure_error(HardwareManager& hardware_manager, bool is_engine_running_stable,
                                       unsigned long engine_start_time_ms, unsigned long current_time_ms, float current_rpm) {
    bool low_pressure_active = false;
    bool high_pressure_active = false;

    if (ENABLE_PIN_SENSOR_OIL_PRESSURE_0_3 && hardware_manager.get_oil_pressure_0_3_sensor_pin_num() != -1) {
        if (hardware_manager.get_oil_pressure_0_3_status()) {
            low_pressure_active = true;
        }
    }
    if (ENABLE_PIN_SENSOR_OIL_PRESSURE_1_8 && hardware_manager.get_oil_pressure_1_8_sensor_pin_num() != -1) {
        if (hardware_manager.get_oil_pressure_1_8_status()) {
            high_pressure_active = true;
        }
    }

    if (is_engine_running_stable && engine_start_time_ms > 0) {
        unsigned long time_since_engine_start = current_time_ms - engine_start_time_ms;
        if (time_since_engine_start > OIL_CHECK_DELAY_MS) {
            if (low_pressure_active) {
                return ICON_OIL_PRESSURE;
            }
            if (current_rpm >= MIN_RPM_FOR_HIGH_PRESSURE_CHECK) {
                if (!high_pressure_active) {
                    return ICON_OIL_PRESSURE;
                }
            }
        }
    } else if (!is_engine_running_stable && low_pressure_active) {
        return ICON_NONE;
    }
    return ICON_NONE;
}

unsigned long _last_towing_detection_log_time_ms = 0;

bool check_towing_mode_status(unsigned long last_vss_activity_ms, float current_rpm,
                              bool is_engine_running, unsigned long current_time_ms,
                              float current_speed_kmh) {
    bool vehicle_is_moving = (current_speed_kmh >= TOWING_MODE_MIN_SPEED_KMH);
    bool rpm_low = current_rpm <= TOWING_MODE_MAX_RPM;
    bool engine_off_or_unstable = !is_engine_running;
    bool is_towing = vehicle_is_moving && rpm_low && engine_off_or_unstable;

    if (is_towing) {
        if ((current_time_ms - _last_towing_detection_log_time_ms) > 5000UL) {
            log_debug_str(String("Детектовано режим буксирування: Швидкість=") + current_speed_kmh + " км/год, Оберти=" + (int)current_rpm + ", Двигун вимкнено=" + (engine_off_or_unstable?"true":"false"));
            _last_towing_detection_log_time_ms = current_time_ms;
        }
    }
    return is_towing;
}