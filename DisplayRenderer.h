// =====================================================================================================================
// --- Display Renderer (DisplayRenderer.h) ---
// =====================================================================================================================
#ifndef DISPLAY_RENDERER_H
#define DISPLAY_RENDERER_H

#include "Config.h"
#include "StateManager.h"
#include "HardwareManager.h"
#include "FuelMonitor.h"
#include "EngineMonitor.h"
#include "ScreenRenderers.h"
#include <Adafruit_SH110X.h>
#include "KLineManager.h"

class DisplayStateMachine;

class DisplayRenderer {
private:
    StateManager& _state;
    HardwareManager& _hardware; 
    FuelMonitor* _fuel_monitor; 
    EngineMonitor* _engine_monitor; 
    KLineManager* _kline_manager; 
    Adafruit_SH1107* _oled_driver; 

public:
    DisplayRenderer(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance,
                    FuelMonitor* fuel_monitor_ptr, EngineMonitor* engine_monitor_ptr,
                    KLineManager* kline_manager_ptr);

    void init(); 
    void render();
    
    void _draw_mode_error(Adafruit_SH1107* oled_obj, int mode);
    void _draw_main_screen_by_mode(Adafruit_SH1107* oled_obj, int mode);
    void _draw_error_screen_icon_only();
    void _draw_startup_ok_screen();
    void _draw_loop_error_screen(); 
};

#endif // DISPLAY_RENDERER_H