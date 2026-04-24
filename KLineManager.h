// =====================================================================================================================
// --- KLine Manager (KLineManager.h) ---
// =====================================================================================================================
#ifndef KLINE_MANAGER_H
#define KLINE_MANAGER_H

#include <Arduino.h>
#include <KLineKWP1281Lib.h>
#include "Config.h"
#include "DebugUtils.h"
#include "StateManager.h"
#include "HardwareManager.h"
#include "EngineMonitor.h" 

#include <FreeRTOS.h>
#include <task.h> 

#include "KLineTypes.h" 

extern volatile bool g_acc_status_kline;
void kline_begin_function(unsigned long baud);
void kline_end_function();
void kline_send_function(uint8_t data);
bool kline_receive_function(uint8_t *data);
#if KLINE_DEBUG_TRAFFIC_ENABLED
void kline_debug_function(bool direction, uint8_t message_sequence, uint8_t message_type, uint8_t *data, size_t length);
#endif

class KLineManager {
public:
    KLineManager(StateManager& state_manager_instance, HardwareManager& hardware_manager_instance, EngineMonitor* engine_monitor_ptr);
    
    bool clearFaults();
    
    uint8_t getFaultCount() const { return _current_fault_count; }
    uint8_t* getFaultCodes() { return _faults; }
    
    void init();
    void update(unsigned long current_time_ms);
    
    void suspendConnectionTask();
    void resumeConnectionTask();
    
    bool is_connected() const { return _currentKLineConnectionState == KLINE_CONNECTED; }
    bool has_dtcs() const { return _has_dtcs; }
    const char* get_dtc_error_message() const { return _dtc_error_message; }
    
    KLineConnectionState getConnectionState() const { return _currentKLineConnectionState; }
    
    void disconnectKLine();
    
    void run_service_mode_kline_cycle(unsigned long current_time_ms);
    
    KLineKWP1281Lib& getKLineDiag() { return _diag; }
    
private:
    StateManager& _state;
    HardwareManager& _hardware;
    EngineMonitor* _engine_monitor; 
    
    KLineKWP1281Lib& _diag;
    
    uint8_t _faults[3 * KLINE_DTC_BUFFER_MAX_FAULT_CODES];
    uint8_t _current_fault_count;
    
    TaskHandle_t _kLineConnectionTaskHandle = NULL;
    volatile KLineConnectionState _currentKLineConnectionState;
    volatile int _connection_retries_count;
    bool _has_processed_acc_on_start;
    
    unsigned long _last_check_light_toggle_time;
    bool _check_light_is_on;
    bool _has_dtcs;
    char _dtc_error_message[64];
    
    volatile bool _error_occurred_outside_task = false;
    static void _kLineErrorCallback(uint8_t module, unsigned long baud);
    void _handleKLineError();
    
    void _set_check_engine_light(bool state);
    bool _read_and_process_dtcs();
    void _manage_check_engine_light_based_on_conditions(unsigned long current_time_ms);
    
    static void kLineConnectionTask(void* parameter);
};

#endif // KLINE_MANAGER_H