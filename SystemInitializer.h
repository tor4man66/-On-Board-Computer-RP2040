// =====================================================================================================================
// --- System Initializer (SystemInitializer.h) ---
// =====================================================================================================================
#ifndef SYSTEM_INITIALIZER_H
#define SYSTEM_INITIALIZER_H

#include "DebugUtils.h"
#include "StateManager.h"
#include "Config.h" 

class HardwareManager;
class AudioManager;
class DataPersistence;
class EngineMonitor;
class PowerMonitor;
class FuelMonitor;
class BrightnessMonitor;
class ErrorCoordinator;
class ButtonController;
class CalculationEngine;
class DisplayRenderer;
class DisplayStateMachine;
class KLineManager;

class SystemInitializer {
private:
    StateManager& state_manager;
    HardwareManager* hardware_manager;
    AudioManager* audio_manager;
    DataPersistence* data_persistence;
    EngineMonitor* engine_monitor;
    PowerMonitor* power_monitor;
    FuelMonitor* fuel_monitor;
    BrightnessMonitor* brightness_monitor;
    ErrorCoordinator* error_coordinator;
    ButtonController* button_controller;
    CalculationEngine* calculation_engine;
    DisplayRenderer* display_renderer;
    DisplayStateMachine* display_state_machine;
    KLineManager* kline_manager;
    
    bool _core_initialized = false;
    bool _components_initialized = false;
    
public:
    SystemInitializer(StateManager& sm);
    
    void initialize_application_components(SystemMode mode);
    void initialize_runtime_setup();
    
    HardwareManager* getHardwareManager();
    AudioManager* getAudioManager();
    DataPersistence* getDataPersistence();
    EngineMonitor* getEngineMonitor();
    PowerMonitor* getPowerMonitor();
    FuelMonitor* getFuelMonitor();
    BrightnessMonitor* getBrightnessMonitor();
    ErrorCoordinator* getErrorCoordinator();
    ButtonController* getButtonController();
    CalculationEngine* getCalculationEngine();
    DisplayRenderer* getDisplayRenderer();
    DisplayStateMachine* getDisplayStateMachine();
    KLineManager* getKLineManager();
};

#endif // SYSTEM_INITIALIZER_H