#ifndef KLineKWP1281Lib_H
#define KLineKWP1281Lib_H

#define KWP1281_DEBUG_LEVEL 0

#define KWP1281_TEXT_TABLE_SUPPORTED
#define KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED
//#define KWP1281_OBD_FAULT_CODE_DESCRIPTION_SUPPORTED
#define KWP1281_FAULT_CODE_ELABORATION_SUPPORTED

#include <Arduino.h>
#include "units.h"
#include "decimals.h"
#include "keyed_struct.h"

#if KWP1281_DEBUG_LEVEL >= 1
#define K_LOG_ERROR(str, ...) if (_debug_port) _debug_port->print("E: " str, ##__VA_ARGS__)
#define K_LOG_ERROR_(str, ...) if (_debug_port) _debug_port->print(str, ##__VA_ARGS__)
#else
#define K_LOG_ERROR(str, ...)
#define K_LOG_ERROR_(str, ...)
#endif
#if KWP1281_DEBUG_LEVEL >= 2
#define K_LOG_WARNING(str, ...) if (_debug_port) _debug_port->print("W: " str, ##__VA_ARGS__)
#define K_LOG_WARNING_(str, ...) if (_debug_port) _debug_port->print(str, ##__VA_ARGS__)
#else
#define K_LOG_WARNING(str, ...)
#define K_LOG_WARNING_(str, ...)
#endif
#if KWP1281_DEBUG_LEVEL >= 3
#define K_LOG_INFO(str, ...) if (_debug_port) _debug_port->print("I: " str, ##__VA_ARGS__)
#define K_LOG_INFO_(str, ...) if (_debug_port) _debug_port->print(str, ##__VA_ARGS__)
#else
#define K_LOG_INFO(str, ...)
#define K_LOG_INFO_(str, ...)
#endif
#if KWP1281_DEBUG_LEVEL >= 4
#define K_LOG_DEBUG(str, ...) if (_debug_port) _debug_port->print("D: " str, ##__VA_ARGS__)
#define K_LOG_DEBUG_(str, ...) if (_debug_port) _debug_port->print(str, ##__VA_ARGS__)
#else
#define K_LOG_DEBUG(str, ...)
#define K_LOG_DEBUG_(str, ...)
#endif

#ifdef KWP1281_TEXT_TABLE_SUPPORTED
#include "text_table_EN.h"
#endif

#ifdef KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED
#include "fault_code_description_EN.h"
#endif

#ifdef KWP1281_OBD_FAULT_CODE_DESCRIPTION_SUPPORTED
#include "OBD_fault_code_description_EN.h"
#endif

#ifdef KWP1281_FAULT_CODE_ELABORATION_SUPPORTED
#include "fault_code_elaboration_EN.h"
#endif

#if defined(__AVR__)
#define READ_POINTER_FROM_PROGMEM pgm_read_word_near
#else
#define READ_POINTER_FROM_PROGMEM pgm_read_dword_near
#endif

#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

class KLineKWP1281Lib
{
public:
  using customErrorFunction_type     = void (*)(uint8_t module, unsigned long baud);
  using custom5baudWaitFunction_type = void (*)(void);
  using beginFunction_type           = void (*)(unsigned long baud);
  using endFunction_type             = void (*)(void);
  using sendFunction_type            = void (*)(uint8_t data);
  using receiveFunction_type         = bool (*)(uint8_t *data);
  using KWP1281debugFunction_type    = void (*)(bool direction, uint8_t message_sequence, uint8_t message_type, uint8_t *data, size_t length);

  enum executionStatus {
    FAIL,
    SUCCESS,
    ERROR,

    GROUP_BASIC_SETTINGS,
    GROUP_HEADER,
    GROUP_BODY,
    TIMEOUT_ERROR
  };

  enum measurementType {
    UNKNOWN,
    VALUE,
    TEXT
  };

  KLineKWP1281Lib(
    beginFunction_type beginFunction, endFunction_type endFunction,
    sendFunction_type sendFunction, receiveFunction_type receiveFunction,
    uint8_t tx_pin, bool full_duplex = true, Stream* debug_port = nullptr
  );

  unsigned long initComplementDelay = 40;
  unsigned long complementDelay = 2;
  unsigned long byteDelay = 2;
  unsigned long blockDelay = 10;

  unsigned long initResponseTimeout = 1000;
  unsigned long complementResponseTimeout = 50;
  unsigned long echoTimeout = 1000;
  unsigned long responseTimeout = 2000;
  unsigned long byteTimeout = 100;

  void KWP1281debugFunction(KWP1281debugFunction_type debug_function);

  void custom5baudWaitFunction(custom5baudWaitFunction_type function);
  void customErrorFunction(customErrorFunction_type function);

  executionStatus attemptConnect(uint8_t module, unsigned long baud_rate, bool request_extra_identification = true);
  executionStatus connect(uint8_t module, unsigned long baud_rate, bool request_extra_identification, unsigned long total_timeout_ms);
  void update();
  void disconnect(bool wait_for_response = true);

  char* getPartNumber();
  char* getIdentification();
  char* getExtraIdentification();
  uint16_t getCoding();
  uint32_t getWorkshopCode();

  executionStatus login(uint16_t login_code, uint32_t workshop_code);

  executionStatus recode(uint16_t coding, uint32_t workshop_code);

  executionStatus readFaults(uint8_t &amount_of_fault_codes, uint8_t* fault_code_buffer = nullptr, size_t fault_code_buffer_size = 0);
  static uint16_t getFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
  static bool isOBDFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
  static bool isOBDFaultCode(uint16_t fault_code);
  static char* getOBDFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size);
  static char* getOBDFaultCode(uint16_t fault_code, char* str, size_t string_size);
  static char* getFaultDescription(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size);
  static char* getFaultDescription(uint16_t fault_code, char* str, size_t string_size);
  static size_t getFaultDescriptionLength(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
  static size_t getFaultDescriptionLength(uint16_t fault_code);
  static uint8_t getFaultElaborationCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
  static char* getFaultElaboration(bool &is_intermittent, uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size, char* str, size_t string_size);
  static char* getFaultElaboration(bool &is_intermittent, uint8_t elaboration_code, char* str, size_t string_size);
  static size_t getFaultElaborationLength(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t* fault_code_buffer, size_t fault_code_buffer_size);
  static size_t getFaultElaborationLength(uint8_t elaboration_code);
  executionStatus clearFaults();

  executionStatus readAdaptation(uint8_t channel, uint16_t &value);
  executionStatus testAdaptation(uint8_t channel, uint16_t value);
  executionStatus adapt(uint8_t channel, uint16_t value, uint32_t workshop_code);

  executionStatus basicSetting(uint8_t &amount_of_values, uint8_t group, uint8_t* basic_setting_buffer = nullptr, size_t basic_setting_buffer_size = 0);
  static uint8_t getBasicSettingValue(uint8_t value_index, uint8_t amount_of_values, uint8_t* basic_setting_buffer, size_t basic_setting_buffer_size);

  executionStatus readGroup(uint8_t &amount_of_measurements, uint8_t group, uint8_t* measurement_buffer, size_t measurement_buffer_size);

  static uint8_t getFormula(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
  static uint8_t getNWb(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
  static uint8_t getMWb(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
  static uint8_t *getMeasurementData(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);
  static uint8_t getMeasurementDataLength(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size);

  static measurementType getMeasurementType(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
  static measurementType getMeasurementType(uint8_t formula);
  static double getMeasurementValue(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
  static double getMeasurementValue(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length);
  static double getMeasurementValue(uint8_t formula, uint8_t NWb, uint8_t MWb);
  static char* getMeasurementUnits(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size, char* str, size_t string_size);
  static char* getMeasurementUnits(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length, char* str, size_t string_size);
  static char* getMeasurementUnits(uint8_t formula, uint8_t NWb, uint8_t MWb, char* str, size_t string_size);
  static char* getMeasurementText(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size, char* str, size_t string_size);
  static char* getMeasurementText(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length, char* str, size_t string_size);
  static size_t getMeasurementTextLength(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
  static size_t getMeasurementTextLength(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length);
  static uint8_t getMeasurementDecimals(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t* measurement_buffer, size_t measurement_buffer_size);
  static uint8_t getMeasurementDecimals(uint8_t formula);

  static uint8_t getFormulaFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size);
  static uint8_t getNWbFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size);
  static uint8_t getMWbFromBody(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *body_buffer, size_t body_buffer_size);
  static uint8_t *getDataTableFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size);
  static uint8_t getDataTableLengthFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size);

  static measurementType getMeasurementTypeFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size);
  static measurementType getMeasurementTypeFromHeader(uint8_t formula);
  static double getMeasurementValueFromHeaderBody(uint8_t measurement_index, uint8_t amount_of_measurements_in_header, uint8_t *header_buffer, size_t header_buffer_size, uint8_t amount_of_measurements_in_body, uint8_t *body_buffer, size_t body_buffer_size);
  static double getMeasurementValueFromHeaderBody(uint8_t formula, uint8_t NWb, uint8_t MWb, uint8_t *data_table, uint8_t data_table_length);
  static char* getMeasurementUnitsFromHeaderBody(uint8_t measurement_index, uint8_t amount_of_measurements_in_header, uint8_t *header_buffer, size_t header_buffer_size, uint8_t amount_of_measurements_in_body, uint8_t *body_buffer, size_t body_buffer_size, char* str, size_t string_size);
  static char* getMeasurementUnitsFromHeaderBody(uint8_t formula, uint8_t NWb, uint8_t MWb, char* str, size_t string_size);
  static char* getMeasurementTextFromHeaderBody(uint8_t measurement_index, uint8_t amount_of_measurements_in_header, uint8_t *header_buffer, size_t header_buffer_size, uint8_t amount_of_measurements_in_body, uint8_t *body_buffer, size_t body_buffer_size, char* str, size_t string_size);
  static char* getMeasurementTextFromHeaderBody(uint8_t formula, uint8_t NWb, uint8_t MWb, uint8_t *data_table, uint8_t data_table_length, char* str, size_t string_size);
  static size_t getMeasurementTextLengthFromHeaderBody(uint8_t measurement_index, uint8_t amount_of_measurements_in_header, uint8_t *header_buffer, size_t header_buffer_size, uint8_t amount_of_measurements_in_body, uint8_t *body_buffer, size_t body_buffer_size);
  static size_t getMeasurementTextLengthFromHeaderBody(uint8_t formula, uint8_t NWb, uint8_t MWb, uint8_t *data_table, uint8_t data_table_length);
  static uint8_t getMeasurementDecimalsFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size);
  static uint8_t getMeasurementDecimalsFromHeader(uint8_t formula);

  executionStatus readROM(uint8_t chunk_size, uint16_t start_address, size_t &bytes_received, uint8_t* memory_buffer, uint8_t memory_buffer_size);

  executionStatus outputTests(uint16_t &current_output_test);
  static char* getOutputTestDescription(uint16_t output_test, char* str, size_t string_size);
  static size_t getOutputTestDescriptionLength(uint16_t output_test);

private:
  static const double negative_pow_10[8];

  static const uint8_t KWP_ACKNOWLEDGE             = 0x09;
  static const uint8_t KWP_REFUSE                  = 0x0A;
  static const uint8_t KWP_DISCONNECT              = 0x06;

  static const uint8_t KWP_REQUEST_EXTRA_ID        = 0x00;
  static const uint8_t KWP_REQUEST_LOGIN           = 0x2B;
  static const uint8_t KWP_REQUEST_RECODE          = 0x10;
  static const uint8_t KWP_REQUEST_FAULT_CODES     = 0x07;
  static const uint8_t KWP_REQUEST_CLEAR_FAULTS    = 0x05;
  static const uint8_t KWP_REQUEST_ADAPTATION      = 0x21;
  static const uint8_t KWP_REQUEST_ADAPTATION_TEST = 0x22;
  static const uint8_t KWP_REQUEST_ADAPTATION_SAVE = 0x2A;
  static const uint8_t KWP_REQUEST_GROUP_READING   = 0x29;
  static const uint8_t KWP_REQUEST_GROUP_READING_0 = 0x12;
  static const uint8_t KWP_REQUEST_READ_RAM        = 0x01;
  static const uint8_t KWP_REQUEST_READ_ROM        = 0x03;
  static const uint8_t KWP_REQUEST_READ_EEPROM     = 0x19;
  static const uint8_t KWP_REQUEST_OUTPUT_TEST     = 0x04;
  static const uint8_t KWP_REQUEST_BASIC_SETTING   = 0x28;
  static const uint8_t KWP_REQUEST_BASIC_SETTING_0 = 0x11;

  static const uint8_t KWP_RECEIVE_ID_DATA         = 0xF6;
  static const uint8_t KWP_RECEIVE_FAULT_CODES     = 0xFC;
  static const uint8_t KWP_RECEIVE_ADAPTATION      = 0xE6;
  static const uint8_t KWP_RECEIVE_GROUP_HEADER    = 0x02;
  static const uint8_t KWP_RECEIVE_GROUP_READING   = 0xE7;
  static const uint8_t KWP_RECEIVE_ROM             = 0xFD;
  static const uint8_t KWP_RECEIVE_OUTPUT_TEST     = 0xF5;
  static const uint8_t KWP_RECEIVE_BASIC_SETTING   = 0xF4;

  beginFunction_type        _beginFunction;
  endFunction_type          _endFunction;
  sendFunction_type         _sendFunction;
  receiveFunction_type      _receiveFunction;
  KWP1281debugFunction_type _debugFunction;

  uint8_t _tx_pin;

  bool _full_duplex;

  Stream* _debug_port;

  enum RETURN_TYPE {
    ERROR_TIMEOUT,
    ERROR_OK,
    TYPE_ID,
    TYPE_ACK,
    TYPE_REFUSE,
    TYPE_FAULT_CODES,
    TYPE_ADAPTATION,
    TYPE_GROUP_HEADER,
    TYPE_GROUP_READING,
    TYPE_ROM,
    TYPE_OUTPUT_TEST,
    TYPE_BASIC_SETTING
  };

  struct module_identification {
    char part_number[13];
    char identification[25];
    char extra_identification[37];
    uint32_t coding;
    uint32_t workshop_code;
  } identification_data;

  enum DEBUG_TYPE {
    UNEXPECTED_RESPONSE,
    SEND_ERROR,
    ARRAY_NOT_LARGE_ENOUGH,

    LOGIN_ACCEPTED,
    LOGIN_NOT_ACCEPTED,

    FAULT_CODES_NOT_SUPPORTED,
    CLEARING_FAULT_CODES_NOT_SUPPORTED,
    CLEARING_FAULT_CODES_ACCEPTED,

    INVALID_ADAPTATION_CHANNEL,
    INVALID_ADAPTATION_CHANNEL_OR_VALUE,
    ADAPTATION_RECEIVED,
    ADAPTATION_ACCEPTED,
    ADAPTATION_NOT_ACCEPTED,

    INVALID_BASIC_SETTING_GROUP,
    RECEIVED_EMPTY_BASIC_SETTING_GROUP,
    RECEIVED_BASIC_SETTING,

    INVALID_MEASUREMENT_GROUP,
    RECEIVED_EMPTY_GROUP,
    RECEIVED_GROUP_HEADER,
    INVALID_GROUP_HEADER_MAP_LENGTH,
    RECEIVED_GROUP_BODY_OR_BASIC_SETTING,
    INVALID_GROUP_BODY_OR_BASIC_SETTING_LENGTH,
    RECEIVED_GROUP,

    READ_ROM_NOT_SUPPORTED,
    RECEIVED_ROM,

    NO_PART_NUMBER_AVAILABLE,
    RECEIVED_PART_NUMBER,
    NO_ID_PART1_AVAILABLE,
    RECEIVED_ID_PART1,
    NO_ID_PART2_AVAILABLE,
    RECEIVED_ID_PART2,
    NO_CODING_WSC_AVAILABLE,
    RECEIVED_CODING_WSC,
    EXTRA_ID_AVAILABLE,
    NO_EXTRA_ID_AVAILABLE,
    NO_EXTRA_ID_PART1_AVAILABLE,
    RECEIVED_EXTRA_ID_PART1,
    NO_EXTRA_ID_PART2_AVAILABLE,
    RECEIVED_EXTRA_ID_PART2,
    NO_EXTRA_ID_PART3_AVAILABLE,
    RECEIVED_EXTRA_ID_PART3,

    OUTPUT_TESTS_NOT_SUPPORTED,
    RECEIVED_OUTPUT_TEST,
    END_OF_OUTPUT_TESTS,

    DISCONNECT_INFO
  };

  unsigned long _current_baud_rate;
  uint8_t _current_module;

  custom5baudWaitFunction_type _5baudWaitFunction;
  customErrorFunction_type _errorFunction;
  bool error_function_allowed = false;

  unsigned long last_byte_ms;

  uint8_t keyword_buffer[2];
  uint8_t receive_buffer[16];
  uint8_t receive_byte;

  bool may_send_protocol_parameters_again;

  uint8_t message_sequence;

  void bitbang_5baud(uint8_t module);

  executionStatus read_identification(bool request_extra_identification = true);

  bool receive_keywords(uint8_t *buffer);
  bool read_byte(uint8_t *byte_out, unsigned long timeout_ms, bool must_send_complement = true);
  void consume_UART_echo(uint8_t byte_in);
  void send_complement(uint8_t byte_in);
  RETURN_TYPE receive_message(size_t *bytes_received_out, uint8_t *buffer_out, size_t buffer_size, unsigned long timeout_ms);
  bool acknowledge();

  bool send_byte(uint8_t byte_in, bool wait_for_complement = true);
  bool send_message(uint8_t message_type, uint8_t *parameters = nullptr, size_t parameter_count = 0);

  void error_function();

  void show_debug_info(DEBUG_TYPE type, uint8_t parameter = 0);

  void show_debug_command_description(bool direction, uint8_t command);

  static uint8_t get_measurement_length(uint8_t *buffer, uint8_t bytes_received, uint8_t buffer_index);
  static double ToSigned(double MW);
  static double ToSigned(double NW, double MW);
  static double To16Bit(double NW, double MW);
  static int compare_keyed_structs(const void *a, const void *b);
};

#endif
