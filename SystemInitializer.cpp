// =====================================================================================================================
// --- System Initializer Implementation (SystemInitializer.cpp) ---
// =====================================================================================================================
#include "SystemInitializer.h"
#include "DebugUtils.h"
#include "Config.h"

#include "HardwareManager.h"
#include "AudioManager.h"
#include "DataPersistence.h"
#include "EngineMonitor.h"
#include "PowerMonitor.h"
#include "FuelMonitor.h"
#include "BrightnessMonitor.h"
#include "ErrorCoordinator.h"
#include "ButtonController.h"
#include "CalculationEngine.h"
#include "DisplayRenderer.h"
#include "DisplayStateMachine.h"
#include "KLineManager.h" 

SystemInitializer::SystemInitializer(StateManager& sm) : state_manager(sm) {
    log_init_debug("Ініціалізація SystemInitializer (основний модуль)...");
    state_manager.error_logger = &project_error_logger;
    log_init_debug("ErrorLogger ініціалізовано та призначено для StateManager.");
    
    hardware_manager = nullptr;
    audio_manager = nullptr;
    data_persistence = nullptr;
    engine_monitor = nullptr;
    power_monitor = nullptr;
    fuel_monitor = nullptr;
    brightness_monitor = nullptr;
    error_coordinator = nullptr;
    button_controller = nullptr;
    calculation_engine = nullptr;
    display_renderer = nullptr;
    display_state_machine = nullptr;
    kline_manager = nullptr;

    _core_initialized = true;
    log_init_debug("SystemInitializer (основний модуль) ініціалізовано.");
}

void SystemInitializer::initialize_application_components(SystemMode mode) {
    if (_components_initialized) return;
    log_init_debug("Розпочато ініціалізацію компонентів програми.");
    
    hardware_manager = new HardwareManager(state_manager);
    audio_manager = new AudioManager(state_manager, *hardware_manager);
    data_persistence = new DataPersistence(state_manager);
    brightness_monitor = new BrightnessMonitor(state_manager, *hardware_manager);
    button_controller = new ButtonController(state_manager, *audio_manager, *hardware_manager);

    if (mode == NORMAL_MODE) {
        log_init_debug("SystemMode: NORMAL MODE. Ініціалізація всіх моніторів та движка розрахунків.");
        engine_monitor = new EngineMonitor(state_manager, *hardware_manager);
        power_monitor = new PowerMonitor(state_manager, *hardware_manager);
        fuel_monitor = new FuelMonitor(state_manager, *hardware_manager);
        error_coordinator = new ErrorCoordinator(state_manager, *hardware_manager, *fuel_monitor, *engine_monitor);
        calculation_engine = new CalculationEngine(state_manager, *hardware_manager, *fuel_monitor, *data_persistence);
    } else { 
        log_init_debug("SystemMode: SERVICE MODE. Ініціалізація мінімальних компонентів.");
        engine_monitor = new EngineMonitor(state_manager, *hardware_manager);
        power_monitor = nullptr;
        fuel_monitor = new FuelMonitor(state_manager, *hardware_manager);
        error_coordinator = new ErrorCoordinator(state_manager, *hardware_manager, *fuel_monitor, *engine_monitor);
        calculation_engine = nullptr;
    }
    
    #if KLINE_ENABLE
    log_init_debug("Ініціалізація KLineManager...");
    kline_manager = new KLineManager(state_manager, *hardware_manager, engine_monitor);
    #else
    log_init_debug("SystemInitializer: KLINE ENABLE встановлено в false, KLineManager буде nullptr.");
    kline_manager = nullptr;
    #endif
    
    display_renderer = new DisplayRenderer(state_manager, *hardware_manager, fuel_monitor, engine_monitor, kline_manager);
    display_state_machine = new DisplayStateMachine(state_manager, *error_coordinator, *audio_manager, *display_renderer);

    _components_initialized = true;
    log_init_debug("Ініціалізацію компонентів програми завершено.");
}

void SystemInitializer::initialize_runtime_setup() {
    if (!_components_initialized) {
        state_manager.error_logger->log_error("Помилка: Initialize Application Components не викликано перед Initialize Runtime Setup.");
        log_init_debug("Помилка: Initialize Application Components не викликано перед Initialize Runtime Setup.");
        return;
    }
    log_init_debug("Запуск налаштування системи під час виконання...");
    
    hardware_manager->init_all_hardware();
    log_init_debug("Апаратне забезпечення ініціалізовано.");
    
    if (display_renderer) { 
        display_renderer->init();
    } else {
        state_manager.error_logger->log_error("Помилка: Display Renderer не ініціалізовано перед Display Renderer->Init.");
        log_init_debug("Помилка: Display Renderer не ініціалізовано перед Display Renderer->Init.");
    }

    #if KLINE_ENABLE
    if (kline_manager) {
        log_init_debug("KLine: викликаю Init для запуску задачі з'єднання."); 
        kline_manager->init(); 
        log_init_debug("KLineManager Task ініціалізовано.");
    } else {
        log_init_debug("KLineManager не було створено (можливо, KLINE ENABLE вимкнено).");
    }
    #endif

    data_persistence->load_persistent_data();
    log_init_debug("Збережені дані завантажено.");
    
    if (fuel_monitor && ENABLE_PIN_FUEL_LEVEL_ADC) {
        if (state_manager.oled_status == "OK" && state_manager.oled_driver_instance) {
            float initial_fuel_percent_raw = fuel_monitor->get_raw_fuel_percent();
            float initial_fuel_buffer_value;
            if (initial_fuel_percent_raw < 0) {
                initial_fuel_buffer_value = 0.0f;
                state_manager.last_smoothed_fuel_percent = 0.0f;
            } else {
                char fuel_display_value[8];
                sprintf(fuel_display_value, "%.1f%%", initial_fuel_percent_raw);
                initial_fuel_buffer_value = initial_fuel_percent_raw;
                state_manager.last_smoothed_fuel_percent = initial_fuel_percent_raw;
                log_init_debug_str(String("Початковий рівень палива: ") + fuel_display_value + ".");
            }
            fuel_monitor->initialize_fuel_buffer(initial_fuel_buffer_value);
        } else {
             log_init_debug("OLED або драйвер OLED не ініціалізовано, пропущено попереднє заповнення буферів рівня палива для OLED.");
        }
    } else {
        log_init_debug("Датчик палива не ініціалізовано або вимкнено, пропущено попереднє заповнення буферів рівня палива.");
    }
    
    log_init_debug_str(String("Початковий режим дисплея: ") + state_manager.current_display_mode + ".");
    log_init_debug("Налаштування системи під час виконання - ЗАВЕРШЕНО.");
}

HardwareManager* SystemInitializer::getHardwareManager() { return hardware_manager; }
AudioManager* SystemInitializer::getAudioManager() { return audio_manager; }
DataPersistence* SystemInitializer::getDataPersistence() { return data_persistence; }
EngineMonitor* SystemInitializer::getEngineMonitor() { return engine_monitor; }
PowerMonitor* SystemInitializer::getPowerMonitor() { return power_monitor; }
FuelMonitor* SystemInitializer::getFuelMonitor() { return fuel_monitor; }
BrightnessMonitor* SystemInitializer::getBrightnessMonitor() { return brightness_monitor; }
ErrorCoordinator* SystemInitializer::getErrorCoordinator() { return error_coordinator; }
ButtonController* SystemInitializer::getButtonController() { return button_controller; }
CalculationEngine* SystemInitializer::getCalculationEngine() { return calculation_engine; }
DisplayRenderer* SystemInitializer::getDisplayRenderer() { return display_renderer; }
DisplayStateMachine* SystemInitializer::getDisplayStateMachine() { return display_state_machine; }
KLineManager* SystemInitializer::getKLineManager() {
    return kline_manager;
}