// =====================================================================================================================
// --- Audio Manager Implementation (AudioManager.cpp) ---
// =====================================================================================================================
#include "AudioManager.h"
#include "Config.h"
#include "DebugUtils.h"

AudioManager::AudioManager(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance)
: _state(state_manager_instance), _hardware(hardware_manager_instance) {
    log_init_debug("Ініціалізація AudioManager...");
}

void AudioManager::start_beep(unsigned int freq_hz, unsigned long duration_ms) {
    if (!ENABLE_SPEAKER || _hardware.get_speaker_pin_num() == -1) return;

    if (!_state.beep_active) {
        _state.beep_active = true;
        _state.beep_start_ms = _state.current_time_ms;
        _state.beep_duration_ms = duration_ms;
        _state.beep_freq = freq_hz;
        _hardware.set_speaker_freq_duty(freq_hz, 32768);
        log_debug_str(String("Запущено біп: ") + freq_hz + " Гц, " + duration_ms + " мс.");
    }
}

void AudioManager::update_beep_manager() {
    if (_state.beep_active) {
        unsigned long elapsed_ms = _state.current_time_ms - _state.beep_start_ms;
        if (elapsed_ms >= _state.beep_duration_ms) {
            _hardware.set_speaker_freq_duty(0, 0);
            _state.beep_active = false;
            log_debug("Біп завершено.");
        }
    }
    
    manage_sensor_alarm(); 
}

void AudioManager::play_display_mode_switch_beep() {
    start_beep(BUTTON_DISPLAY_MODE_SWITCH_BEEP_FREQ, BUTTON_DISPLAY_MODE_SWITCH_BEEP_DURATION_MS);
}

void AudioManager::mute_speaker() {
    if (ENABLE_SPEAKER && _hardware.get_speaker_pin_num() != -1) {
        _hardware.set_speaker_freq_duty(0, 0);
        _state.beep_active = false;
        _state.sensor_alarm_active = false;
        log_debug("Динамік вимкнено.");
    }
}

void AudioManager::manage_sensor_alarm() {
    if (!ENABLE_SPEAKER || _hardware.get_speaker_pin_num() == -1) {
        return;
    }
    if (!_state.sensor_alarm_active) {
        if (_state.current_speaker_duty != 0) {
            _hardware.set_speaker_freq_duty(0, 0);
        }
        _state.alarm_phase = 0;
        _state.alarm_phase_start_time_ms = 0;
        return;
    }

    if (_state.beep_active) {
        return;
    }

    unsigned long current_time_ms = _state.current_time_ms;
    if (_state.alarm_phase_start_time_ms == 0) {
        _state.alarm_phase_start_time_ms = current_time_ms;
        _hardware.set_speaker_freq_duty(ALARM_SEQUENCE[0].frequency_hz, 32768);
        return;
    }

    unsigned int current_alarm_sequence_index = _state.alarm_phase % ALARM_SEQUENCE_SIZE;
    AlarmPhase current_phase = ALARM_SEQUENCE[current_alarm_sequence_index];

    if ((current_time_ms - _state.alarm_phase_start_time_ms) >= current_phase.duration_ms) {
        _state.alarm_phase = (_state.alarm_phase + 1) % ALARM_SEQUENCE_SIZE;
        _state.alarm_phase_start_time_ms = current_time_ms;
        AlarmPhase next_phase = ALARM_SEQUENCE[_state.alarm_phase];

        if (DEBUG_ENABLED) {
            log_debug_str(String("Сирена: перехід до фази ") + _state.alarm_phase + ". Наступна: Тривалість: " + next_phase.duration_ms + ", Частота: " + next_phase.frequency_hz + " Гц.");
        }
        if (next_phase.frequency_hz > 0) {
            _hardware.set_speaker_freq_duty(next_phase.frequency_hz, 32768);
        } else {
            _hardware.set_speaker_freq_duty(0, 0);
        }
    } else if (current_phase.frequency_hz > 0) {
        if (_state.current_speaker_freq != current_phase.frequency_hz) {
            _hardware.set_speaker_freq_duty(current_phase.frequency_hz, 32768);
        }
    } else {
        if (_state.current_speaker_duty != 0) {
            _hardware.set_speaker_freq_duty(0, 0);
        }
    }
}
