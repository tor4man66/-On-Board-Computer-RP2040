// =====================================================================================================================
// --- Calculation Engine (CalculationEngine.h) ---
// =====================================================================================================================
#ifndef CALCULATION_ENGINE_H
#define CALCULATION_ENGINE_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "HardwareManager.h"
#include "FuelMonitor.h"
#include "DataPersistence.h"
#include "SensorFunctions.h"

class CalculationEngine {
private:
    StateManager& _state;
    FuelMonitor& _fuel_monitor;
    DataPersistence& _data_persistence;

public:
    CalculationEngine(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance,
                      FuelMonitor& fuel_monitor_instance, DataPersistence& data_persistence_instance);

    void process_periodic_updates(float interval_sec, int vss_pulses, unsigned long inj_total_raw_time_us);

private:
    void _check_towing_mode();
    void _calculate_sensor_data(int pulses_to_process, unsigned long pulse_time_to_process_us, float interval_sec,
                                float* distance_km_current_interval_ptr, float* volume_L_current_interval_ptr,
                                float* current_speed_kmh_ptr, float* raw_volume_l_per_h_ptr);
    float _update_main_display_value(float raw_volume_l_per_h, float current_speed_kmh, float distance_km_current_interval, float volume_L_current_interval);
    void _update_trip_and_persistence(float distance_km_current_interval, float volume_L_current_interval, float current_speed_kmh);
};

#endif // CALCULATION_ENGINE_H
