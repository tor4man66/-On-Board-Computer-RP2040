// =====================================================================================================================
// --- Error Coordinator (ErrorCoordinator.h) ---
// =====================================================================================================================
#ifndef ERROR_COORDINATOR_H
#define ERROR_COORDINATOR_H

#include <cstdint>
#include "Config.h"
#include "Icons.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "HardwareManager.h"
#include "FuelMonitor.h"
#include "EngineMonitor.h"
#include "SensorFunctions.h"

class ErrorCoordinator {
private:
    StateManager& _state;
    HardwareManager& _hardware;
    FuelMonitor& _fuel_monitor;
    EngineMonitor& _engine_monitor;

public:
    ErrorCoordinator(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance,
                     FuelMonitor& fuel_monitor_instance, EngineMonitor& engine_monitor_instance);

    uint8_t get_error_severity_level(ErrorIconType* error_list, int count);
    void check_for_errors(unsigned long current_time_ms, ErrorIconType* found_errors, int* found_count_ptr);
};

#endif // ERROR_COORDINATOR_H
