// =====================================================================================================================
// --- Display Renderer Implementation (DisplayRenderer.cpp) ---
// =====================================================================================================================
#include "DisplayRenderer.h"
#include "Config.h"
#include "DebugUtils.h"
#include "ScreenRenderers.h"
#include "DisplayComponents.h"  

DisplayRenderer::DisplayRenderer(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance,
                                 FuelMonitor* fuel_monitor_ptr, EngineMonitor* engine_monitor_ptr,
                                 KLineManager* kline_manager_ptr)
: _state(state_manager_instance),
_hardware(hardware_manager_instance),
_fuel_monitor(fuel_monitor_ptr),
_engine_monitor(engine_monitor_ptr),
_kline_manager(kline_manager_ptr),
_oled_driver(nullptr)
{
    log_init_debug("Ініціалізація DisplayRenderer.");
}

void DisplayRenderer::init() {
    if (_state.oled_driver_instance != nullptr) {
        _oled_driver = static_cast<Adafruit_SH1107*>(_state.oled_driver_instance);
        _oled_driver->clearDisplay();
        _oled_driver->display();
        _oled_driver->setTextWrap(false);
        _oled_driver->cp437(true);
        log_init_debug("DisplayRenderer: OLED драйвер встановлено.");
    } else {
        log_init_debug("DisplayRenderer: OLED драйвер не ініціалізовано в StateManager.");
    }
}

void DisplayRenderer::render() {
    if (_state.oled_status != "OK" || _oled_driver == nullptr || !ENABLE_OLED) {
        return;
    }
    
    _oled_driver->clearDisplay();
    _oled_driver->setTextColor(SH110X_WHITE);
    _oled_driver->setCursor(0, 0);
    _oled_driver->setFont(NULL);
    
    _state.blink_on = ((millis() / BLINK_INTERVAL_MS) % 2 == 0);

    if (_state.is_service_mode) {
        ServiceModeDisplayData data = prepare_service_mode_data(_state);
        render_service_mode_screen(_oled_driver, data);
        _oled_driver->display();
        return;
    }

    if (_state.current_display_mode == "STARTUP_OK_SCREEN") {
        _draw_startup_ok_screen();
    } else if (_state.current_display_mode == "STARTUP_CRITICAL_ERROR") {
        _draw_error_screen_icon_only();
    } else if (_state.current_display_mode == "NON_CRITICAL_CYCLE_ERROR") {
        if (_state.active_errors_count == 0 || (_state.active_errors_count == 1 && _state.active_errors[0] == ICON_NONE)) {
            _draw_main_screen_by_mode(_oled_driver, _state.get_current_main_display_mode());
        } else if (_state.non_critical_cycle_phase == 1) {
            _draw_main_screen_by_mode(_oled_driver, _state.get_current_main_display_mode());
        } else {
            _draw_error_screen_icon_only();
        }
    } else if (_state.current_display_mode == "CRITICAL_CYCLE_ERROR") {
        _draw_error_screen_icon_only();
    } else if (_state.current_display_mode == "MAIN") {
        _draw_main_screen_by_mode(_oled_driver, _state.get_current_main_display_mode());
    } else if (_state.current_display_mode == "LOOP_ERROR") {
        _draw_loop_error_screen();
    }

    _oled_driver->display();
}

void DisplayRenderer::_draw_main_screen_by_mode(Adafruit_SH1107* oled_obj, int mode) {
    if (!oled_obj) return;

    switch (mode) {
        case 0:
            if (_fuel_monitor && _engine_monitor) {
                render_mode_0_screen(oled_obj, prepare_mode_0_data(_state, *_fuel_monitor, *_engine_monitor));
                return;
            }
            break;

        case 1:
            if (_fuel_monitor && _engine_monitor) {
                render_mode_1_screen(oled_obj, prepare_mode_1_data(_state, *_fuel_monitor, *_engine_monitor));
                return;
            }
            break;

        case 2:
            if (_kline_manager) {
                render_mode_2_screen(oled_obj, prepare_mode_2_data(_state, *_kline_manager));
                return;
            }
            break;

        default:
            if (_fuel_monitor && _engine_monitor) {
                render_mode_0_screen(oled_obj, prepare_mode_0_data(_state, *_fuel_monitor, *_engine_monitor));
                return;
            }
            break;
    }

    _draw_mode_error(oled_obj, mode);
}

void DisplayRenderer::_draw_mode_error(Adafruit_SH1107* oled_obj, int mode) {
    const char* error_text;
    switch (mode) {
        case 0:  error_text = MODE_ERROR_TEXT_MODE_0;  break;
        case 1:  error_text = MODE_ERROR_TEXT_MODE_1;  break;
        case 2:  error_text = MODE_ERROR_TEXT_MODE_2;  break;
        default: error_text = MODE_ERROR_TEXT_DEFAULT;  break;
    }
    
    oled_obj->setFont(MODE_ERROR_TEXT_FONT);
    oled_obj->setCursor(MODE_ERROR_TEXT_X_POS, MODE_ERROR_TEXT_Y_POS);
    oled_obj->setTextSize(MODE_ERROR_TEXT_SIZE);
    oled_obj->print(error_text);
    oled_obj->setFont(NULL);
}

void DisplayRenderer::_draw_error_screen_icon_only() {
    if (!_oled_driver || _state.active_errors_count == 0) return;
    Adafruit_SH1107* oled_obj = _oled_driver;

    if (_state.current_error_display_index >= _state.active_errors_count) {
        _state.current_error_display_index = 0;
    }

    ErrorIconType error_type_to_display = _state.active_errors[_state.current_error_display_index];
    const ErrorIconData& error_data = GET_ERROR_ICON(error_type_to_display);

    if (error_data.blink && !_state.blink_on) {
        return;
    }

    const unsigned char* bits_to_draw = error_data.icon_data;

    if (error_data.is_animated && error_data.animation_frames != nullptr) {
        static uint32_t last_frame_ms = 0;
        static uint8_t frame_idx = 0;

        if (_state.current_time_ms - last_frame_ms >= ERROR_ANIMATION_SPEED) {
            frame_idx++;
            if (frame_idx >= error_data.frame_count) {
                frame_idx = 0;
            }
            last_frame_ms = _state.current_time_ms;
        }
        bits_to_draw = error_data.animation_frames[frame_idx];
    }

    if (bits_to_draw != icon_empty_bits) {
        oled_obj->drawBitmap(error_data.icon_pos_x, error_data.icon_pos_y,
                             bits_to_draw, error_data.width, error_data.height, 1);
    }
}

void DisplayRenderer::_draw_startup_ok_screen() {
    if (!_oled_driver) return;
    Adafruit_SH1107* oled_obj = _oled_driver;
    const ErrorIconData& ok_data = GET_ERROR_ICON(ICON_STATUS_OK);

    if (ok_data.icon_data != icon_empty_bits) {
        oled_obj->drawBitmap(ok_data.icon_pos_x, ok_data.icon_pos_y,
                             ok_data.icon_data, ok_data.width, ok_data.height, 1);
    }
}

void DisplayRenderer::_draw_loop_error_screen() {
    if (!_oled_driver) return;
    Adafruit_SH1107* oled_obj = _oled_driver;
    oled_obj->setTextSize(LOOP_ERROR_TEXT_SIZE);
    oled_obj->setTextColor(SH110X_WHITE);
    oled_obj->setCursor(LOOP_ERROR_X_POS, LOOP_ERROR_Y_POS);
    oled_obj->println(LOOP_ERROR_TEXT);
    oled_obj->setCursor(0, 0);
    oled_obj->setTextSize(1);
    oled_obj->print(_state.critical_error_message.c_str());
}