// =====================================================================================================================
// --- Service Mode Manager (ServiceModeManager.h) ---
// =====================================================================================================================
#ifndef SERVICE_MODE_MANAGER_H
#define SERVICE_MODE_MANAGER_H

#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include "Config.h"
#include "StateManager.h"
#include "HardwareManager.h"
#include "KLineManager.h"

class ServiceModeManager {
public:
    ServiceModeManager(StateManager& state, HardwareManager& hardware, KLineManager* kline);

    void run();

private:
    StateManager& _state;
    HardwareManager& _hardware;
    KLineManager* _kline;
    Adafruit_SH1107* _oled;

    void _initDisplay();

    void _fillCustomTexts();

    void _updateKLine(unsigned long currentTime);

    void _drawScreen();

    void _cleanupAndExit();
};

#endif // SERVICE_MODE_MANAGER_H
