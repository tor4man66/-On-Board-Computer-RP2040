// =====================================================================================================================
// --- KLine Manager Implementation (KLineManager.cpp) ---
// =====================================================================================================================
#include "KLineManager.h"
#include "Config.h"
#include <KLineKWP1281Lib.h>
#include "DebugUtils.h"

volatile bool g_acc_status_kline = false;

#if KLINE_DEBUG_INFO_ENABLED
KLineKWP1281Lib KLineDiag(kline_begin_function, kline_end_function, kline_send_function, kline_receive_function, KLINE_TX_PIN, KLINE_IS_FULL_DUPLEX, &Serial);
#else
KLineKWP1281Lib KLineDiag(kline_begin_function, kline_end_function, kline_send_function, kline_receive_function, KLINE_TX_PIN, KLINE_IS_FULL_DUPLEX);
#endif

void kline_begin_function(unsigned long baud) {
    KLINE_SERIAL.setTX(KLINE_TX_PIN);
    KLINE_SERIAL.setRX(KLINE_RX_PIN);
    KLINE_SERIAL.begin(baud, KLINE_SERIAL_CONFIG);
    log_debug_str(String("K-Line: UART запущено на TX:") + KLINE_TX_PIN + " RX:" + KLINE_RX_PIN + " на " + baud + " бод.");
}

void kline_end_function() {
    KLINE_SERIAL.end();
    log_debug("K-Line: UART зупинено.");
}

void kline_send_function(uint8_t data) {
    if (!g_acc_status_kline) { 
        return;
    }
    KLINE_SERIAL.write(data);
}

bool kline_receive_function(uint8_t *data) {
    if (!g_acc_status_kline) {
        return false;
    }
    
    if (KLINE_SERIAL.available()) {
        *data = KLINE_SERIAL.read();
        return true;
    }
    return false;
}

#if KLINE_DEBUG_TRAFFIC_ENABLED
void kline_debug_function(bool direction, uint8_t message_sequence, uint8_t message_type, uint8_t *data, size_t length) {
    Serial.print(direction ? "[K-LINE RX] " : "[K-LINE TX] ");
    Serial.print("S:");
    if (message_sequence < 0x10) Serial.print(0);
    Serial.print(message_sequence, HEX);
    Serial.print(" T:");
    if (message_type < 0x10) Serial.print(0);
    Serial.print(message_type, HEX);
    Serial.print(" L:");
    Serial.print(length);
    if (length) {
        Serial.print(" D:");
        for (size_t i = 0; i < length; i++) {
            if (data[i] < 0x10) Serial.print(0);
            Serial.print(data[i], HEX);
            Serial.print(' ');
        }
    }
    Serial.println();
}
#endif

extern KLineManager* kline_manager_ptr;

KLineManager::KLineManager(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance, EngineMonitor* engine_monitor_ptr)
: _state(state_manager_instance),
_hardware(hardware_manager_instance),
_engine_monitor(engine_monitor_ptr), 
_diag(KLineDiag),
_currentKLineConnectionState(KLINE_DISCONNECTED),
_connection_retries_count(0),
_has_processed_acc_on_start(false),
_last_check_light_toggle_time(0),
_check_light_is_on(false),
_current_fault_count(0),
_has_dtcs(false)
{
    _dtc_error_message[0] = '\0';
    _diag.customErrorFunction(KLineManager::_kLineErrorCallback);
}

void KLineManager::_kLineErrorCallback(uint8_t module, unsigned long baud) {
    log_debug("K-Line: KLineKWP1281Lib ініціює зворотний виклик користувацької помилки (тайм-аут).");
    if (kline_manager_ptr) {
        kline_manager_ptr->_error_occurred_outside_task = true;
    }
}

void KLineManager::_handleKLineError() {
    if (_error_occurred_outside_task) {
        log_debug("K-Line: Обробка помилки K-Line, виявленої поза завданням. Примусове відключення та скидання стану.");
        _error_occurred_outside_task = false;
        
        if (_currentKLineConnectionState != KLINE_DISCONNECTED) {
            _diag.disconnect(false);
            _currentKLineConnectionState = KLINE_DISCONNECTED;
            _set_check_engine_light(true);
            strncpy(_dtc_error_message, "COMMS LOST", sizeof(_dtc_error_message) - 1);
            _dtc_error_message[sizeof(_dtc_error_message) - 1] = '\0';
        }
    }
}


void KLineManager::suspendConnectionTask() {
    if (_kLineConnectionTaskHandle != NULL) {
        log_debug("K-Line: Призупинення задачі з'єднання.");
        vTaskSuspend(_kLineConnectionTaskHandle);
    }
}

void KLineManager::resumeConnectionTask() {
    if (_kLineConnectionTaskHandle != NULL) {
        log_debug("K-Line: Відновлення задачі з'єднання.");
        vTaskResume(_kLineConnectionTaskHandle);
    }
}

void KLineManager::init() {
    if (_state.is_service_mode) { 
        log_init_debug("K-Line: Сервісний режим, FreeRTOS задача з'єднання НЕ СТВОРЮЄТЬСЯ.");
        _currentKLineConnectionState = KLINE_DISCONNECTED; 
        return;
    }
    
    if (!KLINE_ENABLE) {
        log_init_debug("K-Line: Функціонал вимкнено.");
        _currentKLineConnectionState = KLINE_PERMANENTLY_DISCONNECTED;
        _set_check_engine_light(false);
        return;
    }
    
    log_init_debug("K-Line: викликано, створюю задачу з'єднання.");
    
    xTaskCreate(
        KLineManager::kLineConnectionTask,
        "KLineConnTask",
        4096,
        this,
        tskIDLE_PRIORITY + 1,
        &_kLineConnectionTaskHandle
    );
    
    if (_kLineConnectionTaskHandle == NULL) {
        log_init_debug("K-Line: ПОМИЛКА створення задачі з'єднання.");
    } else {
        vTaskDelay(pdMS_TO_TICKS(1));
        log_init_debug("K-Line: Задача з'єднання успішно створена.");
        _currentKLineConnectionState = KLINE_CONNECTING;
    }
}

void KLineManager::kLineConnectionTask(void* parameter) {
    KLineManager* manager = static_cast<KLineManager*>(parameter);
    
    log_debug("K-Line (ЗАДАЧА): Задачу з'єднання запущено.");
    
    while (true) {
        manager->_hardware.feed_wdt();
        
        bool acc_on = manager->_hardware.get_acc_status();
        g_acc_status_kline = acc_on;
        
        if (acc_on && !manager->_has_processed_acc_on_start) {
            log_debug("K-Line (ЗАДАЧА): Новий цикл ACC ON виявлено. Скидання лічильника спроб.");
            manager->_connection_retries_count = 0;
            manager->_has_processed_acc_on_start = true;
            if (manager->_currentKLineConnectionState == KLINE_PERMANENTLY_DISCONNECTED) {
                manager->_currentKLineConnectionState = KLINE_DISCONNECTED;
            }
        }
        
        if (!KLINE_ENABLE || manager->_currentKLineConnectionState == KLINE_PERMANENTLY_DISCONNECTED) {
            log_debug("K-Line (ЗАДАЧА): Функціонал K-Line вимкнено або PERMANENTLY DISCONNECTED. Призупинення задачі.");
            manager->_has_processed_acc_on_start = false;
            kline_end_function();
            vTaskSuspend(NULL);
            continue;
        }
        
        if (!acc_on) {
            manager->_has_processed_acc_on_start = false;
            
            if (manager->_currentKLineConnectionState != KLINE_DISCONNECTED) {
                log_debug("K-Line (ЗАДАЧА): ACC вимкнено. Відключення від ЕБУ.");
                manager->_hardware.feed_wdt();
                manager->_diag.disconnect(false);
                manager->_hardware.feed_wdt();
                manager->_currentKLineConnectionState = KLINE_DISCONNECTED;
                manager->_set_check_engine_light(false);
            }
            log_debug("K-Line (ЗАДАЧА): ACC OFF. Призупинення задачі K-Line.");
            kline_end_function();
            vTaskSuspend(NULL);
            continue;
        }
        
        if (manager->_currentKLineConnectionState == KLINE_CONNECTED) {
            log_debug("K-Line (ЗАДАЧА): Вже підключено. Очікування наступного циклу перевірки ACC.");
            vTaskDelay(pdMS_TO_TICKS(KLINE_CONNECTED_TASK_IDLE_DELAY_MS));
        } else {
            log_debug_str(String("K-Line (ЗАДАЧА): ACC ON. Спроба підключення до ЕБУ (спроба ") + (manager->_connection_retries_count + 1) + " з " + KLINE_MAX_CONNECTION_RETRIES + ")...");
            manager->_currentKLineConnectionState = KLINE_CONNECTING;
            
            manager->_hardware.feed_wdt();
            KLineKWP1281Lib::executionStatus connection_status = manager->_diag.connect(KLINE_CONNECT_TO_MODULE, KLINE_MODULE_BAUD_RATE, false, KLINE_MAX_CONNECTION_TIMEOUT_MS);
            manager->_hardware.feed_wdt();
            
            acc_on = manager->_hardware.get_acc_status();
            g_acc_status_kline = acc_on;
            if (!acc_on) {
                log_debug("K-Line (ЗАДАЧА): ACC вимкнено під час/після Connect. Обробка і призупинення.");
                manager->_hardware.feed_wdt();
                manager->_diag.disconnect(false);
                manager->_hardware.feed_wdt();
                manager->_currentKLineConnectionState = KLINE_DISCONNECTED;
                manager->_set_check_engine_light(false);
                manager->_has_processed_acc_on_start = false;
                kline_end_function();
                vTaskSuspend(NULL);
                continue;
            }
            
            if (connection_status == KLineKWP1281Lib::SUCCESS) {
                uint8_t temp_fault_count = 0;
                uint8_t temp_faults[3 * KLINE_DTC_BUFFER_MAX_FAULT_CODES];
                
                manager->_hardware.feed_wdt();
                if (manager->_diag.readFaults(temp_fault_count, temp_faults, sizeof(temp_faults)) == KLineKWP1281Lib::SUCCESS) {
                    log_debug("K-Line (ЗАДАЧА): ЕБУ успішно підключено (підтверджено зчитуванням помилок).");
                    manager->_currentKLineConnectionState = KLINE_CONNECTED;
                    manager->_connection_retries_count = 0;
                    manager->_dtc_error_message[0] = '\0';
                    manager->_state.kline_last_update_time_ms = millis();
                    manager->_set_check_engine_light(false);
                } else {
                    log_debug("K-Line (ЗАДАЧА): ЕБУ підключено, але зчитування DTC не вдалося.");
                    manager->_connection_retries_count++;
                    if (manager->_connection_retries_count >= KLINE_MAX_CONNECTION_RETRIES) {
                        log_debug("K-Line (ЗАДАЧА): Досягнуто максимальної кількості спроб перепідключення. Перехід у PERMANENTLY DISCONNECTED.");
                        manager->_currentKLineConnectionState = KLINE_PERMANENTLY_DISCONNECTED;
                        strncpy(manager->_dtc_error_message, "NO CONNECTION", sizeof(manager->_dtc_error_message) - 1);
                        manager->_dtc_error_message[sizeof(manager->_dtc_error_message) - 1] = '\0';
                    } else {
                        manager->_currentKLineConnectionState = KLINE_DISCONNECTED;
                        strncpy(manager->_dtc_error_message, "RECONNECTING", sizeof(manager->_dtc_error_message) - 1);
                        manager->_dtc_error_message[sizeof(manager->_dtc_error_message) - 1] = '\0';
                    }
                    manager->_set_check_engine_light(true);
                }
                acc_on = manager->_hardware.get_acc_status();
                g_acc_status_kline = acc_on;
                if (!acc_on) {
                    log_debug("K-Line (ЗАДАЧА): ACC вимкнено під час/після readFaults(). Обробка і призупинення.");
                    manager->_hardware.feed_wdt();
                    manager->_diag.disconnect(false);
                    manager->_hardware.feed_wdt();
                    manager->_currentKLineConnectionState = KLINE_DISCONNECTED;
                    manager->_set_check_engine_light(false);
                    manager->_has_processed_acc_on_start = false;
                    kline_end_function();
                    vTaskSuspend(NULL);
                    continue;
                }
            } else {
                if (connection_status == KLineKWP1281Lib::TIMEOUT_ERROR) {
                    log_debug("K-Line (ЗАДАЧА): Підключення до ЕБУ за часом.");
                    strncpy(manager->_dtc_error_message, "TIMEOUT", sizeof(manager->_dtc_error_message) - 1);
                    manager->_dtc_error_message[sizeof(manager->_dtc_error_message) - 1] = '\0';
                } else {
                    log_debug("K-Line (ЗАДАЧА): Підключення до ЕБУ не вдалося (інші причини).");
                    strncpy(manager->_dtc_error_message, "NO CONNECTION", sizeof(manager->_dtc_error_message) - 1);
                    manager->_dtc_error_message[sizeof(manager->_dtc_error_message) - 1] = '\0';
                }
                
                manager->_hardware.feed_wdt();
                manager->_diag.disconnect(false);
                manager->_hardware.feed_wdt();
                manager->_set_check_engine_light(true);
                
                manager->_connection_retries_count++;
                if (manager->_connection_retries_count >= KLINE_MAX_CONNECTION_RETRIES) {
                    log_debug("K-Line (ЗАДАЧА): Досягнуто максимальної кількості спроб перепідключення. Перехід у PERMANENTLY DISCONNECTED.");
                    manager->_currentKLineConnectionState = KLINE_PERMANENTLY_DISCONNECTED;
                } else {
                    manager->_currentKLineConnectionState = KLINE_DISCONNECTED;
                }
            }
            
            if (manager->_hardware.get_acc_status()) {
                vTaskDelay(pdMS_TO_TICKS(KLINE_CONNECT_TASK_RETRY_DELAY_MS));
            } else {
                log_debug("K-Line (ЗАДАЧА): ACC OFF після спроби підключення. Пропускаємо затримку.");
            }
        }
    }
}

void KLineManager::disconnectKLine() {
    if (_currentKLineConnectionState != KLINE_DISCONNECTED) {
        log_debug("K-Line Manager: Явне відключення K-Line.");
        _diag.disconnect(false);
        _currentKLineConnectionState = KLINE_DISCONNECTED;
    }
}

void KLineManager::update(unsigned long current_time_ms) {
    if (!KLINE_ENABLE || _state.is_service_mode) return; 
    
    _handleKLineError();
    
    _manage_check_engine_light_based_on_conditions(current_time_ms);
    
    _state.kline_connection_status = _currentKLineConnectionState;
    if (_dtc_error_message[0] != '\0') {
        strncpy(_state.kline_error_message, _dtc_error_message, sizeof(_dtc_error_message) - 1);
        _state.kline_error_message[sizeof(_dtc_error_message) - 1] = '\0';
    } else {
        _state.kline_error_message[0] = '\0';
    }
    
    if (_currentKLineConnectionState == KLINE_CONNECTED) {
        _hardware.feed_wdt();
        if ((current_time_ms - _state.kline_last_update_time_ms) >= KLINE_UPDATE_INTERVAL_MS) {
            _state.kline_last_update_time_ms = current_time_ms;
            _has_dtcs = _read_and_process_dtcs();
        }
        _hardware.feed_wdt();
    }
}

void KLineManager::run_service_mode_kline_cycle(unsigned long current_time_ms) {
    _hardware.feed_wdt();
    bool acc_on = _hardware.get_acc_status();
    g_acc_status_kline = acc_on; 
    
    if (!acc_on && SERVICE_MODE_ACC_PIN_CHECK) {
        log_debug("K-Line (Сервісний Режим): ACC вимкнено. Відключення");
        if (_currentKLineConnectionState != KLINE_DISCONNECTED) {
            _diag.disconnect(false);
            _currentKLineConnectionState = KLINE_DISCONNECTED;
        }
        for (uint8_t i = 0; i < MAX_SERVICE_DISPLAY_LINES; ++i) {
            _state.service_display_lines[i].formatted_value = "ACC OFF";
            _state.service_display_lines[i].has_value = false;
        }
        return;
    }
    
    if (_currentKLineConnectionState != KLINE_CONNECTED) {
        log_debug("K-Line (Сервісний Режим): Спроба підключення до ЕБУ...");
        _state.kline_connection_status = KLINE_CONNECTING;
        
        KLineKWP1281Lib::executionStatus connection_status = _diag.connect(KLINE_CONNECT_TO_MODULE, KLINE_MODULE_BAUD_RATE, false, SERVICE_MODE_KLINE_CONNECT_TIMEOUT_MS);
        
        if (connection_status == KLineKWP1281Lib::SUCCESS) {
            log_debug("K-Line (Сервісний Режим): Успішно підключено до ЕБУ.");
            _currentKLineConnectionState = KLINE_CONNECTED;
            _state.kline_connection_status = KLINE_CONNECTED;
        } else {
            log_debug_str(String("K-Line (Сервісний Режим): НЕ ВДАЛОСЯ З'ЄДНАТИСЯ! Статус: ") + connection_status);
            _diag.disconnect(false);
            _currentKLineConnectionState = KLINE_DISCONNECTED;
            _state.kline_connection_status = KLINE_DISCONNECTED;
            for (uint8_t i = 0; i < MAX_SERVICE_DISPLAY_LINES; ++i) {
                _state.service_display_lines[i].formatted_value = "ERROR";
                _state.service_display_lines[i].has_value = false;
            }
            return; 
        }
    }
    
    if (_currentKLineConnectionState == KLINE_CONNECTED) {
        _diag.update();
        
        uint8_t measurement_buffer[80]; 
        uint8_t measurement_body_buffer[4]; 
        
        for (uint8_t line_idx = 0; line_idx < MAX_SERVICE_DISPLAY_LINES; ++line_idx) {
            uint8_t group_num = SERVICE_MEASUREMENT_MAP[line_idx][0];
            uint8_t param_idx = SERVICE_MEASUREMENT_MAP[line_idx][1];
            
            uint8_t amount_of_measurements = 0;
            bool received_group_header = false;
            uint8_t amount_of_measurements_in_header = 0;
            
            _state.service_display_lines[line_idx].has_value = false; 
            
            for (uint8_t attempt = 0; attempt < 1; attempt++) { 
                KLineKWP1281Lib::executionStatus readGroup_status;
                if (!received_group_header) {
                    readGroup_status = _diag.readGroup(amount_of_measurements, group_num, measurement_buffer, sizeof(measurement_buffer));
                } else {
                    readGroup_status = _diag.readGroup(amount_of_measurements, group_num, measurement_body_buffer, sizeof(measurement_body_buffer));
                }
                
                if (readGroup_status == KLineKWP1281Lib::GROUP_HEADER) {
                    received_group_header = true;
                    amount_of_measurements_in_header = amount_of_measurements;
                    attempt--; 
                    continue;
                } else if (readGroup_status == KLineKWP1281Lib::GROUP_BODY || readGroup_status == KLineKWP1281Lib::SUCCESS) {
                    if (param_idx < amount_of_measurements) {
                        KLineKWP1281Lib::measurementType measurement_type;
                        if (!received_group_header) {
                            measurement_type = KLineKWP1281Lib::getMeasurementType(param_idx, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer));
                        } else {
                            measurement_type = KLineKWP1281Lib::getMeasurementTypeFromHeader(param_idx, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer));
                        }
                        
                        if (measurement_type == KLineKWP1281Lib::VALUE) {
                            double value;
                            uint8_t decimals;
                            char units_string[16];
                            
                            if (!received_group_header) {
                                value = KLineKWP1281Lib::getMeasurementValue(param_idx, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer));
                                KLineKWP1281Lib::getMeasurementUnits(param_idx, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer), units_string, sizeof(units_string));
                                decimals = KLineKWP1281Lib::getMeasurementDecimals(param_idx, amount_of_measurements, measurement_buffer, sizeof(measurement_buffer));
                            } else {
                                value = KLineKWP1281Lib::getMeasurementValueFromHeaderBody(param_idx, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer), amount_of_measurements, measurement_body_buffer, sizeof(measurement_body_buffer));
                                KLineKWP1281Lib::getMeasurementUnitsFromHeaderBody(param_idx, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer), amount_of_measurements, measurement_body_buffer, sizeof(measurement_body_buffer), units_string, sizeof(units_string));
                                decimals = KLineKWP1281Lib::getMeasurementDecimalsFromHeader(param_idx, amount_of_measurements_in_header, measurement_buffer, sizeof(measurement_buffer));
                            }
                            
                            char value_str[32];
                            dtostrf(value, 1, decimals, value_str); // Форматуємо значення
                            _state.service_display_lines[line_idx].formatted_value = value_str;
                            _state.service_display_lines[line_idx].has_value = true;
                        } else {
                            _state.service_display_lines[line_idx].formatted_value = "TYPE ERR";
                        }
                    } else {
                        _state.service_display_lines[line_idx].formatted_value = "N/A";
                    }
                    break; 
                } else if (readGroup_status == KLineKWP1281Lib::FAIL || readGroup_status == KLineKWP1281Lib::ERROR) {
                    _state.service_display_lines[line_idx].formatted_value = "READ ERR";
                    _state.service_display_lines[line_idx].has_value = false;
                    break;
                }
            }
        }
    }
}


void KLineManager::_set_check_engine_light(bool state) {
    if (_check_light_is_on != state) {
        _check_light_is_on = state;
        _hardware.set_check_engine_light_state(_check_light_is_on);
        log_debug_str(String("K-Line: Індикатор Check Engine: ") + (_check_light_is_on ? "УВІМКНЕНО" : "ВИМКНЕНО"));
    }
}

bool KLineManager::_read_and_process_dtcs() {
    _current_fault_count = 0;
    
    _hardware.feed_wdt();
    if (_diag.readFaults(_current_fault_count, _faults, sizeof(_faults)) == KLineKWP1281Lib::SUCCESS) {
        _hardware.feed_wdt();
        log_debug_str(String("K-Line: Отримано DTC: ") + _current_fault_count);
        
        if (_current_fault_count > 0) {
            log_debug("K-Line: Знайдено коди помилок:");
            uint8_t available_fault_codes = _current_fault_count;
            if (sizeof(_faults) < (size_t)_current_fault_count * 3) {
                log_debug("K-Line: Буфер недостатній для всіх помилок.");
                available_fault_codes = sizeof(_faults) / 3;
            }
            for (uint8_t i = 0; i < available_fault_codes; i++) {
                uint16_t fault_code = KLineKWP1281Lib::getFaultCode(i, available_fault_codes, _faults, sizeof(_faults));
                char fault_code_string[8];
                sprintf(fault_code_string, "%05u", fault_code);
                char description_string[64];
                KLineKWP1281Lib::getFaultDescription(fault_code, description_string, sizeof(description_string));
                log_debug_str(String("  ") + fault_code_string + " - " + description_string);
            }
            strcpy(_dtc_error_message, "DTCs PRESENT");
            return true;
        } else {
            log_debug("K-Line: Коди помилок не знайдено.");
            _dtc_error_message[0] = '\0';
            return false;
        }
    } else {
        log_debug("K-Line: Помилка зчитування DTC. Обробка буде виконана централізовано через _handleKLineError.");
        strcpy(_dtc_error_message, "K-LINE ERROR: DTC READ FAIL");
        return false;
    }
}

void KLineManager::_manage_check_engine_light_based_on_conditions(unsigned long current_time_ms) {

    float engineRPM = _engine_monitor ? _engine_monitor->get_current_rpm() : 0.0f;
    
    switch (_currentKLineConnectionState) {
        case KLINE_CONNECTED:
            if (_hardware.get_acc_status()) {
                if (engineRPM > KLINE_MIN_RPM_FOR_ENGINE_RUNNING) {
                    if (_has_dtcs) {
                        _set_check_engine_light(true);
                    } else {
                        _set_check_engine_light(false);
                    }
                } else {
                    _set_check_engine_light(true);
                }
            } else {
                _set_check_engine_light(false);
            }
            break;
            
        case KLINE_DISCONNECTED:
            if (_hardware.get_acc_status()) {
                _set_check_engine_light(true);
            } else {
                _set_check_engine_light(false);
            }
            break;
            
        case KLINE_CONNECTING:
            if (_hardware.get_acc_status()) {
                if (_check_light_is_on && (current_time_ms - _last_check_light_toggle_time >= KLINE_BLINK_ON_DURATION_MS)) {
                    _set_check_engine_light(false);
                    _last_check_light_toggle_time = current_time_ms;
                } else if (!_check_light_is_on && (current_time_ms - _last_check_light_toggle_time >= KLINE_BLINK_OFF_DURATION_MS)) {
                    _set_check_engine_light(true);
                    _last_check_light_toggle_time = current_time_ms;
                }
            } else {
                _set_check_engine_light(false);
            }
            break;
            
        case KLINE_PERMANENTLY_DISCONNECTED:
            if (_hardware.get_acc_status()) {
                if (_check_light_is_on && (current_time_ms - _last_check_light_toggle_time >= KLINE_PERM_DISCONNECTED_BLINK_ON_DURATION_MS)) {
                    _set_check_engine_light(false);
                    _last_check_light_toggle_time = current_time_ms;
                } else if (!_check_light_is_on && (current_time_ms - _last_check_light_toggle_time >= KLINE_PERM_DISCONNECTED_BLINK_OFF_DURATION_MS)) {
                    _set_check_engine_light(true);
                    _last_check_light_toggle_time = current_time_ms;
                }
            } else {
                _set_check_engine_light(false);
            }
            break;
    }
}

bool KLineManager::clearFaults() {
    if (_currentKLineConnectionState != KLINE_CONNECTED) {
        log_debug("K-Line: Спроба очистити помилки: Немає з'єднання з ЕБУ.");
        return false;
    }
    
    log_debug("K-Line: Спроба очищення кодів помилок...");
    _hardware.feed_wdt();
    
    _diag.clearFaults();
    
    log_debug("K-Line: Команда очищення відправлена.");
    _hardware.feed_wdt();
    
    _has_dtcs = false;
    _dtc_error_message[0] = '\0';
    _current_fault_count = 0;
    
    _diag.disconnect(false);
    _currentKLineConnectionState = KLINE_DISCONNECTED;
    _connection_retries_count = 0;
    
    _state.kline_last_update_time_ms = millis();
    
    return true;
}