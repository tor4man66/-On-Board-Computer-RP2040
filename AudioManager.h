// =====================================================================================================================
// --- Audio Manager (AudioManager.h) ---
// =====================================================================================================================
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "HardwareManager.h"

class AudioManager {
private:
    StateManager& _state;
    HardwareManager& _hardware;

public:
    AudioManager(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance);

    void start_beep(unsigned int freq_hz, unsigned long duration_ms);
    void update_beep_manager();
    void play_display_mode_switch_beep();
    void mute_speaker();
    void manage_sensor_alarm();
};

#endif // AUDIO_MANAGER_H
