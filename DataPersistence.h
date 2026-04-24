// =====================================================================================================================
// --- Data Persistence (DataPersistence.h) ---
// =====================================================================================================================
#ifndef DATA_PERSISTENCE_H
#define DATA_PERSISTENCE_H

#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include <EEPROM.h>

class DataPersistence {
private:
    StateManager& _state;

    bool is_eeprom_initialized();
    void _initialize_default_data();

public:
    DataPersistence(StateManager& state_manager_instance);

    void load_persistent_data();
    void save_persistent_data();
    void reset_persistent_trip();
};

#endif // DATA_PERSISTENCE_H
