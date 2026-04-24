// =====================================================================================================================
// Проект: Бортовий Комп'ютер 🚀
// Оригінальний Автор: tor4man ( GitHub-tor4man66 ) https://github.com/tor4man66/Project-On-Board-Computer-on-Raspberry-Pi-Pico.git
// Дата адаптації: 24.04.2026
// =====================================================================================================================
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <Wire.h> 
#include <EEPROM.h>
#include <algorithm>

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#ifdef ARDUINO_ARCH_RP2040
#include "hardware/watchdog.h"
#endif

#include "Config.h"

#include "DebugUtils.h"
#include "StateManager.h"
#include "SystemInitializer.h"
#include "IRQHandlers.h"
#include "HardwareManager.h"
#include "EngineMonitor.h"
#include "DisplayRenderer.h"
#include "ScreenRenderers.h"
#include "AudioManager.h"
#include "DataPersistence.h"
#include "PowerMonitor.h"
#include "FuelMonitor.h"
#include "BrightnessMonitor.h"
#include "ErrorCoordinator.h"
#include "ButtonController.h"
#include "CalculationEngine.h"
#include "DisplayStateMachine.h"
#include "ServiceModeManager.h"  

// Підключаємо наш "чарівний" сон
extern "C" {
    #include "DormantManager.h"
}

#if KLINE_ENABLE
#include <KLineKWP1281Lib.h>
#include "KLineManager.h"
KLineManager* kline_manager_ptr;
#endif

StateManager& state_manager = StateManager::getInstance();
SystemInitializer system_initializer(state_manager);

HardwareManager* hardware_manager_ptr;
AudioManager* audio_manager_ptr;
DataPersistence* data_persistence_ptr;
EngineMonitor* engine_monitor_ptr;
PowerMonitor* power_monitor_ptr;
FuelMonitor* fuel_monitor_ptr;
BrightnessMonitor* brightness_monitor_ptr;
ErrorCoordinator* error_coordinator_ptr;
ButtonController* button_controller_ptr;
CalculationEngine* calculation_engine_ptr;
DisplayRenderer* display_renderer_ptr;
DisplayStateMachine* display_state_machine_ptr;
ServiceModeManager* service_mode_manager_ptr = nullptr; 

void initialize_system(SystemMode mode) {
    log_init_debug("ІНІЦІАЛІЗАЦІЯ: Ініціалізація компонентів програми...");
    system_initializer.initialize_application_components(mode);

    hardware_manager_ptr = system_initializer.getHardwareManager();
    audio_manager_ptr = system_initializer.getAudioManager();
    data_persistence_ptr = system_initializer.getDataPersistence();
    engine_monitor_ptr = system_initializer.getEngineMonitor();
    power_monitor_ptr = system_initializer.getPowerMonitor();
    fuel_monitor_ptr = system_initializer.getFuelMonitor();
    brightness_monitor_ptr = system_initializer.getBrightnessMonitor();
    error_coordinator_ptr = system_initializer.getErrorCoordinator();
    button_controller_ptr = system_initializer.getButtonController();
    calculation_engine_ptr = system_initializer.getCalculationEngine();
    display_renderer_ptr = system_initializer.getDisplayRenderer();
    display_state_machine_ptr = system_initializer.getDisplayStateMachine();

    #if KLINE_ENABLE
    kline_manager_ptr = system_initializer.getKLineManager();
    if (!kline_manager_ptr) {
        log_init_debug("KLineManager не ініціалізовано.");
    }
    #endif

    log_init_debug("ІНІЦІАЛІЗАЦІЯ: Запуск налаштування системи під час виконання (включаючи ініціалізацію апаратних пінів)...");
    system_initializer.initialize_runtime_setup();

    if (hardware_manager_ptr) {
        service_mode_manager_ptr = new ServiceModeManager(state_manager, *hardware_manager_ptr, kline_manager_ptr);
    }

    setup_irq_handlers(hardware_manager_ptr->get_inj_pin_num(),
                       hardware_manager_ptr->get_vss_pin_num(),
                       hardware_manager_ptr->get_ignition_rpm_pin_num());

    display_state_machine_ptr->init_startup_state(millis());

    log_init_debug("ІНІЦІАЛІЗАЦІЯ: Ініціалізацію компонентів програми завершено.");
}

unsigned long last_update_ms = 0;
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000);
    log_init_debug("Запуск налаштування БК...");

    SystemMode initial_mode = NORMAL_MODE;

    #ifdef SERVICE_MODE_BUTTON_PIN
    pinMode(SERVICE_MODE_BUTTON_PIN, INPUT_PULLUP);
    delay(50);

    #ifdef ENABLE_SERVICE_MODE
    if (digitalRead(SERVICE_MODE_BUTTON_PIN) == LOW && ENABLE_SERVICE_MODE) {
        log_init_debug("Кнопку сервісного режиму утримано при запуску. Вхід у сервісний режим.");
        state_manager.is_service_mode = true;
        initial_mode = SERVICE_MODE;
    }
    #endif
    #endif

    initialize_system(initial_mode);

    if (state_manager.is_service_mode && service_mode_manager_ptr) {
        service_mode_manager_ptr->run();
        return;
    }

    last_update_ms = millis();
    log_init_debug("Налаштування БК ЗАВЕРШЕНО.");
}

void loop() {
    if (state_manager.is_service_mode && service_mode_manager_ptr) {
        service_mode_manager_ptr->run();
        return;
    }

    state_manager.current_time_ms = millis();
    if (state_manager.is_dormant_mode) {
        log_debug("Збереження статистики перед режимом сну...");
        state_manager.persistent_data_dirty = true;
        if (data_persistence_ptr) {
            data_persistence_ptr->save_persistent_data();
        }
        if (audio_manager_ptr) {
            audio_manager_ptr->mute_speaker();
        }

        hardware_manager_ptr->enterLowPowerMode();

        #ifdef ARDUINO_ARCH_RP2040
        watchdog_disable();
        #endif

        log_debug("Очікування ACC LOW перед Dormant...");
        while (hardware_manager_ptr->get_acc_status()) {
            delay(10);
        }

        log_debug("Вхід у DORMANT MODE.");
        sleep_run_from_xosc();                          
        sleep_goto_dormant_until_edge_high(PIN_ACC);    

        sleep_power_up(); 
        log_debug("Пробудження! DORMANT MODE перервано.");

        #ifdef ARDUINO_ARCH_RP2040
        watchdog_enable(WATCHDOG_TIME_RESET, 1);
        #endif
        hardware_manager_ptr->exitLowPowerMode();

        #ifdef SERVICE_MODE_BUTTON_PIN
        delay(50);
        if (digitalRead(SERVICE_MODE_BUTTON_PIN) == LOW && ENABLE_SERVICE_MODE) {
            log_init_debug("Кнопку сервісного режиму утримано після пробудження. Вхід у сервісний режим.");
            state_manager.is_service_mode = true;
            if (service_mode_manager_ptr) {
                service_mode_manager_ptr->run();
            }
            return;
        }
        #endif

        #if KLINE_ENABLE
        if (kline_manager_ptr) {
            kline_manager_ptr->resumeConnectionTask();
        }
        #endif

        if (state_manager.oled_driver_instance != nullptr) {
            static_cast<Adafruit_SH1107*>(state_manager.oled_driver_instance)->oled_command(SH110X_DISPLAYON);
            static_cast<Adafruit_SH1107*>(state_manager.oled_driver_instance)->setContrast(state_manager.current_oled_contrast);
        }
        state_manager.is_dormant_mode = false;
        state_manager.acc_present = true;
        state_manager.acc_lost_time_ms = 0;
        last_update_ms = millis();
    }
    hardware_manager_ptr->feed_wdt();

    #if KLINE_ENABLE
    if (kline_manager_ptr) {
        kline_manager_ptr->update(state_manager.current_time_ms);
    }
    #endif

    display_state_machine_ptr->update(state_manager.current_time_ms);

    if (audio_manager_ptr) {
        audio_manager_ptr->update_beep_manager();
    }

    if (button_controller_ptr) {
        button_controller_ptr->process_button_state(state_manager.current_time_ms);
    }

    #ifdef UPDATE_INTERVAL_MS
    if ((state_manager.current_time_ms - last_update_ms) >= UPDATE_INTERVAL_MS) {
        float actual_interval_sec = (float)(state_manager.current_time_ms - last_update_ms) / 1000.0f;
        int vss_pulses;
        unsigned long inj_total_raw_time_us;
        int last_inj_raw_duration;
        int last_ignition_period;
        unsigned long last_vss_activity_ms;
        read_irq_counters_atomic(&vss_pulses, &inj_total_raw_time_us, &last_inj_raw_duration, &last_ignition_period, &last_vss_activity_ms);

        if (engine_monitor_ptr) {
            engine_monitor_ptr->update(actual_interval_sec, last_inj_raw_duration, last_ignition_period, last_vss_activity_ms);
        }
        if (power_monitor_ptr) {
            power_monitor_ptr->update(actual_interval_sec);
        }
        if (fuel_monitor_ptr) {
            fuel_monitor_ptr->update(actual_interval_sec);
        }
        if (brightness_monitor_ptr) {
            brightness_monitor_ptr->update(state_manager.current_time_ms);
        }
        if (calculation_engine_ptr) {
            calculation_engine_ptr->process_periodic_updates(actual_interval_sec, vss_pulses, inj_total_raw_time_us);
        }
        if (data_persistence_ptr) {
            data_persistence_ptr->save_persistent_data();
        }
        last_update_ms = millis();
    }
    #endif

    display_renderer_ptr->render();

    #ifdef MAIN_LOOP_IDLE_SLEEP_MS
    delay(MAIN_LOOP_IDLE_SLEEP_MS);
    #endif
}