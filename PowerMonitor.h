// =====================================================================================================================
// --- Power Monitor (PowerMonitor.h) ---
// =====================================================================================================================
#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "HardwareManager.h"

class PowerMonitor {
private:
    StateManager& _state;
    HardwareManager& _hardware;

public:
    PowerMonitor(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance);

    float _get_raw_voltage_value();
    void update(float interval_sec);
};

#endif // POWER_MONITOR_H
