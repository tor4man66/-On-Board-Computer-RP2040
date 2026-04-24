// =====================================================================================================================
// --- Display Components (DisplayComponents.h) ---
// =====================================================================================================================
#ifndef DISPLAY_COMPONENTS_H
#define DISPLAY_COMPONENTS_H

#include "Config.h"
#include "StateManager.h"
#include "FuelMonitor.h"
#include "EngineMonitor.h"
#include "KLineManager.h"
#include "DisplayExtensions.h"
#include <Adafruit_GFX.h>

void draw_battery_icon(Adafruit_GFX* oled_obj, int16_t x, int16_t y, float voltage);
void draw_fuel_gauge(Adafruit_GFX* oled_obj, StateManager& state_manager, float fuel_level_percent,
                     int16_t x, int16_t y, int16_t total_width, int16_t block_height, uint8_t num_blocks, uint8_t block_spacing);

struct Mode0DisplayData {
    String main_value_str;
    uint8_t main_value_font_size;
    String main_unit_text;
    uint8_t main_unit_font_size_x;
    uint8_t main_unit_font_size_y;
    bool can_show_l100km;
    String pers_stat_text;
    String trip_stat_text;
    float battery_voltage;
};

struct Mode1DisplayData {
    String speed_text;
    String voltage_text;
    String rpm_text;
    String fuel_text;
    String inj_text;
};

struct Mode2DisplayData {
    String status_text;
    uint8_t fault_count;
    uint16_t fault_codes[DTC_MAX_CODES_TO_DISPLAY];
    bool is_clearing;
    bool acc_off;
};

struct ServiceModeDisplayData {
    String kline_status_text;
    String line_values[MAX_SERVICE_DISPLAY_LINES];
    String line_texts[MAX_SERVICE_DISPLAY_LINES];
};

Mode0DisplayData prepare_mode_0_data(StateManager& state_manager, FuelMonitor& fuel_monitor, EngineMonitor& engine_monitor);
Mode1DisplayData prepare_mode_1_data(StateManager& state_manager, FuelMonitor& fuel_monitor, EngineMonitor& engine_monitor);
Mode2DisplayData prepare_mode_2_data(StateManager& state_manager, KLineManager& kline_manager);
ServiceModeDisplayData prepare_service_mode_data(StateManager& state_manager);

#endif // DISPLAY_COMPONENTS_H
