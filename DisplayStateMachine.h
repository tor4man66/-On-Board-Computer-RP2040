// =====================================================================================================================
// --- Display State Machine (DisplayStateMachine.h) ---
// =====================================================================================================================
#ifndef DISPLAY_STATE_MACHINE_H
#define DISPLAY_STATE_MACHINE_H

#include "Config.h"
#include "StateManager.h"
#include "ErrorCoordinator.h"
#include "AudioManager.h"
#include "DisplayRenderer.h"
#include "Icons.h"
#include "DebugUtils.h"
#include <algorithm>
#include <cstdint>

class DisplayStateMachine {
public:
    const String STATE_STARTUP_INIT = "STARTUP_INIT";
    const String STATE_STARTUP_OK_SCREEN = "STARTUP_OK_SCREEN";
    const String STATE_STARTUP_CRITICAL_ERROR = "STARTUP_CRITICAL_ERROR";
    const String STATE_MAIN = "MAIN";
    const String STATE_NON_CRITICAL_CYCLE_ERROR = "NON_CRITICAL_CYCLE_ERROR";
    const String STATE_CRITICAL_CYCLE_ERROR = "CRITICAL_CYCLE_ERROR";
    const String STATE_LOOP_ERROR = "LOOP_ERROR";

private:
    StateManager& _state;
    ErrorCoordinator& _error_coordinator;
    AudioManager& _audio;
    DisplayRenderer& _renderer;

    bool _render_error_active = false;
    unsigned long _render_error_start_time = 0;
    String _render_error_message = "";

    ErrorIconType _queued_errors_for_next_cycle[ICON_COUNT];
    int _queued_errors_count = 0;
    bool pending_non_critical_error_display_after_startup = false;

public:
    DisplayStateMachine(StateManager& state_manager_instance, ErrorCoordinator& error_coordinator_instance,
                        AudioManager& audio_manager_instance, DisplayRenderer& display_renderer_instance);

    void init_startup_state(unsigned long current_time_ms);
    void initiate_loop_error_display(const char* exception_msg, unsigned long current_time_ms);
    void reset_render_error_state();
    void update(unsigned long current_time_ms);

private:
    void _handle_startup_init_state(unsigned long current_time_ms);
    void _handle_startup_ok_screen_state(unsigned long current_time_ms);
    void _handle_non_critical_cycle_error_state(unsigned long current_time_ms);
    void _handle_critical_error_cycling(unsigned long current_time_ms);
    void _update_active_errors_and_reset_states(ErrorIconType* new_errors_list, int new_errors_count, bool force_update_log = false);
    void _evaluate_and_transition_error_states(unsigned long current_time_ms);
};

#endif // DISPLAY_STATE_MACHINE_H
