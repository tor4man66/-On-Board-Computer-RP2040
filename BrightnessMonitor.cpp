// =====================================================================================================================
// --- Brightness Monitor Implementation (BrightnessMonitor.cpp) ---
// =====================================================================================================================
#include "BrightnessMonitor.h"
#include "Config.h"
#include "DebugUtils.h"

BrightnessMonitor::BrightnessMonitor(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance)
: _state(state_manager_instance), _hardware(hardware_manager_instance) {
    log_init_debug("Ініціалізація BrightnessMonitor...");
}

void BrightnessMonitor::update(unsigned long current_time_ms) {
    if (!ENABLE_PIN_BRIGHTNESS_ADC || _hardware.get_brightness_adc_pin_num() == -1) {
        return;
    }
    if ((current_time_ms - _state.last_brightness_read_ms) < BRIGHTNESS_ADC_READ_INTERVAL_MS) {
        return;
    }
    _state.last_brightness_read_ms = current_time_ms;

    float raw_adc = (float)_hardware.get_brightness_adc_raw();
    uint8_t new_contrast = 0;

    if (raw_adc <= BRIGHTNESS_ADC_NO_VOLTAGE_THRESHOLD_RAW) {
        new_contrast = OLED_CONTRAST_MAX;
    } else {
        float clamped_adc = max(BRIGHTNESS_ADC_MIN_VOLTAGE_RAW, min(BRIGHTNESS_ADC_MAX_VOLTAGE_RAW, raw_adc));
        float adc_range = BRIGHTNESS_ADC_MAX_VOLTAGE_RAW - BRIGHTNESS_ADC_MIN_VOLTAGE_RAW;
        uint8_t contrast_range = OLED_CONTRAST_MAX - OLED_CONTRAST_MIN;

        if (adc_range <= 0.001f) {
            new_contrast = OLED_CONTRAST_MIN;
        } else {
            float scaled_adc = (clamped_adc - BRIGHTNESS_ADC_MIN_VOLTAGE_RAW) / adc_range;
            new_contrast = (uint8_t)(scaled_adc * contrast_range + OLED_CONTRAST_MIN);
            new_contrast = max((uint8_t)OLED_CONTRAST_MIN, min((uint8_t)OLED_CONTRAST_MAX, new_contrast));
        }
    }

    if (abs((int)new_contrast - (int)_state.last_applied_contrast) >= BRIGHTNESS_CONTRAST_HYSTERESIS_VALUE) {
        _state.current_oled_contrast = new_contrast;
        _state.last_applied_contrast = new_contrast;

        if (_state.oled_driver_instance != nullptr) {
            static_cast<Adafruit_SH1107*>(_state.oled_driver_instance)->setContrast(new_contrast);
            if (abs((int)new_contrast - _last_logged_contrast) >= BRIGHTNESS_CONTRAST_HYSTERESIS_VALUE) {
                log_debug_str(String("BrightnessMonitor: Встановлено контраст OLED: ") + new_contrast + " (сирий ADC: " + raw_adc + ").");
                _last_logged_contrast = new_contrast;
            }
        } else {
            if (abs((int)new_contrast - _last_logged_contrast) >= BRIGHTNESS_CONTRAST_HYSTERESIS_VALUE) {
                log_debug_str(String("BrightnessMonitor: Контраст розраховано до ") + new_contrast + " (OLED неактивний, сирий ADC: " + raw_adc + ").");
                _last_logged_contrast = new_contrast;
            }
        }
    }
}
