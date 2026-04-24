// =====================================================================================================================
// --- Hardware Manager Implementation (HardwareManager.cpp) ---
// =====================================================================================================================
#include "HardwareManager.h"
#include "Config.h"
#include "DebugUtils.h"
#include <Adafruit_SH110X.h>

HardwareManager::HardwareManager(StateManager& state_manager_instance) : _state(state_manager_instance) {
    log_init_debug("Ініціалізація HardwareManager...");
    _initialized = true;
    _state.kline_error_message[0] = '\0';
}

void HardwareManager::init_all_hardware() {
    log_init_debug("Запуск ініціалізації всього обладнання...");
    _init_pins();
    _init_adc();
    _init_pwm_speaker();
    _init_wdt();
    _init_oled();
    log_init_debug("Ініціалізація всього обладнання - ЗАВЕРШЕНО.");
}

void HardwareManager::_init_pins() {
    log_init_debug("Ініціалізація виводів GPIO...");
    if (ENABLE_PIN_INJ) {
        _inj_pin_num = PIN_INJ;
        pinMode(_inj_pin_num, INPUT_PULLUP);
        log_init_debug_str(String("  Пін форсунки (GPIO ") + _inj_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  Пін форсунки (GPIO ") + PIN_INJ + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_VSS) {
        _vss_pin_num = PIN_VSS;
        pinMode(_vss_pin_num, INPUT_PULLUP);
        log_init_debug_str(String("  Пін датчика VSS (GPIO ") + _vss_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  Пін датчика VSS (GPIO ") + PIN_VSS + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_BUTTON_RESET) {
        _reset_button_pin_num = PIN_BUTTON_RESET;
        pinMode(_reset_button_pin_num, INPUT_PULLUP);
        log_init_debug_str(String("  Пін кнопки скидання (GPIO ") + _reset_button_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  Пін кнопки скидання (GPIO ") + PIN_BUTTON_RESET + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_SENSOR_BRAKE_FLUID) {
        _brake_fluid_sensor_pin_num = PIN_SENSOR_BRAKE_FLUID;
        pinMode(_brake_fluid_sensor_pin_num, INPUT_PULLUP);
        log_init_debug_str(String("  Пін датчика гальмівної рідини (GPIO ") + _brake_fluid_sensor_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  Пін датчика гальмівної рідини (GPIO ") + PIN_SENSOR_BRAKE_FLUID + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_SENSOR_OIL_PRESSURE_0_3) {
        _oil_pressure_0_3_sensor_pin_num = PIN_SENSOR_OIL_PRESSURE_0_3;
        pinMode(_oil_pressure_0_3_sensor_pin_num, INPUT_PULLUP);
        log_init_debug_str(String("  Пін датчика тиску масла 0.3 бар (GPIO ") + _oil_pressure_0_3_sensor_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  Пін датчика тиску масла 0.3 бар (GPIO ") + PIN_SENSOR_OIL_PRESSURE_0_3 + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_SENSOR_OVERHEAT_AND_LOW_COOLANT) {
        _overheat_and_low_coolant_sensor_pin_num = PIN_SENSOR_OVERHEAT_AND_LOW_COOLANT;
        pinMode(_overheat_and_low_coolant_sensor_pin_num, INPUT_PULLUP);
        log_init_debug_str(String("  Пін датчика перегріву/низького рівня охолоджуючої рідини (GPIO ") + _overheat_and_low_coolant_sensor_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  Пін датчика перегріву/низького рівня охолоджуючої рідини (GPIO ") + PIN_SENSOR_OVERHEAT_AND_LOW_COOLANT + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_SENSOR_OIL_PRESSURE_1_8) {
        _oil_pressure_1_8_sensor_pin_num = PIN_SENSOR_OIL_PRESSURE_1_8;
        pinMode(_oil_pressure_1_8_sensor_pin_num, INPUT_PULLUP);
        log_init_debug_str(String("  Пін датчика тиску масла 1.8 бар (GPIO ") + _oil_pressure_1_8_sensor_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  Пін датчика тиску масла 1.8 бар (GPIO ") + PIN_SENSOR_OIL_PRESSURE_1_8 + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_IGNITION_RPM) {
        _ignition_rpm_pin_num = PIN_IGNITION_RPM;
        pinMode(_ignition_rpm_pin_num, INPUT_PULLUP);
        log_init_debug_str(String("  Пін запалювання RPM (GPIO ") + _ignition_rpm_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  Пін запалювання RPM (GPIO ") + PIN_IGNITION_RPM + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_ACC) {
        _acc_pin_num = PIN_ACC;
        pinMode(_acc_pin_num, INPUT_PULLDOWN);
        log_init_debug_str(String("  Пін ACC (GPIO ") + _acc_pin_num + ") - УСПІХ.");
    } else {
        _acc_pin_num = -1;
        log_init_debug("  Пін ACC - ВИМКНЕНО користувачем в налаштуваннях.");
    }

    if (KLINE_ENABLE) {
        _check_engine_light_pin_num = KLINE_CHECK_ENGINE_LIGHT_PIN;
        pinMode(_check_engine_light_pin_num, OUTPUT);
        digitalWrite(_check_engine_light_pin_num, LOW);
        log_init_debug_str(String("  Пін індикатора Check Engine (GPIO ") + _check_engine_light_pin_num + ") - УСПІХ.");
    } else {
        _check_engine_light_pin_num = -1;
        log_init_debug_str(String("  Пін індикатора Check Engine (GPIO ") + KLINE_CHECK_ENGINE_LIGHT_PIN + ") - ВИМКНЕНО (KLINE_ENABLE = false).");
    }

    log_init_debug("Ініціалізація виводів GPIO - ЗАВЕРШЕНО.");
}

void HardwareManager::_init_adc() {
    log_init_debug("Ініціалізація АЦП...");
    analogReadResolution(12);
    if (ENABLE_PIN_BOARD_VOLTAGE_ADC) {
        _voltage_adc_pin_num = PIN_ADC;
        log_init_debug_str(String("  АЦП для напруги плати (GPIO ") + _voltage_adc_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  АЦП для напруги плати (GPIO ") + PIN_ADC + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_FUEL_LEVEL_ADC) {
        _fuel_level_adc_pin_num = PIN_FUEL_LEVEL_ADC;
        log_init_debug_str(String("  АЦП для рівня палива (GPIO ") + _fuel_level_adc_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  АЦП для рівня палива (GPIO ") + PIN_FUEL_LEVEL_ADC + ") - ВИМКНЕНО в налаштуваннях.");
    }
    if (ENABLE_PIN_BRIGHTNESS_ADC) {
        _brightness_adc_pin_num = PIN_BRIGHTNESS_ADC;
        log_init_debug_str(String("  АЦП для яскравості OLED (GPIO ") + _brightness_adc_pin_num + ") - УСПІХ.");
    } else {
        log_init_debug_str(String("  АЦП для яскравості OLED (GPIO ") + PIN_BRIGHTNESS_ADC + ") - ВИМКНЕНО в налаштуваннях.");
    }
    log_init_debug("Ініціалізація АЦП - ЗАВЕРШЕНО.");
}

void HardwareManager::_init_pwm_speaker() {
    log_init_debug("Ініціалізація ШІМ-динаміка...");
    if (ENABLE_SPEAKER) {
        _speaker_pin_num = PIN_SPEAKER;
        pinMode(_speaker_pin_num, OUTPUT);
        _state.current_speaker_duty = 0;
        _state.current_speaker_freq = 0;
        log_init_debug_str(String("  ШІМ-динамік (GPIO ") + _speaker_pin_num + ") - УСПІХ.");
    } else {
        _speaker_pin_num = -1;
        log_init_debug_str(String("  ШІМ-динамік (GPIO ") + PIN_SPEAKER + ") - ВИМКНЕНО в налаштуваннях.");
    }
    log_init_debug("Ініціалізація ШІМ-динаміка - ЗАВЕРШЕНО.");
}

void HardwareManager::_init_wdt() {
    log_init_debug("Ініціалізація сторожового таймера...");
    if (ENABLE_WATCHDOG_TIMER) {
        #ifdef ARDUINO_ARCH_RP2040
        watchdog_enable(WATCHDOG_TIME_RESET, 1);
        log_init_debug("  Сторожовий таймер - УСПІШНО ініціалізовано (RP2040).");
        #else
        _state.error_logger->log_error("Сторожовий таймер увімкнено, але не підтримується цією архітектурою.");
        log_init_debug("  Сторожовий таймер - УВІМКНЕНО, але не підтримується цією архітектурою (не-RP2040). Будь ласка, налаштуйте для вашої плати.");
        #endif
    } else {
        log_init_debug("  Сторожовий таймер - ВИМКНЕНО в налаштуваннях.");
    }
    log_init_debug("Ініціалізація сторожового таймера - ЗАВЕРШЕНО.");
}

void HardwareManager::_init_oled() {
    log_init_debug("Ініціалізація OLED дисплея...");
    String oled_status_str = "OFF";
    if (ENABLE_OLED) {
        _i2c = &Wire;
        #ifdef ARDUINO_ARCH_RP2040
        _i2c->setSDA(PIN_I2C_SDA);
        _i2c->setSCL(PIN_I2C_SCL);
        log_init_debug_str(String("  Налаштування виводів I2C: SDA (GPIO") + PIN_I2C_SDA + "), SCL (GPIO" + PIN_I2C_SCL + ").");
        #endif
        _i2c->begin();
        _i2c->setClock(I2C_FREQ);
        byte error, address;
        int nDevices = 0;
        log_init_debug("  Сканування шини I2C...");
        for (address = 1; address < 127; address++) {
            _i2c->beginTransmission(address);
            error = _i2c->endTransmission();
            if (error == 0) {
                log_init_debug_str(String("  Пристрій I2C знайдено за адресою 0x") + String(address, HEX));
                nDevices++;
            } else if (error == 4) {
                log_init_debug_str(String("  Невідома помилка або пристрій зайнятий за адресою 0x") + String(address, HEX) + ".");
            }
        }
        if (nDevices == 0) {
            log_init_debug("  Пристроїв I2C не знайдено.");
        } else {
            log_init_debug_str(String("  Знайдено ") + nDevices + " пристроїв I2C.");
        }
        Adafruit_SH1107* sh1107_driver = new Adafruit_SH1107(OLED_WIDTH, OLED_HEIGHT, _i2c, -1);
        if (sh1107_driver->begin(OLED_ADDR_HEX, true)) {
            sh1107_driver->setRotation(3);
            sh1107_driver->setContrast(OLED_CONTRAST);
            sh1107_driver->clearDisplay();
            oled_status_str = "OK";
            log_init_debug_str(String("  OLED SH1107 за адресою 0x") + String(OLED_ADDR_HEX, HEX) + " - УСПІШНО ініціалізовано.");
            _state.oled_driver_instance = sh1107_driver;
        } else {
            _state.error_logger->log_error_str(String("OLED SH1107 за адресою 0x") + String(OLED_ADDR_HEX, HEX) + " - ПОМИЛКА ІНІЦІАЛІЗАЦІЇ. Перевірте з'єднання або тип дисплея.");
            log_init_debug_str(String("  OLED SH1107 за адресою 0x") + String(OLED_ADDR_HEX, HEX) + " - ПОМИЛКА ІНІЦІАЛІЗАЦІЇ. Перевірте з'єднання або тип дисплея.");
            delete sh1107_driver;
            _state.oled_driver_instance = nullptr;
        }
    } else {
        log_init_debug("  OLED дисплей - ВИМКНЕНО в налаштуваннях.");
    }
    _state.oled_status = oled_status_str;
    log_init_debug_str(String("Ініціалізація OLED дисплея - ЗАВЕРШЕНО. Статус: ") + oled_status_str + ".");
}

void HardwareManager::feed_wdt() {
    if (ENABLE_WATCHDOG_TIMER) {
        #ifdef ARDUINO_ARCH_RP2040
        watchdog_update();
        #endif
    }
}

int HardwareManager::get_voltage_adc_raw() {
    if (!ENABLE_PIN_BOARD_VOLTAGE_ADC || _voltage_adc_pin_num == -1) {
        return 0;
    }
    return analogRead(_voltage_adc_pin_num);
}

int HardwareManager::get_fuel_adc_raw() {
    if (!ENABLE_PIN_FUEL_LEVEL_ADC || _fuel_level_adc_pin_num == -1) {
        return 0;
    }
    return analogRead(_fuel_level_adc_pin_num);
}

int HardwareManager::get_brightness_adc_raw() {
    if (!ENABLE_PIN_BRIGHTNESS_ADC || _brightness_adc_pin_num == -1) {
        return 0;
    }
    return analogRead(_brightness_adc_pin_num);
}

bool HardwareManager::get_acc_status() {
    if (!ENABLE_PIN_ACC || _acc_pin_num == -1) {
        return true;
    }
    return digitalRead(_acc_pin_num) == HIGH;
}

void HardwareManager::set_speaker_freq_duty(unsigned int freq, unsigned int duty) {
    if (!ENABLE_SPEAKER || _speaker_pin_num == -1) return;
    if (freq == 0) {
        if (_state.current_speaker_duty != 0) {
            noTone(_speaker_pin_num);
            _state.current_speaker_duty = 0;
        }
        _state.current_speaker_freq = 0;
    } else {
        if (_state.current_speaker_freq != freq) {
            tone(_speaker_pin_num, freq);
        }
        _state.current_speaker_duty = duty;
        _state.current_speaker_freq = freq;
    }
}

bool HardwareManager::get_brake_fluid_status() {
    return (_brake_fluid_sensor_pin_num != -1) && (digitalRead(_brake_fluid_sensor_pin_num) == SENSOR_ACTIVE_LOW_VALUE);
}

bool HardwareManager::get_oil_pressure_0_3_status() {
    return (_oil_pressure_0_3_sensor_pin_num != -1) && (digitalRead(_oil_pressure_0_3_sensor_pin_num) == SENSOR_ACTIVE_LOW_VALUE);
}

bool HardwareManager::get_oil_pressure_1_8_status() {
    return (_oil_pressure_1_8_sensor_pin_num != -1) && (digitalRead(_oil_pressure_1_8_sensor_pin_num) == SENSOR_ACTIVE_HIGH_VALUE);
}

bool HardwareManager::get_overheat_and_low_coolant_status() {
    return (_overheat_and_low_coolant_sensor_pin_num != -1) && (digitalRead(_overheat_and_low_coolant_sensor_pin_num) == SENSOR_ACTIVE_LOW_VALUE);
}

int HardwareManager::get_inj_pin_num() { return _inj_pin_num; }
int HardwareManager::get_vss_pin_num() { return _vss_pin_num; }
int HardwareManager::get_ignition_rpm_pin_num() { return _ignition_rpm_pin_num; }
int HardwareManager::get_reset_button_pin_num() { return _reset_button_pin_num; }
int HardwareManager::get_speaker_pin_num() { return _speaker_pin_num; }
int HardwareManager::get_brake_fluid_sensor_pin_num() { return _brake_fluid_sensor_pin_num; }
int HardwareManager::get_oil_pressure_0_3_sensor_pin_num() { return _oil_pressure_0_3_sensor_pin_num; }
int HardwareManager::get_oil_pressure_1_8_sensor_pin_num() { return _oil_pressure_1_8_sensor_pin_num; }
int HardwareManager::get_overheat_and_low_coolant_sensor_pin_num() { return _overheat_and_low_coolant_sensor_pin_num; }
int HardwareManager::get_voltage_adc_pin_num() { return _voltage_adc_pin_num; }
int HardwareManager::get_fuel_level_adc_pin_num() { return _fuel_level_adc_pin_num; }
int HardwareManager::get_brightness_adc_pin_num() { return _brightness_adc_pin_num; }
int HardwareManager::get_acc_pin_num() { return _acc_pin_num; }

void HardwareManager::set_check_engine_light_state(bool state) {
    if (KLINE_ENABLE && _check_engine_light_pin_num != -1) {
        digitalWrite(_check_engine_light_pin_num, state ? HIGH : LOW);
    }
}
int HardwareManager::get_check_engine_light_pin_num() { return _check_engine_light_pin_num; }

void HardwareManager::enterLowPowerMode() {
    log_debug("HardwareManager: Вхід у режим низького споживання...");
    
    if (_state.oled_driver_instance != nullptr) {
        static_cast<Adafruit_SH1107*>(_state.oled_driver_instance)->oled_command(SH110X_DISPLAYOFF);
        delay(10);
    }
    
    if (_speaker_pin_num != -1) {
        noTone(_speaker_pin_num);
        digitalWrite(_speaker_pin_num, LOW);
    }
    
    if (KLINE_ENABLE && _check_engine_light_pin_num != -1) {
        digitalWrite(_check_engine_light_pin_num, LOW);
    }
    
    log_debug("HardwareManager: Готово до сну.");
}

void HardwareManager::exitLowPowerMode() {
    log_debug("HardwareManager: Пробудження...");
    
    if (_state.oled_driver_instance != nullptr) {
        static_cast<Adafruit_SH1107*>(_state.oled_driver_instance)->oled_command(SH110X_DISPLAYON);
        static_cast<Adafruit_SH1107*>(_state.oled_driver_instance)->setContrast(_state.current_oled_contrast);
    }
    
    log_debug("HardwareManager: Пробудження завершено.");
}
