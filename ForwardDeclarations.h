// =====================================================================================================================
// --- Forward Declarations (ForwardDeclarations.h) ---
// =====================================================================================================================
#ifndef FORWARD_DECLARATIONS_H
#define FORWARD_DECLARATIONS_H

#include <cstdint>
#include <Adafruit_GFX.h>
#include "StateManager.h"

class StateManager;
class HardwareManager;
class AudioManager;
class DataPersistence;
class FuelMonitor;
class EngineMonitor;
class PowerMonitor;
class BrightnessMonitor;
class ErrorCoordinator;
class ButtonController;
class DisplayRenderer;
class DisplayStateMachine;

struct Mode0DisplayData;
struct Mode1DisplayData;
struct Mode2DisplayData;
struct ServiceModeDisplayData;

ErrorIconType check_brake_fluid_error(HardwareManager& hardware_manager);
ErrorIconType check_coolant_error(HardwareManager& hardware_manager);
ErrorIconType check_oil_pressure_error(HardwareManager& hardware_manager, bool is_engine_running_stable,
                                       unsigned long engine_start_time_ms, unsigned long current_time_ms, float current_rpm);
ErrorIconType check_charging_error(bool is_engine_running_stable, float current_battery_voltage);
bool check_towing_mode_status(unsigned long last_vss_activity_ms, float current_rpm, bool is_engine_running, unsigned long current_time_ms);

void stretched_text(Adafruit_GFX* display, const char* text, int16_t x, int16_t y, uint8_t size_x, uint8_t size_y, uint16_t color);
void large_text(Adafruit_GFX* display, const char* text, int16_t x, int16_t y, uint8_t size, uint16_t color);
void round_rect(Adafruit_GFX* display, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);

#endif // FORWARD_DECLARATIONS_H