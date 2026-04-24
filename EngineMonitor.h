// =====================================================================================================================
// --- Engine Monitor (EngineMonitor.h) ---
// =====================================================================================================================
#ifndef ENGINE_MONITOR_H
#define ENGINE_MONITOR_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "HardwareManager.h"

class EngineMonitor {
private:
    StateManager& _state;
    HardwareManager& _hardware;
    unsigned long _last_log_time_ms = 0;

public:
    EngineMonitor(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance);

    void update(float interval_sec, int last_inj_raw_duration, int last_ignition_period, unsigned long last_vss_activity_ms);
    void _check_engine_status(unsigned long current_time_ms);
    void _update_rpm_from_raw_data(int period_between_pulses_us);
    void _update_injector_pulse_data(int raw_duration);

    int get_current_inj_period();
    float get_current_rpm();
};

#endif // ENGINE_MONITOR_H
