// =====================================================================================================================
// --- Hardware Manager (HardwareManager.h) ---
// =====================================================================================================================
#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>

#if KLINE_ENABLE
extern HardwareSerial& KLineSerialPort;
#endif

class HardwareManager {
private:
    StateManager& _state;
    bool _initialized = false;
    int _inj_pin_num = -1;
    int _vss_pin_num = -1;
    int _reset_button_pin_num = -1;
    int _brake_fluid_sensor_pin_num = -1;
    int _oil_pressure_0_3_sensor_pin_num = -1;
    int _overheat_and_low_coolant_sensor_pin_num = -1;
    int _oil_pressure_1_8_sensor_pin_num = -1;
    int _ignition_rpm_pin_num = -1;
    int _speaker_pin_num = -1;
    int _fuel_level_adc_pin_num = -1;
    int _voltage_adc_pin_num = -1;
    int _brightness_adc_pin_num = -1;
    int _acc_pin_num = -1;
    int _check_engine_light_pin_num = -1;
    TwoWire* _i2c = nullptr;

public:
    HardwareManager(StateManager& state_manager_instance);
    void enterLowPowerMode();
    void exitLowPowerMode();
    void init_all_hardware();
    void _init_pins();
    void _init_adc();
    void _init_pwm_speaker();
    void _init_wdt();
    void _init_oled();
    void feed_wdt();
    int get_voltage_adc_raw();
    int get_fuel_adc_raw();
    int get_brightness_adc_raw();
    bool get_acc_status();
    void set_speaker_freq_duty(unsigned int freq, unsigned int duty);
    bool get_brake_fluid_status();
    bool get_oil_pressure_0_3_status();
    bool get_oil_pressure_1_8_status();
    bool get_overheat_and_low_coolant_status();
    int get_inj_pin_num();
    int get_vss_pin_num();
    int get_ignition_rpm_pin_num();
    int get_reset_button_pin_num();
    int get_speaker_pin_num();
    int get_brake_fluid_sensor_pin_num();
    int get_oil_pressure_0_3_sensor_pin_num();
    int get_oil_pressure_1_8_sensor_pin_num();
    int get_overheat_and_low_coolant_sensor_pin_num();
    int get_voltage_adc_pin_num();
    int get_fuel_level_adc_pin_num();
    int get_brightness_adc_pin_num();
    int get_acc_pin_num();
    void set_check_engine_light_state(bool state);
    int get_check_engine_light_pin_num();
};
#endif // HARDWARE_MANAGER_H
