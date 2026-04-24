// =====================================================================================================================
// --- Button Controller (ButtonController.h) ---
// =====================================================================================================================
#ifndef BUTTON_CONTROLLER_H
#define BUTTON_CONTROLLER_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "AudioManager.h"
#include "HardwareManager.h"

class ButtonController {
private:
    StateManager& _state;
    AudioManager& _audio;
    HardwareManager& _hardware;

    struct LongPressConfig {
        const char* long_press_action;
        unsigned long button_trip_reset_hold_ms;
        unsigned int button_trip_reset_beep_freq;
        unsigned long button_trip_reset_beep_duration_ms;
    };

    LongPressConfig _long_press_configs[MAX_MAIN_DISPLAY_MODES];

public:
    ButtonController(StateManager& state_manager_instance, AudioManager& audio_manager_instance, HardwareManager& hardware_manager_instance);

    LongPressConfig _get_current_long_press_config();
    void _execute_long_press_action(const char* action_type, LongPressConfig& current_config);
    void process_button_state(unsigned long current_time_ms);
};

#endif // BUTTON_CONTROLLER_H
