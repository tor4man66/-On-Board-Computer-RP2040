// =====================================================================================================================
// --- Display Components Implementation (DisplayComponents.cpp) ---
// =====================================================================================================================
#include "DisplayComponents.h"
#include "Config.h"
#include "DebugUtils.h"

void draw_fuel_gauge(Adafruit_GFX* oled_obj, StateManager& state_manager, float fuel_level_percent,
                     int16_t x, int16_t y, int16_t total_width, int16_t block_height, uint8_t num_blocks, uint8_t block_spacing) {
    if (!oled_obj || total_width <= 0 || block_height <= 0 || num_blocks <= 0) {
        return;
    }

    int16_t total_spacing_width = (num_blocks - 1) * block_spacing;
    if (total_width <= total_spacing_width) {
        return;
    }

    int16_t individual_block_width = (total_width - total_spacing_width) / num_blocks;
    if (individual_block_width <= 0) {
        return;
    }

    uint8_t filled_blocks_count;
    if (fuel_level_percent < 0) {
        filled_blocks_count = 0;
    } else {
        filled_blocks_count = (uint8_t)((fuel_level_percent / 100.0f) * num_blocks);
        if (fuel_level_percent > 0 && filled_blocks_count == 0 && fuel_level_percent < 100) {
            filled_blocks_count = 1;
        } else if (fuel_level_percent >= 100) {
            filled_blocks_count = num_blocks;
        }
    }

    for (uint8_t i = 0; i < num_blocks; ++i) {
        int16_t current_block_x = x + i * (individual_block_width + block_spacing);
        oled_obj->drawRect(current_block_x, y, individual_block_width, block_height, 1);

        if (i < filled_blocks_count) {
            bool is_last_filled_block = (i == filled_blocks_count - 1);
            if (!is_last_filled_block || state_manager.blink_on) {
                int16_t fill_padding = 1;
                if (individual_block_width > 2 * fill_padding && block_height > 2 * fill_padding) {
                    oled_obj->fillRect(
                        current_block_x + fill_padding,
                        y + fill_padding,
                        individual_block_width - 2 * fill_padding,
                        block_height - 2 * fill_padding,
                        1
                    );
                }
            }
        }
    }
}

Mode0DisplayData prepare_mode_0_data(StateManager& state_manager, FuelMonitor& fuel_monitor, EngineMonitor& engine_monitor) {
    Mode0DisplayData data;
    float current_speed_kmh = state_manager.current_speed;
    float display_value = state_manager.smoothed_val;

    char value_buffer[8];
    bool raw_value_is_valid = true;

    if (state_manager.is_towing_mode_active) {
        strcpy(value_buffer, MAIN_VALUE_TOWING_TEXT);
        raw_value_is_valid = false;
    } else if ((current_speed_kmh > COASTING_MIN_SPEED_KMH &&
                state_manager.rpm > COASTING_MIN_RPM &&
                display_value < COASTING_FUEL_THRESHOLD_LH)) {
        strcpy(value_buffer, MAIN_VALUE_COASTING_TEXT);
        raw_value_is_valid = false;
    } else if (display_value < STATIONARY_THRESHOLD) {
        strcpy(value_buffer, MAIN_VALUE_STATIONARY_TEXT);
        raw_value_is_valid = false;
    } else if (display_value > MAX_DISPLAY_L100KM_VALUE) {
        strcpy(value_buffer, MAIN_VALUE_ERROR_TEXT);
        raw_value_is_valid = false;
    } else {
        if (display_value < 10.0f) {
            sprintf(value_buffer, "%4.2f", display_value);
        } else {
            sprintf(value_buffer, "%4.1f", display_value);
        }
    }
    data.main_value_str = value_buffer;
    data.main_value_font_size = MAIN_VALUE_FONT_SIZE;

    data.main_unit_text = UNIT_TEXT_LH2;
    data.can_show_l100km = (current_speed_kmh >= MIN_SPEED_FOR_L100KM_KMH) &&
                           (state_manager.get_trip_distance_travelled_km() >= MIN_DISTANCE_FOR_L100KM_KM);
    if (data.can_show_l100km && raw_value_is_valid) {
        data.main_unit_text = UNIT_TEXT_L100KM2;
    }
    data.main_unit_font_size_x = MAIN_UNIT_FONT_SIZE_X;
    data.main_unit_font_size_y = MAIN_UNIT_FONT_SIZE_Y;

    float avg_p_val = 0.0f;
    if (state_manager.get_persistent_trip_distance_km() >= MIN_PERS_DISPLAY_DISTANCE_KM) {
        if (state_manager.get_persistent_trip_distance_km() > 0.001f) {
            avg_p_val = (state_manager.get_persistent_trip_fuel_L() / state_manager.get_persistent_trip_distance_km()) * 100.0f;
        }
    }
    char pers_avg_buffer[8];
    if (avg_p_val > 0.0f && avg_p_val <= MAX_DISPLAY_L100KM_VALUE) {
        sprintf(pers_avg_buffer, "%*.1f", PERS_L100KM_DISPLAY_WIDTH, avg_p_val);
    } else {
        sprintf(pers_avg_buffer, "%*s", PERS_L100KM_DISPLAY_WIDTH, PERS_L100KM_DEFAULT_TEXT);
    }
    data.pers_stat_text = String(pers_avg_buffer) + UNIT_TEXT_L100KM + "   ";

    char trip_fuel_buffer[8];
    float f_str_val = state_manager.get_trip_fuel_consumed_L();
    if (f_str_val >= TRIP_FUEL_MIN_DISPLAY_L) {
        sprintf(trip_fuel_buffer, "%*.1f", TRIP_FUEL_DISPLAY_WIDTH, f_str_val);
    } else {
        sprintf(trip_fuel_buffer, "%*s", TRIP_FUEL_DISPLAY_WIDTH, TRIP_FUEL_DEFAULT_TEXT);
    }

    char trip_distance_buffer[8];
    int d_str_val = (int)state_manager.get_trip_distance_travelled_km();
    if (d_str_val >= TRIP_DISTANCE_MIN_DISPLAY_KM) {
        sprintf(trip_distance_buffer, "%*d", TRIP_DISTANCE_DISPLAY_WIDTH, d_str_val);
    } else {
        sprintf(trip_distance_buffer, "%*s", TRIP_DISTANCE_DISPLAY_WIDTH, TRIP_DISTANCE_DEFAULT_TEXT);
    }
    data.trip_stat_text = String(trip_fuel_buffer) + UNIT_TEXT_LITERS + "  " + String(trip_distance_buffer) + UNIT_TEXT_KM;

    data.battery_voltage = state_manager.current_battery_voltage;
    return data;
}

Mode1DisplayData prepare_mode_1_data(StateManager& state_manager, FuelMonitor& fuel_monitor, EngineMonitor& engine_monitor) {
    Mode1DisplayData data;

    float current_speed_kmh = state_manager.current_speed;
    char speed_buffer[8];
    if (current_speed_kmh <= 0) {
        strcpy(speed_buffer, SP_SCR_DEFAULT_TEXT_KMH);
    } else {
        sprintf(speed_buffer, "%3d", (int)current_speed_kmh);
    }
    data.speed_text = String(speed_buffer) + UNIT_TEXT_KMH;

    char voltage_buffer[8];
    if (state_manager.current_battery_voltage < 0.001f || state_manager.current_battery_voltage <= SP_SCR_VOLTAGE_MIN_DISPLAY_V) {
        strcpy(voltage_buffer, SP_SCR_DEFAULT_TEXT_V);
    } else {
        sprintf(voltage_buffer, "%.1f", state_manager.current_battery_voltage);
    }
    data.voltage_text = String(voltage_buffer) + UNIT_TEXT_VOLTS;

    float rpm_val = engine_monitor.get_current_rpm();
    char rpm_buffer[8];
    if (rpm_val < 0.001f) {
        state_manager.avg_rpm_for_display = 0.0f;
        strcpy(rpm_buffer, SP_SCR_DEFAULT_TEXT_RPM);
    } else {
        float current_rpm_raw = rpm_val;
        if (current_rpm_raw > 0) {
            if (state_manager.avg_rpm_for_display < 0.001f) {
                state_manager.avg_rpm_for_display = current_rpm_raw;
            } else {
                state_manager.avg_rpm_for_display = (state_manager.avg_rpm_for_display * SP_SCR_RPM_SMOOTH_FACTOR) +
                                                   (current_rpm_raw * (1.0f - SP_SCR_RPM_SMOOTH_FACTOR));
            }
        } else {
            state_manager.avg_rpm_for_display = 0.0f;
        }
        if (state_manager.avg_rpm_for_display <= MIN_DISPLAY_RPM) {
            strcpy(rpm_buffer, SP_SCR_DEFAULT_TEXT_RPM);
        } else {
            sprintf(rpm_buffer, "%4d", (int)state_manager.avg_rpm_for_display);
        }
    }
    data.rpm_text = String(rpm_buffer) + UNIT_TEXT_RPM;

    float fuel_percent_val = fuel_monitor.get_smoothed_fuel_percent();
    char fuel_buffer[8];
    if (fuel_percent_val < 0.0f || fuel_percent_val <= SP_SCR_FUEL_MIN_DISPLAY_PERCENT) {
        strcpy(fuel_buffer, SP_SCR_FUEL_DEFAULT_TEXT);
    } else {
        float fuel_L = (fuel_percent_val / 100.0f) * FUEL_TANK_CAPACITY_L;
        sprintf(fuel_buffer, "%2d", (int)fuel_L);
    }
    data.fuel_text = String(fuel_buffer) + UNIT_TEXT_LITERS;

    int inj_us = engine_monitor.get_current_inj_period();
    char inj_buffer[8];
    if (inj_us <= 0) {
        state_manager.avg_inj_ms = 0.0f;
        strcpy(inj_buffer, SP_SCR_DEFAULT_TEXT_MS);
    } else {
        float current_ms = (float)inj_us / 1000.0f;
        if (current_ms > 0) {
            if (state_manager.avg_inj_ms < 0.001f) {
                state_manager.avg_inj_ms = current_ms;
            } else {
                state_manager.avg_inj_ms = (state_manager.avg_inj_ms * SP_SCR_INJ_SMOOTH_FACTOR) +
                                           (current_ms * (1.0f - SP_SCR_INJ_SMOOTH_FACTOR));
            }
        } else {
            state_manager.avg_inj_ms = 0.0f;
        }
        if (state_manager.avg_inj_ms <= 0.0f || state_manager.avg_inj_ms > 99.99f) {
            strcpy(inj_buffer, SP_SCR_DEFAULT_TEXT_MS);
        } else {
            sprintf(inj_buffer, "%5.2f", state_manager.avg_inj_ms);
        }
    }
    data.inj_text = String(inj_buffer) + UNIT_TEXT_MS;

    return data;
}

Mode2DisplayData prepare_mode_2_data(StateManager& state_manager, KLineManager& kline_manager) {
    Mode2DisplayData data;
    data.fault_count = 0;
    data.is_clearing = false;
    data.acc_off = !state_manager.acc_present;

    String temp_mode = state_manager.get_and_reset_temp_display_mode();
    if (temp_mode == "DTC_CLEANING") {
        data.is_clearing = true;
        data.status_text = DTC_TEXT_CLEANING;
        return data;
    }

    KLineConnectionState conn_state = kline_manager.getConnectionState();
    bool has_dtcs = kline_manager.has_dtcs();

    if (data.acc_off) {
        data.status_text = DTC_TEXT_SLEEP;
        return data;
    }

    if (conn_state == KLINE_CONNECTING) {
        data.status_text = DTC_TEXT_CONNECTING;
        return data;
    }

    if (conn_state != KLINE_CONNECTED) {
        data.status_text = DTC_TEXT_ERROR;
        return data;
    }

    if (!has_dtcs) {
        data.status_text = DTC_TEXT_NO_ERRORS;
        return data;
    }

    data.status_text = "";
    data.fault_count = kline_manager.getFaultCount();
    uint8_t* faults = kline_manager.getFaultCodes();

    int codes_to_copy = std::min((uint8_t)DTC_MAX_CODES_TO_DISPLAY, data.fault_count);
    for (uint8_t i = 0; i < codes_to_copy; ++i) {
        data.fault_codes[i] = KLineKWP1281Lib::getFaultCode(i, data.fault_count, faults, 3 * KLINE_DTC_BUFFER_MAX_FAULT_CODES);
    }
    data.fault_count = codes_to_copy;

    return data;
}

ServiceModeDisplayData prepare_service_mode_data(StateManager& state_manager) {
    ServiceModeDisplayData data;

    switch (state_manager.kline_connection_status) {
        case KLINE_DISCONNECTED:
            data.kline_status_text = "K-LINE: DISC";
            break;
        case KLINE_CONNECTING:
            data.kline_status_text = "K-LINE: CONN...";
            break;
        case KLINE_CONNECTED:
            data.kline_status_text = "K-LINE: OK";
            break;
        case KLINE_PERMANENTLY_DISCONNECTED:
            data.kline_status_text = "K-LINE: FAIL";
            break;
    }

    for (uint8_t i = 0; i < MAX_SERVICE_DISPLAY_LINES; ++i) {
        data.line_values[i] = state_manager.service_display_lines[i].formatted_value;
        data.line_texts[i] = state_manager.service_display_lines[i].custom_text;
    }

    return data;
}