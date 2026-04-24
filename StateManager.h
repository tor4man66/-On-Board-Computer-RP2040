// =====================================================================================================================
// --- State Manager(StateManager.h) ---
// =====================================================================================================================
#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include <cstdint>
#include <Adafruit_GFX.h>
#include "Config.h"
#include "DebugUtils.h"
#include "Icons.h"
#include "KLineTypes.h"

class StateManager {
private:
    StateManager() {
        init_state();
    }

    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;

    String _temp_display_mode = "";
    unsigned long _temp_display_mode_start_time = 0;

public:   

    static StateManager& getInstance();

    void set_temp_display_mode(String mode);
    String get_and_reset_temp_display_mode();

    ErrorLogger* error_logger;
    bool is_critical_error = false;         
    String critical_error_message;       

    bool acc_just_lost = false;
    bool acc_loss_initiated = false;
    bool display_ready_for_dormant = false;
    bool acc_present = true;
    unsigned long acc_lost_time_ms = 0;
    bool is_dormant_mode = false;

    float current_battery_voltage = 14.0f;
    int dynamic_dead_time_us = INJ_DEAD_TIME_US;
    unsigned long _last_fuel_update_time_ms = 0;
    unsigned long _last_voltage_update_time_ms = 0;

    float rpm = 0.0f;
    int current_inj_period_us = 0;
    float avg_inj_ms = 0.0f;
    float avg_rpm_for_display = 0.0f;
    bool is_engine_running = false;
    unsigned long last_inj_activity_time_ms = 0;
    unsigned long last_vss_activity_time_ms = 0;
    unsigned long engine_start_time_ms = 0;
    bool is_engine_running_stable = false;
    unsigned long last_stable_rpm_time_ms = 0;
    bool is_towing_mode_active = false;

    float _trip_fuel_consumed_L = 0.0f;
    float _trip_distance_travelled_km = 0.0f;
    float _persistent_trip_fuel_L = 0.0f;
    float _persistent_trip_distance_km = 0.0f;
    unsigned long last_persistent_save_time_ms = 0;
    bool persistent_data_dirty = false;

    unsigned long kline_last_update_time_ms = 0;
    unsigned long kline_last_connect_attempt_time_ms = 0;

    volatile KLineConnectionState kline_connection_status = KLINE_DISCONNECTED;

    char kline_error_message[64];

    String current_display_mode = "STARTUP_INIT";
    int _current_main_display_mode = DEFAULT_MAIN_DISPLAY_MODE;
    unsigned long last_main_display_mode_save_time_ms = 0;
    String oled_status = "OFF";
    Adafruit_GFX* oled_driver_instance = nullptr;
    uint8_t current_oled_contrast = OLED_CONTRAST_MAX;
    unsigned long last_brightness_read_ms = 0;
    uint8_t last_applied_contrast = OLED_CONTRAST_MAX;

    float current_speed = 0.0f;
    float smoothed_val = 0.0f;
    float main_val_buffer[MAIN_VAL_BUFFER_SIZE];
    String last_display_unit = UNIT_TEXT_LH;

    unsigned long button_press_timer_start = 0;
    bool button_trip_reset_candidate = false;
    bool button_display_mode_switch_candidate = false;
    bool button_trip_ready_beep_played = false;

    ErrorIconType active_errors[ICON_COUNT];
    int active_errors_count = 0;
    int current_error_display_index = 0;
    unsigned long last_error_cycle_time_ms = 0;
    bool blink_on = true;
    unsigned long last_blink_toggle_time_ms = 0;
    bool sensor_alarm_active = false;
    uint8_t alarm_phase = 0;
    unsigned long alarm_phase_start_time_ms = 0;
    bool is_low_fuel_active_by_hysteresis = false;
    uint8_t non_critical_cycle_phase = 0;
    unsigned long non_critical_last_state_change_time_ms = 0;

    unsigned int current_speaker_freq = 0;
    unsigned int current_speaker_duty = 0;
    bool beep_active = false;
    unsigned long beep_start_ms = 0;
    unsigned long beep_duration_ms = 0;
    unsigned int beep_freq = 0;

    unsigned long startup_phase_start_ms = 0;
    bool fuel_level_available = false;
    float last_smoothed_fuel_percent = 0.0f;
    unsigned long current_time_ms = 0;

    bool is_service_mode = false; 

    struct ServiceDisplayLine {
        String formatted_value;  
        String custom_text;      
        bool has_value = false;  
    };
    ServiceDisplayLine service_display_lines[MAX_SERVICE_DISPLAY_LINES]; 
    unsigned long last_service_mode_kline_update_ms = 0; 

    void init_state();

    float get_trip_fuel_consumed_L();
    void set_trip_fuel_consumed_L(float value);
    float get_trip_distance_travelled_km();
    void set_trip_distance_travelled_km(float value);
    float get_persistent_trip_fuel_L();
    void set_persistent_trip_fuel_L(float value);
    float get_persistent_trip_distance_km();
    void set_persistent_trip_distance_km(float value);
    int get_current_main_display_mode();
    void set_current_main_display_mode(int value);
};

#endif // STATE_MANAGER_H