// =====================================================================================================================
// --- Data Persistence Implementation (DataPersistence.cpp) ---
// =====================================================================================================================
#include "DataPersistence.h"
#include "Config.h"
#include "DebugUtils.h"

DataPersistence::DataPersistence(StateManager& state_manager_instance) : _state(state_manager_instance) {
    log_init_debug("Ініціалізація DataPersistence...");
    EEPROM.begin(EEPROM_SIZE);
}

bool DataPersistence::is_eeprom_initialized() {
    unsigned long magic;
    EEPROM.get(EEPROM_ADDR_MAGIC, magic);
    return magic == EEPROM_MAGIC_NUMBER;
}

void DataPersistence::_initialize_default_data() {
    _state.set_persistent_trip_fuel_L(0.0f);
    _state.set_persistent_trip_distance_km(0.0f);
    _state.set_trip_fuel_consumed_L(0.0f);
    _state.set_trip_distance_travelled_km(0.0f);
    _state.set_current_main_display_mode(DEFAULT_MAIN_DISPLAY_MODE);
    _state.persistent_data_dirty = true;
    log_init_debug("  Дані EEPROM ініціалізовано значеннями за замовчуванням.");
}

void DataPersistence::load_persistent_data() {
    log_init_debug("Завантаження персистентних даних з EEPROM...");
    if (is_eeprom_initialized()) {
        EEPROM.get(EEPROM_ADDR_TRIP_FUEL, _state._trip_fuel_consumed_L);
        EEPROM.get(EEPROM_ADDR_TRIP_DISTANCE, _state._trip_distance_travelled_km);
        EEPROM.get(EEPROM_ADDR_PERS_FUEL, _state._persistent_trip_fuel_L);
        EEPROM.get(EEPROM_ADDR_PERS_DISTANCE, _state._persistent_trip_distance_km);
        EEPROM.get(EEPROM_ADDR_DISPLAY_MODE, _state._current_main_display_mode);
        log_init_debug("  Дані успішно завантажено з EEPROM.");
    } else {
        _initialize_default_data();
        log_init_debug("  EEPROM не ініціалізовано або пошкоджено. Записано значення за замовчуванням.");
        save_persistent_data();
    }
    _state.persistent_data_dirty = false;
}

void DataPersistence::save_persistent_data() {
    unsigned long now = _state.current_time_ms;
    if (_state.persistent_data_dirty && (now - _state.last_persistent_save_time_ms) >= PERSISTENT_SAVE_INTERVAL_MS) {
        log_debug("Збереження персистентних даних в EEPROM розпочато...");
        EEPROM.put(EEPROM_ADDR_MAGIC, EEPROM_MAGIC_NUMBER);
        EEPROM.put(EEPROM_ADDR_TRIP_FUEL, _state._trip_fuel_consumed_L);
        EEPROM.put(EEPROM_ADDR_TRIP_DISTANCE, _state._trip_distance_travelled_km);
        EEPROM.put(EEPROM_ADDR_PERS_FUEL, _state._persistent_trip_fuel_L);
        EEPROM.put(EEPROM_ADDR_PERS_DISTANCE, _state._persistent_trip_distance_km);
        EEPROM.put(EEPROM_ADDR_DISPLAY_MODE, _state._current_main_display_mode);
        EEPROM.commit();
        _state.last_persistent_save_time_ms = now;
        _state.persistent_data_dirty = false;
        log_debug("Персистентні дані успішно збережено в EEPROM.");
    }
}

void DataPersistence::reset_persistent_trip() {
    if (_state.get_persistent_trip_distance_km() >= RESET_PERSISTENT_TRIP_DISTANCE_KM) {
        if (_state.get_persistent_trip_distance_km() > 0) {
            log_debug("Автоматичне скидання PERS лічильників.");
            _state.set_persistent_trip_fuel_L(0.0f);
            _state.set_persistent_trip_distance_km(0.0f);
            _state.persistent_data_dirty = true;
        }
    }
}
