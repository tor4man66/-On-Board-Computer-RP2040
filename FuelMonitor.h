// =====================================================================================================================
// --- Fuel Monitor (FuelMonitor.h) ---
// =====================================================================================================================
#ifndef FUEL_MONITOR_H
#define FUEL_MONITOR_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "HardwareManager.h"

class FuelMonitor {
private:
    StateManager& _state;
    HardwareManager& _hardware;
    float _fuel_buffer[FUEL_BUFFER_SIZE];
    int _fuel_buffer_idx = 0;

public:
    FuelMonitor(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance);

    void initialize_fuel_buffer(float initial_value);
    float get_raw_fuel_percent();
    void process_fuel_smoothing();
    float get_smoothed_fuel_percent();
    void update(float interval_sec);
};

#endif // FUEL_MONITOR_H
