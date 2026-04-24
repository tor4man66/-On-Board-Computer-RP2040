// =====================================================================================================================
// --- Sensor Functions (SensorFunctions.h) ---
// =====================================================================================================================
#ifndef SENSOR_FUNCTIONS_H
#define SENSOR_FUNCTIONS_H

#include "Config.h"
#include "Icons.h"
#include "HardwareManager.h"
#include "DebugUtils.h"

ErrorIconType check_brake_fluid_error(HardwareManager& hardware_manager);
ErrorIconType check_coolant_error(HardwareManager& hardware_manager);
ErrorIconType check_oil_pressure_error(HardwareManager& hardware_manager, bool is_engine_running_stable,
                                       unsigned long engine_start_time_ms, unsigned long current_time_ms, float current_rpm);
ErrorIconType check_charging_error(bool is_engine_running_stable, float current_battery_voltage);
bool check_towing_mode_status(unsigned long last_vss_activity_ms, float current_rpm,
                              bool is_engine_running, unsigned long current_time_ms,
                              float current_speed_kmh);

#endif // SENSOR_FUNCTIONS_H
