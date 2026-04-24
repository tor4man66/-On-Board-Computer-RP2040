// =====================================================================================================================
// --- Display State Machine Implementation (DisplayStateMachine.cpp) ---
// =====================================================================================================================
#include "DisplayStateMachine.h"
#include "Config.h"
#include "DebugUtils.h"
#include <algorithm>
#include <cstdint>

DisplayStateMachine::DisplayStateMachine(StateManager& state_manager_instance, ErrorCoordinator& error_coordinator_instance,
                                         AudioManager& audio_manager_instance, DisplayRenderer& display_renderer_instance)
: _state(state_manager_instance), _error_coordinator(error_coordinator_instance),
_audio(audio_manager_instance), _renderer(display_renderer_instance) {
    log_init_debug("Ініціалізація DisplayStateMachine...");
}

void DisplayStateMachine::init_startup_state(unsigned long current_time_ms) {
    _handle_startup_init_state(current_time_ms);
}

void DisplayStateMachine::initiate_loop_error_display(const char* exception_msg, unsigned long current_time_ms) {
    String error_msg = String("Критична помилка в циклі відображення: ") + exception_msg;
    _state.error_logger->log_error_str(error_msg);
    log_debug_str(error_msg + ". Перехід до LOOP ERROR екрану.");

    _render_error_active = true;
    _render_error_start_time = current_time_ms;
    _render_error_message = exception_msg;
    _state.current_display_mode = STATE_LOOP_ERROR;
    _audio.mute_speaker();
}

void DisplayStateMachine::reset_render_error_state() {
    if (_render_error_active) {
        log_debug("Стан помилки рендерингу скинуто.");
        _render_error_active = false;
        _render_error_start_time = 0;
        _render_error_message = "";
    }
}

void DisplayStateMachine::update(unsigned long current_time_ms) {
    if (_state.acc_just_lost) {
        log_debug("DisplayStateMachine: ACC щойно вимкнувся, помилки продовжать відображатися до кінця циклу.");
        _state.acc_just_lost = false;
    }

    if (_render_error_active) {
        if ((current_time_ms - _render_error_start_time) >= LOOP_ERROR_PAUSE_MS) {
            log_debug("Час відображення помилки LOOP ERROR вичерпано. Спроба повернення до основного режиму.");
            reset_render_error_state();
            _state.current_display_mode = STATE_MAIN;
        }
        return;
    }

    if ((current_time_ms - _state.last_blink_toggle_time_ms) >= BLINK_INTERVAL_MS) {
        _state.blink_on = !_state.blink_on;
        _state.last_blink_toggle_time_ms = current_time_ms;
    }

    _evaluate_and_transition_error_states(current_time_ms);

    if (_state.current_display_mode == STATE_STARTUP_OK_SCREEN) {
        _handle_startup_ok_screen_state(current_time_ms);
    } else if (_state.current_display_mode == STATE_NON_CRITICAL_CYCLE_ERROR) {
        _handle_non_critical_cycle_error_state(current_time_ms);
    } else if (_state.current_display_mode == STATE_CRITICAL_CYCLE_ERROR) {
        _handle_critical_error_cycling(current_time_ms);
    }
}

void DisplayStateMachine::_handle_startup_init_state(unsigned long current_time_ms) {
    ErrorIconType initial_full_errors[ICON_COUNT];
    int initial_full_errors_count = 0;
    _error_coordinator.check_for_errors(current_time_ms, initial_full_errors, &initial_full_errors_count);

    ErrorIconType initial_critical_errors[ICON_COUNT];
    int initial_critical_errors_count = 0;
    for (int i = 0; i < initial_full_errors_count; ++i) {
        if (GET_ERROR_ICON(initial_full_errors[i]).is_critical) {
            initial_critical_errors[initial_critical_errors_count++] = initial_full_errors[i];
        }
    }

    bool has_non_critical_errors_at_startup = false;
    for (int i = 0; i < initial_full_errors_count; ++i) {
        if (GET_ERROR_ICON(initial_full_errors[i]).severity == ERROR_SEVERITY_NON_CRITICAL) {
            has_non_critical_errors_at_startup = true;
            break;
        }
    }

    if (initial_critical_errors_count == 0) {
        _state.current_display_mode = STATE_STARTUP_OK_SCREEN;
        _state.startup_phase_start_ms = current_time_ms;
        pending_non_critical_error_display_after_startup = has_non_critical_errors_at_startup;

        if (has_non_critical_errors_at_startup) {
            ErrorIconType non_critical_errors[ICON_COUNT];
            int non_critical_errors_count = 0;
            for (int i = 0; i < initial_full_errors_count; ++i) {
                if (GET_ERROR_ICON(initial_full_errors[i]).severity == ERROR_SEVERITY_NON_CRITICAL) {
                    non_critical_errors[non_critical_errors_count++] = initial_full_errors[i];
                }
            }
            _update_active_errors_and_reset_states(non_critical_errors, non_critical_errors_count, true);
        } else {
            _update_active_errors_and_reset_states((ErrorIconType[]){ICON_NONE}, 1, true);
        }
        log_init_debug("Виявлено відсутність критичних помилок. Перехід до 'STATUS OK' екрану.");
    } else {
        _state.current_display_mode = STATE_STARTUP_CRITICAL_ERROR;
        _update_active_errors_and_reset_states(initial_critical_errors, initial_critical_errors_count, true);
        _state.sensor_alarm_active = true;
        _state.alarm_phase = 0;
        _state.alarm_phase_start_time_ms = current_time_ms;
        log_init_debug("Виявлено критичні помилки при запуску. Перехід до циклу критичних помилок та активація сирени.");
    }
}

void DisplayStateMachine::_handle_startup_ok_screen_state(unsigned long current_time_ms) {
    if ((current_time_ms - _state.startup_phase_start_ms) >= STARTUP_OK_SCREEN_DURATION_MS) {
        if (pending_non_critical_error_display_after_startup) {
            _state.current_display_mode = STATE_NON_CRITICAL_CYCLE_ERROR;
            _state.non_critical_cycle_phase = 0;
            _state.non_critical_last_state_change_time_ms = current_time_ms;
            log_debug("Завершено 'STATUS OK' екран, перехід до 'NON CRITICAL CYCLE_ERROR' (були виявлені некритичні помилки при запуску).");
        } else {
            _state.active_errors[0] = ICON_NONE;
            _state.active_errors_count = 1;
            _state.current_display_mode = STATE_MAIN;
            log_debug("Завершено 'STATUS OK' екран, перехід до 'MAIN' екрану.");
        }
    }
}

void DisplayStateMachine::_handle_non_critical_cycle_error_state(unsigned long current_time_ms) {
    if (_state.active_errors_count == 0 || (_state.active_errors_count == 1 && _state.active_errors[0] == ICON_NONE)) {
        _state.current_display_mode = STATE_MAIN;
        log_debug("Некритичні помилки зникли, перехід до MAIN екрану.");
        return;
    }

    if (_state.non_critical_cycle_phase == 0) {
        if ((current_time_ms - _state.non_critical_last_state_change_time_ms) >= NON_CRITICAL_ERROR_ICON_DURATION_MS) {
            _state.non_critical_cycle_phase = 1;
            _state.non_critical_last_state_change_time_ms = current_time_ms;
            log_debug("NON_CRITICAL: перехід від іконки до основного екрану.");
        }
    } else if (_state.non_critical_cycle_phase == 1) {
        if ((current_time_ms - _state.non_critical_last_state_change_time_ms) >= NON_CRITICAL_ERROR_MAIN_SCREEN_DURATION_MS) {
            _state.non_critical_cycle_phase = 0;
            _state.non_critical_last_state_change_time_ms = current_time_ms;
            _state.current_error_display_index = (_state.current_error_display_index + 1) % _state.active_errors_count;
            _state.last_error_cycle_time_ms = current_time_ms;
            String current_error_text = GET_ERROR_ICON(_state.active_errors[_state.current_error_display_index]).text;
            log_debug_str(String("NON CRITICAL: повний цикл відображення помилки завершено, наступна: ") + current_error_text + ".");
        }
    }
}

void DisplayStateMachine::_handle_critical_error_cycling(unsigned long current_time_ms) {
    if (_state.active_errors_count == 0 || (_state.active_errors_count == 1 && _state.active_errors[0] == ICON_NONE) ||
        _error_coordinator.get_error_severity_level(_state.active_errors, _state.active_errors_count) != CRITICAL_ERROR_SEVERITY) {
        _state.current_display_mode = STATE_MAIN;
    _state.sensor_alarm_active = false;
    _audio.mute_speaker();
    log_debug("Критичні помилки зникли, перехід до MAIN екрану.");
    return;
        }

        if ((current_time_ms - _state.last_error_cycle_time_ms) >= ERROR_DISPLAY_CYCLE_MS) {
            _state.current_error_display_index = (_state.current_error_display_index + 1) % _state.active_errors_count;
            _state.last_error_cycle_time_ms = current_time_ms;
            String current_error_text = GET_ERROR_ICON(_state.active_errors[_state.current_error_display_index]).text;
            log_debug_str(String("Перемикання до наступної критичної помилки в циклі: ") + current_error_text);
        }
}

void DisplayStateMachine::_update_active_errors_and_reset_states(ErrorIconType* new_errors_list, int new_errors_count, bool force_update_log) {
    ErrorIconType temp_final_errors[ICON_COUNT];
    int temp_final_errors_count = 0;

    ErrorIconType critical_from_new[ICON_COUNT];
    int critical_from_new_count = 0;
    ErrorIconType non_critical_from_new[ICON_COUNT];
    int non_critical_from_new_count = 0;

    for (int i = 0; i < new_errors_count; ++i) {
        if (GET_ERROR_ICON(new_errors_list[i]).is_critical) {
            critical_from_new[critical_from_new_count++] = new_errors_list[i];
        } else if (GET_ERROR_ICON(new_errors_list[i]).severity == ERROR_SEVERITY_NON_CRITICAL && new_errors_list[i] != ICON_NONE) {
            non_critical_from_new[non_critical_from_new_count++] = new_errors_list[i];
        }
    }

    if (critical_from_new_count > 0) {
        if (GET_ERROR_ICON(ICON_WARNING).is_critical) {
            temp_final_errors[temp_final_errors_count++] = ICON_WARNING;
        }
        for (int i = 0; i < critical_from_new_count; ++i) {
            temp_final_errors[temp_final_errors_count++] = critical_from_new[i];
        }
    } else if (non_critical_from_new_count > 0) {
        for (int i = 0; i < non_critical_from_new_count; ++i) {
            temp_final_errors[temp_final_errors_count++] = non_critical_from_new[i];
        }
    } else {
        temp_final_errors[temp_final_errors_count++] = ICON_NONE;
    }

    bool errors_changed = false;
    if (temp_final_errors_count != _state.active_errors_count) {
        errors_changed = true;
    } else {
        ErrorIconType current_sorted[ICON_COUNT];
        memcpy(current_sorted, _state.active_errors, _state.active_errors_count * sizeof(ErrorIconType));
        std::sort(current_sorted, current_sorted + _state.active_errors_count);

        ErrorIconType new_sorted[ICON_COUNT];
        memcpy(new_sorted, temp_final_errors, temp_final_errors_count * sizeof(ErrorIconType));
        std::sort(new_sorted, new_sorted + temp_final_errors_count);

        for (int i = 0; i < temp_final_errors_count; ++i) {
            if (current_sorted[i] != new_sorted[i]) {
                errors_changed = true;
                break;
            }
        }
    }

    if (force_update_log || errors_changed) {
        String debug_msg = "Оновлення активних помилок: З [";
        for(int i = 0; i < _state.active_errors_count; ++i) debug_msg += String(GET_ERROR_ICON(_state.active_errors[i]).text) + (i == _state.active_errors_count - 1 ? "" : ", ");
        debug_msg += "] на [";
        for(int i = 0; i < temp_final_errors_count; ++i) debug_msg += String(GET_ERROR_ICON(temp_final_errors[i]).text) + (i == temp_final_errors_count - 1 ? "" : ", ");
        debug_msg += "]";
        log_debug_str(debug_msg);

        memcpy(_state.active_errors, temp_final_errors, temp_final_errors_count * sizeof(ErrorIconType));
        _state.active_errors_count = temp_final_errors_count;
        _state.current_error_display_index = 0;
        _state.last_error_cycle_time_ms = _state.current_time_ms;
        _state.non_critical_cycle_phase = 0;
        _state.non_critical_last_state_change_time_ms = _state.current_time_ms;

        uint8_t new_alarm_active_severity = _error_coordinator.get_error_severity_level(_state.active_errors, _state.active_errors_count);
        _state.sensor_alarm_active = (new_alarm_active_severity == CRITICAL_ERROR_SEVERITY);

        if (_state.sensor_alarm_active) {
            _state.alarm_phase = 0;
            _state.alarm_phase_start_time_ms = _state.current_time_ms;
            _audio.manage_sensor_alarm();
            log_debug("Сирена активована через критичні помилки.");
        } else {
            _audio.mute_speaker();
            log_debug("Сирена деактивована.");
        }
    }
    _queued_errors_count = 0;
}

void DisplayStateMachine::_evaluate_and_transition_error_states(unsigned long current_time_ms) {
    if (_state.current_display_mode == STATE_LOOP_ERROR || _render_error_active || _state.current_display_mode.startsWith("STARTUP_")) {
        if (_state.current_display_mode != STATE_STARTUP_CRITICAL_ERROR) {
            return;
        }
    }

    ErrorIconType new_ideal_active_errors_list[ICON_COUNT];
    int new_ideal_active_errors_count = 0;
    _error_coordinator.check_for_errors(current_time_ms, new_ideal_active_errors_list, &new_ideal_active_errors_count);

    uint8_t new_ideal_severity = _error_coordinator.get_error_severity_level(new_ideal_active_errors_list, new_ideal_active_errors_count);
    uint8_t current_active_severity = _error_coordinator.get_error_severity_level(_state.active_errors, _state.active_errors_count);

    if (_state.acc_loss_initiated && new_ideal_active_errors_count == 1 && new_ideal_active_errors_list[0] == ICON_NONE) {
        if (!_state.display_ready_for_dormant) {
            _state.display_ready_for_dormant = true;
            log_debug("DisplayStateMachine: Всі помилки зникли після втрати ACC, дисплей готовий до Dormant.");
        }
    } else {
        if (_state.display_ready_for_dormant) {
            _state.display_ready_for_dormant = false;
            log_debug("DisplayStateMachine: Дисплей більше не готовий до Dormant (ACC повернувся або з'явились помилки).");
        }
    }

    bool should_switch_immediately = (new_ideal_severity > current_active_severity) ||
    (current_active_severity == ERROR_SEVERITY_NONE && new_ideal_severity > ERROR_SEVERITY_NONE);

    if (should_switch_immediately) {
        _update_active_errors_and_reset_states(new_ideal_active_errors_list, new_ideal_active_errors_count);
        log_debug_str(String("Негайний перехід на помилки через підвищення критичності: ") + new_ideal_severity + " > " + current_active_severity);

        if (new_ideal_severity == CRITICAL_ERROR_SEVERITY) {
            _state.current_display_mode = STATE_CRITICAL_CYCLE_ERROR;
        } else if (new_ideal_severity == ERROR_SEVERITY_NON_CRITICAL) {
            _state.current_display_mode = STATE_NON_CRITICAL_CYCLE_ERROR;
            _state.non_critical_cycle_phase = 0;
            _state.non_critical_last_state_change_time_ms = current_time_ms;
        } else if (new_ideal_severity == ERROR_SEVERITY_NONE) {
            _state.current_display_mode = STATE_MAIN;
        }
        log_debug_str(String("Перехід в режим: ") + _state.current_display_mode);
        return;
    }

    bool new_errors_different_from_current = false;
    if (new_ideal_active_errors_count != _state.active_errors_count) {
        new_errors_different_from_current = true;
    } else {
        ErrorIconType temp_current_sorted[ICON_COUNT];
        memcpy(temp_current_sorted, _state.active_errors, _state.active_errors_count * sizeof(ErrorIconType));
        std::sort(temp_current_sorted, temp_current_sorted + _state.active_errors_count);

        ErrorIconType temp_new_ideal_sorted[ICON_COUNT];
        memcpy(temp_new_ideal_sorted, new_ideal_active_errors_list, new_ideal_active_errors_count * sizeof(ErrorIconType));
        std::sort(temp_new_ideal_sorted, temp_new_ideal_sorted + new_ideal_active_errors_count);

        for (int i = 0; i < new_ideal_active_errors_count; ++i) {
            if (temp_current_sorted[i] != temp_new_ideal_sorted[i]) {
                new_errors_different_from_current = true;
                break;
            }
        }
    }

    if (new_errors_different_from_current) {
        bool new_errors_different_from_queued = false;
        if (new_ideal_active_errors_count != _queued_errors_count) {
            new_errors_different_from_queued = true;
        } else {
            ErrorIconType temp_queued_sorted[ICON_COUNT];
            memcpy(temp_queued_sorted, _queued_errors_for_next_cycle, _queued_errors_count * sizeof(ErrorIconType));
            std::sort(temp_queued_sorted, temp_queued_sorted + _queued_errors_count);

            ErrorIconType temp_new_ideal_sorted[ICON_COUNT];
            memcpy(temp_new_ideal_sorted, new_ideal_active_errors_list, new_ideal_active_errors_count * sizeof(ErrorIconType));
            std::sort(temp_new_ideal_sorted, temp_new_ideal_sorted + new_ideal_active_errors_count);

            for (int i = 0; i < _queued_errors_count; ++i) {
                if (temp_queued_sorted[i] != temp_new_ideal_sorted[i]) {
                    new_errors_different_from_queued = true;
                    break;
                }
            }
        }

        if (new_errors_different_from_queued) {
            memcpy(_queued_errors_for_next_cycle, new_ideal_active_errors_list, new_ideal_active_errors_count * sizeof(ErrorIconType));
            _queued_errors_count = new_ideal_active_errors_count;
            String debug_msg = "Помилки поміщено в чергу для відкладеного переходу: [";
            for(int i = 0; i < _queued_errors_count; ++i) debug_msg += String(GET_ERROR_ICON(_queued_errors_for_next_cycle[i]).text) + (i == _queued_errors_count - 1 ? "" : ", ");
            debug_msg += "]";
            log_debug_str(debug_msg);
        }
    }

    if (_queued_errors_count > 0) {
        bool is_current_cycle_complete = false;
        if (current_active_severity == ERROR_SEVERITY_NON_CRITICAL) {
            if (_state.non_critical_cycle_phase == 1 &&
                (current_time_ms - _state.non_critical_last_state_change_time_ms) >= NON_CRITICAL_ERROR_MAIN_SCREEN_DURATION_MS) {
                is_current_cycle_complete = true;
                }
        } else if (current_active_severity == CRITICAL_ERROR_SEVERITY) {
            if ((current_time_ms - _state.last_error_cycle_time_ms) >= ERROR_DISPLAY_CYCLE_MS) {
                if (_state.current_error_display_index >= _state.active_errors_count - 1) {
                    is_current_cycle_complete = true;
                }
            }
        } else if (current_active_severity == ERROR_SEVERITY_NONE) {
            is_current_cycle_complete = true;
        }

        if (is_current_cycle_complete) {
            _update_active_errors_and_reset_states(_queued_errors_for_next_cycle, _queued_errors_count);
            log_debug("Черга помилок застосована після завершення поточного циклу.");

            if (new_ideal_severity == CRITICAL_ERROR_SEVERITY) {
                _state.current_display_mode = STATE_CRITICAL_CYCLE_ERROR;
            } else if (new_ideal_severity == ERROR_SEVERITY_NON_CRITICAL) {
                _state.current_display_mode = STATE_NON_CRITICAL_CYCLE_ERROR;
                _state.non_critical_cycle_phase = 0;
                _state.non_critical_last_state_change_time_ms = current_time_ms;
            } else if (new_ideal_severity == ERROR_SEVERITY_NONE) {
                _state.current_display_mode = STATE_MAIN;
            }
            log_debug_str(String("Перехід в режим після черги: ") + _state.current_display_mode);
            _queued_errors_count = 0;
        }
    } else if (_queued_errors_count == 0) {
        if (new_ideal_severity == ERROR_SEVERITY_NONE) {
            if (_state.current_display_mode != STATE_MAIN) {
                _state.current_display_mode = STATE_MAIN;
                log_debug("Перехід до 'MAIN' (помилок немає).");
            }
        } else if (new_ideal_severity == ERROR_SEVERITY_NON_CRITICAL) {
            if (_state.current_display_mode != STATE_NON_CRITICAL_CYCLE_ERROR) {
                _state.current_display_mode = STATE_NON_CRITICAL_CYCLE_ERROR;
                _state.non_critical_cycle_phase = 0;
                _state.non_critical_last_state_change_time_ms = current_time_ms;
                log_debug("Перехід до 'NON CRITICAL CYCLE_ERROR'.");
            }
        } else if (new_ideal_severity == CRITICAL_ERROR_SEVERITY) {
            if (_state.current_display_mode != STATE_CRITICAL_CYCLE_ERROR && _state.current_display_mode != STATE_STARTUP_CRITICAL_ERROR) {
                _state.current_display_mode = STATE_CRITICAL_CYCLE_ERROR;
                log_debug("Перехід до 'CRITICAL CYCLE_ERROR'.");
            }
        }
    }
}
