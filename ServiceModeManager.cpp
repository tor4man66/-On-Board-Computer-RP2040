// =====================================================================================================================
// --- Service Mode Manager Implementation (ServiceModeManager.cpp) ---
// =====================================================================================================================
#include "ServiceModeManager.h"
#include "ScreenRenderers.h"
#include "DebugUtils.h"

extern volatile bool g_acc_status_kline;

ServiceModeManager::ServiceModeManager(StateManager& state, HardwareManager& hardware, KLineManager* kline)
: _state(state), _hardware(hardware), _kline(kline), _oled(nullptr) {}

void ServiceModeManager::run() {
    log_init_debug("Вхід у сервісний режим.");
    Serial.println("Service Mode: Entering service mode.");

    if (_state.oled_driver_instance == nullptr) {
        log_init_debug("ПОМИЛКА: OLED не ініціалізовано в сервісному режимі.");
        Serial.println("ERROR: OLED not initialized in service mode.");
        while (true) { delay(1000); }
    }
    _oled = static_cast<Adafruit_SH1107*>(_state.oled_driver_instance);

    _initDisplay();

    _fillCustomTexts();

    int current_acc_pin = _hardware.get_acc_pin_num();
    if (current_acc_pin != -1) {
        pinMode(current_acc_pin, INPUT_PULLDOWN);
    }

    while (true) {
        _hardware.feed_wdt(); 

        unsigned long current_time = millis();
        bool acc_on = _hardware.get_acc_status();
        g_acc_status_kline = acc_on;

        if (!acc_on) {
            _cleanupAndExit();
            return;
        }

        _updateKLine(current_time);

        // Малювання екрану
        _drawScreen();

        delay(100);
    }
}

void ServiceModeManager::_initDisplay() {
    _oled->oled_command(SH110X_DISPLAYON);
    _oled->setContrast(_state.current_oled_contrast);
    delay(100);

    _oled->clearDisplay();
    _oled->setTextColor(SH110X_WHITE);
    _oled->setFont(SERVICE_KLINE_TEXT_FONT);
    
    if (_kline) {
        stretched_text(_oled, SERVICE_KLINE_CONNECTED_TEXT,
                       SERVICE_KLINE_CONNECTED_X_POS, SERVICE_KLINE_CONNECTED_Y_POS,
                       SERVICE_KLINE_CONNECTED_FONT_SIZE_X, SERVICE_KLINE_CONNECTED_FONT_SIZE_Y, 1);
    } else {
        stretched_text(_oled, SERVICE_KLINE_DISABLED_TEXT,
                       SERVICE_KLINE_DISABLED_X_POS, SERVICE_KLINE_DISABLED_Y_POS,
                       SERVICE_KLINE_DISABLED_FONT_SIZE_X, SERVICE_KLINE_DISABLED_FONT_SIZE_Y, 1);
    }
    
    _oled->setFont(NULL);
    _oled->display();
    delay(1000);
}

void ServiceModeManager::_fillCustomTexts() {
    #ifdef SERVICE_LINE_1_CUSTOM_TEXT
    _state.service_display_lines[0].custom_text = SERVICE_LINE_1_CUSTOM_TEXT;
    #endif
    #ifdef SERVICE_LINE_2_CUSTOM_TEXT
    _state.service_display_lines[1].custom_text = SERVICE_LINE_2_CUSTOM_TEXT;
    #endif
    #ifdef SERVICE_LINE_3_CUSTOM_TEXT
    _state.service_display_lines[2].custom_text = SERVICE_LINE_3_CUSTOM_TEXT;
    #endif
    #ifdef SERVICE_LINE_4_CUSTOM_TEXT
    _state.service_display_lines[3].custom_text = SERVICE_LINE_4_CUSTOM_TEXT;
    #endif
    #ifdef SERVICE_LINE_5_CUSTOM_TEXT
    _state.service_display_lines[4].custom_text = SERVICE_LINE_5_CUSTOM_TEXT;
    #endif
}

void ServiceModeManager::_updateKLine(unsigned long currentTime) {
    #ifdef SERVICE_MODE_KLINE_READ_INTERVAL_MS
    if (_kline && (currentTime - _state.last_service_mode_kline_update_ms) >= SERVICE_MODE_KLINE_READ_INTERVAL_MS) {
        _kline->run_service_mode_kline_cycle(currentTime);
        _state.last_service_mode_kline_update_ms = currentTime;
    }
    #endif
}

void ServiceModeManager::_drawScreen() {
    ServiceModeDisplayData data = prepare_service_mode_data(_state);
    _oled->clearDisplay();
    render_service_mode_screen(_oled, data);
    _oled->display();
}

void ServiceModeManager::_cleanupAndExit() {
    log_init_debug("Сервісний режим: ACC вимкнено. Вихід.");
    Serial.println("Service Mode: ACC OFF. Exiting.");

    _oled->clearDisplay();
    _oled->display();

    _state.is_service_mode = false;

    if (_kline && _kline->getConnectionState() == KLINE_CONNECTED) {
        _kline->disconnectKLine();
    }
}
