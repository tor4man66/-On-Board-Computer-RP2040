// =====================================================================================================================
// --- Brightness Monitor (BrightnessMonitor.h) ---
// =====================================================================================================================
#ifndef BRIGHTNESS_MONITOR_H
#define BRIGHTNESS_MONITOR_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "HardwareManager.h"
#include <Adafruit_SH110X.h>

class BrightnessMonitor {
private:
    StateManager& _state;
    HardwareManager& _hardware;
    int _last_logged_contrast = -1;

public:
    BrightnessMonitor(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance);

    void update(unsigned long current_time_ms);
};

#endif // BRIGHTNESS_MONITOR_H
