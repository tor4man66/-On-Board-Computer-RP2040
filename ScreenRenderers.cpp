// =====================================================================================================================
// --- Screen Renderers Implementation (ScreenRenderers.cpp) ---
// =====================================================================================================================
#include "ScreenRenderers.h"
#include "Config.h"
#include "DebugUtils.h"

#define MAX_STRING 270
char target[MAX_STRING + 1] = "";

struct UTF8Replace {
    uint8_t firstByte;
    uint8_t secondByte;
    uint8_t replacement;
};

const UTF8Replace replacements[] = {
    {208, 132, 194}, {209, 148, 195},
    {208, 134, 196}, {209, 150, 197},
    {208, 135, 198}, {210, 151, 199},
    {210, 144, 200}, {210, 145, 201}
};
const int replacementsCount = sizeof(replacements) / sizeof(replacements[0]);

inline char* Utf8win1251(const char* source) {
    strcpy(target, "");
    int i = 0, j = 0;
    while (source[i] && j < MAX_STRING) {
        unsigned char first = source[i++];
        unsigned char n = first;
        if (first >= 127) {
            unsigned char second = source[i++];
            bool replaced = false;
            for (int k = 0; k < replacementsCount; k++) {
                if (replacements[k].firstByte == first && replacements[k].secondByte == second) {
                    n = replacements[k].replacement;
                    replaced = true;
                    break;
                }
            }
            if (!replaced) n = second;
        }
        target[j++] = n;
    }
    target[j] = '\0';
    return target;
}

void render_mode_0_screen(Adafruit_GFX* oled_obj, const Mode0DisplayData& data) {
    if (!oled_obj) return;

    const char* value_str = data.main_value_str.c_str();
    uint8_t text_size_main = data.main_value_font_size;

    int16_t main_val_calculated_width = strlen(value_str) * FONT_WIDTH_PX * text_size_main;
    int16_t main_val_rendered_x = (128 - main_val_calculated_width) / 2 + MAIN_VALUE_TEXT_X_OFFSET_ADJUST;
    int16_t main_val_y_pos = MAIN_VALUE_Y_POS;

    large_text(oled_obj, value_str, main_val_rendered_x, main_val_y_pos, text_size_main, 1);

    oled_obj->setFont(MAIN_UNIT_TEXT_FONT);
    oled_obj->setTextColor(1);

    const char* unit_text = data.main_unit_text.c_str();
    uint8_t unit_text_size_x = data.main_unit_font_size_x;
    uint8_t unit_text_size_y = data.main_unit_font_size_y;
    int16_t unit_y_pos = main_val_y_pos + (FONT_HEIGHT_PX * text_size_main) + MAIN_UNIT_Y_OFFSET;

    int16_t current_unit_width = strlen(unit_text) * FONT_WIDTH_PX * unit_text_size_x;
    int16_t unit_rendered_x_target_right_edge = main_val_rendered_x + main_val_calculated_width - MAIN_UNIT_RIGHT_ALIGN_OFFSET_PX;
    int16_t unit_rendered_x = unit_rendered_x_target_right_edge - current_unit_width;

    stretched_text(oled_obj, unit_text, unit_rendered_x, unit_y_pos, unit_text_size_x, unit_text_size_y, 1);

    oled_obj->setFont(NULL);
    oled_obj->setFont(STAT_TEXT_FONT);
    oled_obj->setTextColor(1);
    stretched_text(oled_obj, data.pers_stat_text.c_str(), STAT_TEXT_PERS_X_POS, PERS_STAT_Y_POS,
                   STAT_FONT_SIZE_X, STAT_FONT_SIZE_Y, 1);
    stretched_text(oled_obj, data.trip_stat_text.c_str(), STAT_TEXT_TRIP_X_POS, TRIP_STAT_Y_POS,
                   STAT_FONT_SIZE_X, STAT_FONT_SIZE_Y, 1);

    oled_obj->setFont(NULL);
}

void render_mode_1_screen(Adafruit_GFX* oled_obj, const Mode1DisplayData& data) {
    if (!oled_obj) return;

    stretched_text(oled_obj, data.speed_text.c_str(), SP_SCR_SPEED_X, SP_SCR_SPEED_Y,
                   SP_SCR_SPEED_FONT_SIZE, SP_SCR_SPEED_FONT_SIZE, 1);
    stretched_text(oled_obj, data.voltage_text.c_str(), SP_SCR_VOLTAGE_X, SP_SCR_VOLTAGE_Y,
                   SP_SCR_VOLTAGE_FONT_SIZE, SP_SCR_VOLTAGE_FONT_SIZE, 1);
    stretched_text(oled_obj, data.rpm_text.c_str(), SP_SCR_RPM_X, SP_SCR_RPM_Y,
                   SP_SCR_RPM_FONT_SIZE, SP_SCR_RPM_FONT_SIZE, 1);
    stretched_text(oled_obj, data.fuel_text.c_str(), SP_SCR_FUEL_X, SP_SCR_FUEL_Y,
                   SP_SCR_FUEL_FONT_SIZE, SP_SCR_FUEL_FONT_SIZE, 1);
    stretched_text(oled_obj, data.inj_text.c_str(), SP_SCR_INJ_X, SP_SCR_INJ_Y,
                   SP_SCR_INJ_FONT_SIZE, SP_SCR_INJ_FONT_SIZE_Y_FACTOR, 1);
}

void render_mode_2_screen(Adafruit_GFX* oled_obj, const Mode2DisplayData& data) {
    if (!oled_obj) return;

    oled_obj->setTextColor(1);
    oled_obj->setFont(DTC_STATUS_FONT);  

    if (data.is_clearing) {
        stretched_text(oled_obj, data.status_text.c_str(), 
                       DTC_CLEANING_X_POS, DTC_CLEANING_Y_POS,
                       DTC_CLEANING_FONT_SIZE_X, DTC_CLEANING_FONT_SIZE_Y, 1);
        oled_obj->setFont(NULL);
        return;
    }

    if (data.acc_off) {
        stretched_text(oled_obj, data.status_text.c_str(),
                       DTC_SLEEP_X_POS, DTC_SLEEP_Y_POS,
                       DTC_SLEEP_FONT_SIZE_X, DTC_SLEEP_FONT_SIZE_Y, 1);
        oled_obj->setFont(NULL);
        return;
    }

    if (data.status_text == DTC_TEXT_CONNECTING) {
        stretched_text(oled_obj, data.status_text.c_str(),
                       DTC_CONNECTING_X_POS, DTC_CONNECTING_Y_POS,
                       DTC_CONNECTING_FONT_SIZE_X, DTC_CONNECTING_FONT_SIZE_Y, 1);
        oled_obj->setFont(NULL);
        return;
    }

    if (data.status_text == DTC_TEXT_ERROR) {
        stretched_text(oled_obj, data.status_text.c_str(),
                       DTC_ERROR_X_POS, DTC_ERROR_Y_POS,
                       DTC_ERROR_FONT_SIZE_X, DTC_ERROR_FONT_SIZE_Y, 1);
        oled_obj->setFont(NULL);
        return;
    }

    if (data.status_text == DTC_TEXT_NO_ERRORS) {
        stretched_text(oled_obj, data.status_text.c_str(),
                       DTC_NO_ERRORS_X_POS, DTC_NO_ERRORS_Y_POS,
                       DTC_NO_ERRORS_FONT_SIZE_X, DTC_NO_ERRORS_FONT_SIZE_Y, 1);
        oled_obj->setFont(NULL);
        return;
    }

    oled_obj->setFont(DTC_CODE_TEXT_FONT);
    int y_pos = DTC_CODE_Y_START_POS;
    for (uint8_t i = 0; i < data.fault_count; i++) {
        char buffer[8];
        sprintf(buffer, "%05u", data.fault_codes[i]);

        oled_obj->setCursor(DTC_CODE_X_POS, y_pos);
        oled_obj->setTextSize(1);
        oled_obj->print(buffer);

        y_pos += DTC_CODE_Y_STEP;
    }
    oled_obj->setFont(NULL);
}

void render_service_mode_screen(Adafruit_GFX* oled_obj, const ServiceModeDisplayData& data) {
    if (!oled_obj) return;

    oled_obj->setTextColor(1);
    oled_obj->setTextWrap(false);

    oled_obj->setFont(SERVICE_LINE_TEXT_FONT);
    
    int y_pos = SERVICE_MODE_DISPLAY_Y_START_POS + (SERVICE_MODE_DISPLAY_LINE_HEIGHT / 2);

    for (uint8_t i = 0; i < MAX_SERVICE_DISPLAY_LINES; ++i) {
        oled_obj->setCursor(0, y_pos);
        oled_obj->setTextSize(SERVICE_MODE_DISPLAY_FONT_SIZE);
        oled_obj->print(data.line_values[i]);

        oled_obj->setTextSize(SERVICE_MODE_DISPLAY_TEXT_FONT_SIZE);
        oled_obj->print(data.line_texts[i]);

        y_pos += SERVICE_MODE_DISPLAY_LINE_HEIGHT;
    }
    
    oled_obj->setFont(NULL);
}