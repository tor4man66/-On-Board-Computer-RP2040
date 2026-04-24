// =====================================================================================================================
// --- Button Controller Implementation (ButtonController.cpp) ---
// =====================================================================================================================
#include "ButtonController.h"
#include "Config.h"
#include "DebugUtils.h"

#if KLINE_ENABLE
#include "KLineManager.h"
extern KLineManager* kline_manager_ptr;
#endif

ButtonController::ButtonController(StateManager& state_manager_instance, AudioManager& audio_manager_instance, HardwareManager& hardware_manager_instance)
: _state(state_manager_instance), _audio(audio_manager_instance), _hardware(hardware_manager_instance) {
    log_init_debug("Ініціалізація ButtonController...");
    _long_press_configs[0] = {MODE0_LONG_PRESS_ACTION, BUTTON_TRIP_RESET_HOLD_MS, BUTTON_TRIP_RESET_BEEP_FREQ, BUTTON_TRIP_RESET_BEEP_DURATION_MS};
    _long_press_configs[1] = {MODE1_LONG_PRESS_ACTION, MODE1_BUTTON_TRIP_RESET_HOLD_MS, MODE1_BUTTON_TRIP_RESET_BEEP_FREQ, MODE1_BUTTON_TRIP_RESET_BEEP_DURATION_MS};
    _long_press_configs[2] = {MODE2_LONG_PRESS_ACTION, MODE2_BUTTON_HOLD_MS, MODE2_BUTTON_BEEP_FREQ, MODE2_BUTTON_BEEP_DURATION_MS};
}

ButtonController::LongPressConfig ButtonController::_get_current_long_press_config() {
    if (_state.get_current_main_display_mode() >= 0 && _state.get_current_main_display_mode() < MAX_MAIN_DISPLAY_MODES) {
        return _long_press_configs[_state.get_current_main_display_mode()];
    }
    return _long_press_configs[0];
}

void ButtonController::_execute_long_press_action(const char* action_type, LongPressConfig& current_config) {
    if (strcmp(action_type, ACTION_RESET_TRIP_STR) == 0) {
        if (_state.get_current_main_display_mode() == 0) {
            _state.set_trip_fuel_consumed_L(0.0f);
            _state.set_trip_distance_travelled_km(0.0f);
            _audio.start_beep(current_config.button_trip_reset_beep_freq, current_config.button_trip_reset_beep_duration_ms);
            log_debug("Лічильники TRIP скинуто.");
        } else {
            log_debug_str(String("Скидання TRIP відхилено: дія '") + action_type + "' заборонена для режиму " + _state.get_current_main_display_mode() + ".");
        }
    } else if (strcmp(action_type, ACTION_CUSTOM_FUNCTION_A_STR) == 0) {
        log_debug_str(String("Виконано кастомну функцію A для режиму ") + _state.get_current_main_display_mode() + " (довге натискання).");
    } else if (strcmp(action_type, ACTION_DO_NOTHING_STR) == 0) {
        log_debug_str(String("Дія довгого натискання '") + action_type + "' (нічого не робити) для режиму " + _state.get_current_main_display_mode() + ".");
    }

    #if KLINE_ENABLE
    else if (strcmp(action_type, "clear_faults_kline") == 0) {
        log_debug("Виконано дію: Скидання помилок K-Line.");
        _state.set_temp_display_mode("DTC_CLEANING");
        if (kline_manager_ptr) {
            kline_manager_ptr->clearFaults();
        }
    }
    #endif
    else {
        log_debug_str(String("Невідома дія довгого натискання: ") + action_type + " для режиму " + _state.get_current_main_display_mode() + ".");
    }
}

void ButtonController::process_button_state(unsigned long current_time_ms) {
    if (!ENABLE_PIN_BUTTON_RESET || _hardware.get_reset_button_pin_num() == -1) {
        return;
    }

    bool is_button_down = (digitalRead(_hardware.get_reset_button_pin_num()) == LOW);
    LongPressConfig current_long_press_config = _get_current_long_press_config();

    if (is_button_down) {
        if (_state.button_press_timer_start == 0) {
            _state.button_press_timer_start = current_time_ms;
            _state.button_trip_reset_candidate = false;
            _state.button_display_mode_switch_candidate = true;
            _state.button_trip_ready_beep_played = false;
            log_debug("Кнопку натиснуто. Ініціалізовано як кандидат на коротке натискання.");
        } else {
            unsigned long hold_duration = current_time_ms - _state.button_press_timer_start;
            if (strcmp(current_long_press_config.long_press_action, ACTION_RESET_TRIP_STR) == 0 &&
                _state.get_current_main_display_mode() == 0 &&
                hold_duration >= current_long_press_config.button_trip_reset_hold_ms) {
                if (!_state.button_trip_ready_beep_played) {
                    _audio.start_beep(current_long_press_config.button_trip_reset_beep_freq, current_long_press_config.button_trip_reset_beep_duration_ms);
                    _state.button_trip_ready_beep_played = true;
                    log_debug_str(String("Кнопку утримано ") + hold_duration + " мс. Звук готовності для довгої дії: " + current_long_press_config.long_press_action + ".");
                }
                _state.button_trip_reset_candidate = true;
                _state.button_display_mode_switch_candidate = false;
            } else if (strcmp(current_long_press_config.long_press_action, ACTION_RESET_TRIP_STR) != 0 &&
                    hold_duration >= BUTTON_SHORT_PRESS_MAX_MS) { 
                    if (!_state.button_trip_ready_beep_played) {
                        _audio.start_beep(BUTTON_TRIP_RESET_BEEP_FREQ, BUTTON_TRIP_RESET_BEEP_DURATION_MS); 
                        _state.button_trip_ready_beep_played = true;
                        log_debug_str(String("Кнопку утримано ") + hold_duration + " мс. Звук готовності для довгої дії: " + current_long_press_config.long_press_action + ".");
                    }
                    _state.button_trip_reset_candidate = true;
                    _state.button_display_mode_switch_candidate = false;
            }
        }
    } else {
        if (_state.button_press_timer_start != 0) {
            log_debug("Кнопку відпущено.");
            _state.button_trip_ready_beep_played = false;
            unsigned long release_duration = current_time_ms - _state.button_press_timer_start;

            if (_state.button_trip_reset_candidate) {
                _execute_long_press_action(current_long_press_config.long_press_action, current_long_press_config);
            } else if (_state.button_display_mode_switch_candidate &&
                release_duration < BUTTON_SHORT_PRESS_MAX_MS) {
                if (_state.current_display_mode == "MAIN" || _state.current_display_mode == "NON_CRITICAL_CYCLE_ERROR") {
                    _state.set_current_main_display_mode((_state.get_current_main_display_mode() + 1) % MAX_MAIN_DISPLAY_MODES);
                    _audio.play_display_mode_switch_beep();
                    log_debug_str(String("Перемкнуто головний екран на режим: ") + _state.get_current_main_display_mode() + ".");
                } else {
                    log_debug_str(String("Перемикання режиму дисплея відхилено. Поточний стан: ") + _state.current_display_mode);
                }
            }
            _state.button_press_timer_start = 0;
            _state.button_trip_reset_candidate = false;
            _state.button_display_mode_switch_candidate = false;
        }
    }
}