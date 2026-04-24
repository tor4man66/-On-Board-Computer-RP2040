#include "KLineKWP1281Lib.h"
#include "FreeRTOS.h"
#include "task.h"
#ifdef ARDUINO_ARCH_RP2040
#include "hardware/watchdog.h"
#endif

KLineKWP1281Lib::KLineKWP1281Lib(beginFunction_type beginFunction, endFunction_type endFunction, sendFunction_type sendFunction, receiveFunction_type receiveFunction, uint8_t tx_pin, bool full_duplex, Stream* debug_port) :
_beginFunction(beginFunction),
_endFunction(endFunction),
_sendFunction(sendFunction),
_receiveFunction(receiveFunction),
_tx_pin(tx_pin),
_full_duplex(full_duplex),
_debug_port(debug_port)
{}

void KLineKWP1281Lib::KWP1281debugFunction(KWP1281debugFunction_type debug_function)
{
  _debugFunction = debug_function;
}

void KLineKWP1281Lib::custom5baudWaitFunction(custom5baudWaitFunction_type function)
{
  _5baudWaitFunction = function;
}

void KLineKWP1281Lib::customErrorFunction(customErrorFunction_type function)
{
  _errorFunction = function;
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::attemptConnect(uint8_t module, unsigned long baud_rate, bool request_extra_identification)
{
  error_function_allowed = false;

  if (_current_module)
  {
    disconnect();
  }

  K_LOG_INFO("Connecting to module ");
  K_LOG_INFO_(module >> 4, HEX);
  K_LOG_INFO_(module & 0xF, HEX);
  K_LOG_INFO_("\n");

  #ifdef ARDUINO_ARCH_RP2040
  watchdog_update();
  #endif
  bitbang_5baud(module);

  #ifdef ARDUINO_ARCH_RP2040
  watchdog_update();
  #endif
  _beginFunction(baud_rate);

  #ifdef ARDUINO_ARCH_RP2040
  watchdog_update();
  #endif

  if (!read_byte(&receive_byte, initResponseTimeout, false))
  {
    K_LOG_ERROR("Error reading SYNC byte; check interface/baud rate\n");
    return ERROR;
  }

  #ifdef ARDUINO_ARCH_RP2040
  watchdog_update();
  #endif

  if (receive_byte == 0x55)
  {
    K_LOG_INFO("Got SYNC byte\n");

    if (!receive_keywords(keyword_buffer))
    {
      K_LOG_ERROR("Error reading KW1+KW2; check interface/baud rate\n");
      return ERROR;
    }

    vTaskDelay(pdMS_TO_TICKS(initComplementDelay));

    #ifdef ARDUINO_ARCH_RP2040
    watchdog_update();
    #endif

    send_complement(keyword_buffer[1]);
    K_LOG_INFO("Connected to module ");
    K_LOG_INFO_(module >> 4, HEX);
    K_LOG_INFO_(module & 0xF, HEX);
    K_LOG_INFO_("\n");

    uint16_t protocol = ((keyword_buffer[1] & 0x7F) << 7) | keyword_buffer[0];
    K_LOG_INFO("Protocol: ");
    K_LOG_INFO_(protocol);
    K_LOG_INFO_("\n");

    if (protocol != 1281)
    {
      K_LOG_WARNING("Protocol may not be supported\n");
    }

    _current_baud_rate = baud_rate;
    _current_module = module;

    may_send_protocol_parameters_again = true;

    #ifdef ARDUINO_ARCH_RP2040
    watchdog_update();
    #endif

    executionStatus status = read_identification(request_extra_identification);

    error_function_allowed = true;

    return status;
  }
  else
  {
    K_LOG_ERROR("Incorrect SYNC byte; check baud rate\n");
    return ERROR;
  }
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::connect(uint8_t module, unsigned long baud_rate, bool request_extra_identification, unsigned long total_timeout_ms)
{
  unsigned long start_time = millis();
  KLineKWP1281Lib::executionStatus status = ERROR;

  while (status != SUCCESS)
  {
    if (total_timeout_ms > 0 && (millis() - start_time) >= total_timeout_ms)
    {
      K_LOG_ERROR("K-Line connection timed out after " + String(total_timeout_ms) + "ms.\n");
      return TIMEOUT_ERROR;
    }

    status = attemptConnect(module, baud_rate, request_extra_identification);

    if (status != SUCCESS)
    {
      K_LOG_INFO("K-Line connection attempt failed. Retrying in 1s...\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
      #ifdef ARDUINO_ARCH_RP2040
      watchdog_update();
      #endif
    }
  }
  return SUCCESS;
}

void KLineKWP1281Lib::update()
{
  if (!acknowledge())
  {
    show_debug_info(SEND_ERROR);
  }

  size_t bytes_received;
  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_ACK:
      break;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      break;
  }
}

void KLineKWP1281Lib::disconnect(bool wait_for_response)
{
  send_message(KWP_DISCONNECT);

  _current_module = 0;

  if (wait_for_response)
  {
    show_debug_info(DISCONNECT_INFO);
    size_t bytes_received;
    receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout);
  }

  memset(identification_data.part_number, 0, sizeof(identification_data.part_number));
  memset(identification_data.identification, 0, sizeof(identification_data.identification));
  memset(identification_data.extra_identification, 0, sizeof(identification_data.extra_identification));
}

char *KLineKWP1281Lib::getPartNumber()
{
  return identification_data.part_number;
}

char *KLineKWP1281Lib::getIdentification()
{
  return identification_data.identification;
}

char *KLineKWP1281Lib::getExtraIdentification()
{
  return identification_data.extra_identification;
}

uint16_t KLineKWP1281Lib::getCoding()
{
  return identification_data.coding;
}

uint32_t KLineKWP1281Lib::getWorkshopCode()
{
  return identification_data.workshop_code;
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::login(uint16_t login_code, uint32_t workshop_code)
{
  uint8_t login_code_high_byte = login_code >> 8;
  uint8_t login_code_low_byte = login_code & 0xFF;

  uint8_t workshop_code_top_bit = (workshop_code >> 16) & 0x01;
  uint8_t workshop_code_high_byte = (workshop_code >> 8) & 0xFF;
  uint8_t workshop_code_low_byte = workshop_code & 0xFF;

  uint8_t parameters[] = {login_code_high_byte, login_code_low_byte, workshop_code_top_bit, workshop_code_high_byte, workshop_code_low_byte};
  if (!send_message(KWP_REQUEST_LOGIN, parameters, sizeof(parameters)))
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  size_t bytes_received;
  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_REFUSE:
      show_debug_info(LOGIN_NOT_ACCEPTED);
      return FAIL;

    case TYPE_ACK:
      show_debug_info(LOGIN_ACCEPTED);
      return SUCCESS;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::recode(uint16_t coding, uint32_t workshop_code)
{
  coding <<= 1;

  uint8_t coding_high_byte = coding >> 8;
  uint8_t coding_low_byte_workshop_code_top_bit = (coding & 0xFF) | ((workshop_code >> 16) & 0x01);
  uint8_t workshop_code_high_byte = (workshop_code >> 8) & 0xFF;
  uint8_t workshop_code_low_byte = workshop_code & 0xFF;

  uint8_t parameters[] = {coding_high_byte, coding_low_byte_workshop_code_top_bit, workshop_code_high_byte, workshop_code_low_byte};
  if (!send_message(KWP_REQUEST_RECODE, parameters, sizeof(parameters)))
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  return read_identification(false);
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::readFaults(uint8_t &amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size)
{
  if (!send_message(KWP_REQUEST_FAULT_CODES))
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  uint8_t faults = 0;
  uint8_t fault_buffer_index = 0;
  uint8_t fault_codes_in_current_block;
  uint8_t max_fault_codes_in_array = fault_code_buffer_size / 3;

  while (true)
  {
    size_t bytes_received;
    switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
    {
      case TYPE_REFUSE:
        show_debug_info(FAULT_CODES_NOT_SUPPORTED);
        return FAIL;

      case TYPE_ACK:
        amount_of_fault_codes = faults;
        return SUCCESS;

      case TYPE_FAULT_CODES:
      {
        fault_codes_in_current_block = bytes_received / 3;

        if (fault_codes_in_current_block == 1)
        {
          if (receive_buffer[0] == 0xFF && receive_buffer[1] == 0xFF)
          {
            if (receive_buffer[2] == 0x88)
            {
              amount_of_fault_codes = 0;
              return SUCCESS;
            }
          }
        }

        faults += fault_codes_in_current_block;

        if (fault_codes_in_current_block > max_fault_codes_in_array)
        {
          show_debug_info(ARRAY_NOT_LARGE_ENOUGH);
        }

        uint8_t fault_codes_to_copy = min(fault_codes_in_current_block, max_fault_codes_in_array);

        memcpy(fault_code_buffer + fault_buffer_index, receive_buffer, fault_codes_to_copy * 3);

        fault_buffer_index += fault_codes_to_copy * 3;

        max_fault_codes_in_array -= fault_codes_to_copy;

        if (!acknowledge())
        {
          show_debug_info(SEND_ERROR);
          return ERROR;
        }
        break;
      }

      default:
        show_debug_info(UNEXPECTED_RESPONSE);
        return ERROR;
    }
  }
}

uint16_t KLineKWP1281Lib::getFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size)
{
  if (fault_code_buffer_size < amount_of_fault_codes * 3)
  {
    amount_of_fault_codes = fault_code_buffer_size / 3;
  }

  if (fault_code_index >= amount_of_fault_codes)
  {
    return 0;
  }

  uint16_t fault_code = (fault_code_buffer[3 * fault_code_index] << 8) | fault_code_buffer[3 * fault_code_index + 1];
  return fault_code;
}

bool KLineKWP1281Lib::isOBDFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size)
{
  uint16_t fault_code = getFaultCode(fault_code_index, amount_of_fault_codes, fault_code_buffer, fault_code_buffer_size);

  return isOBDFaultCode(fault_code);
}

bool KLineKWP1281Lib::isOBDFaultCode(uint16_t fault_code)
{
  return (fault_code >= 0x4000 && fault_code <= 0x7FFF);
}

char *KLineKWP1281Lib::getOBDFaultCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size, char *str, size_t string_size)
{
  uint16_t fault_code = getFaultCode(fault_code_index, amount_of_fault_codes, fault_code_buffer, fault_code_buffer_size);

  return getOBDFaultCode(fault_code, str, string_size);
}

char *KLineKWP1281Lib::getOBDFaultCode(uint16_t fault_code, char *str, size_t string_size)
{
  if (!str || !string_size)
  {
    return nullptr;
  }

  if (!isOBDFaultCode(fault_code))
  {
    str[0] = '\0';
    return str;
  }

  char category_type = 0;
  switch ((fault_code >> 0xC) & 0xF)
  {
    case 4:
      category_type = 'P';
      break;
    case 5:
      category_type = 'C';
      break;
    case 6:
      category_type = 'B';
      break;
    case 7:
      category_type = 'U';
      break;
  }

  uint8_t category_code = ((fault_code >> 8) & 0xF) / 4;

  uint16_t converted_dtc = fault_code - (((((fault_code >> 0xC) & 0xF) << 4) | (category_code << 2)) << 8);

  if (converted_dtc > 999)
  {
    str[0] = '\0';
    return str;
  }

  snprintf(str, string_size, "%c%d%03d", category_type, category_code, converted_dtc);
  str[string_size - 1] = '\0';
  return str;
}

char *KLineKWP1281Lib::getFaultDescription(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size, char *str, size_t string_size)
{
  uint16_t fault_code = getFaultCode(fault_code_index, amount_of_fault_codes, fault_code_buffer, fault_code_buffer_size);

  return getFaultDescription(fault_code, str, string_size);
}

char *KLineKWP1281Lib::getFaultDescription(uint16_t fault_code, char *str, size_t string_size)
{
  if (!str || !string_size)
  {
    return nullptr;
  }

  if (isOBDFaultCode(fault_code))
  {
    #ifdef KWP1281_OBD_FAULT_CODE_DESCRIPTION_SUPPORTED

    char OBD_fault_code_string[6];
    getOBDFaultCode(fault_code, OBD_fault_code_string, sizeof(OBD_fault_code_string));

    unsigned int text_code;
    sscanf(&OBD_fault_code_string[1], "%04x", &text_code);
    text_code |= (((fault_code >> 0xC) & 0xF) - 4) << 0xE;

    struct keyed_struct bsearch_key;
    bsearch_key.code = text_code;

    struct keyed_struct *result = (struct keyed_struct *)bsearch(
      &bsearch_key, OBD_fault_description_table_entries, ARRAYSIZE(OBD_fault_description_table_entries),
                                                                 sizeof(struct keyed_struct), compare_keyed_structs);

    if (result)
    {
      keyed_struct struct_from_PGM;
      memcpy_P((void *)&struct_from_PGM, result, sizeof(keyed_struct));

      strncpy_P(str, struct_from_PGM.text, string_size);
      str[string_size - 1] = '\0';
    }
    else
    {
      str[0] = '\0';
    }

    #else

    strncpy(str, "EN_obd", string_size);
    str[string_size - 1] = '\0';

    #endif
  }
  else
  {
    #ifdef KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED

    struct keyed_struct bsearch_key;
    bsearch_key.code = fault_code;

    struct keyed_struct *result = (struct keyed_struct *)bsearch(
      &bsearch_key, fault_description_table_entries, ARRAYSIZE(fault_description_table_entries),
                                                                 sizeof(struct keyed_struct), compare_keyed_structs);

    if (result)
    {
      keyed_struct struct_from_PGM;
      memcpy_P((void *)&struct_from_PGM, result, sizeof(keyed_struct));

      strncpy_P(str, struct_from_PGM.text, string_size);
      str[string_size - 1] = '\0';
    }
    else
    {
      str[0] = '\0';
    }

    #else

    strncpy(str, "EN_dsc", string_size);
    str[string_size - 1] = '\0';

    #endif
  }

  return str;
}

size_t KLineKWP1281Lib::getFaultDescriptionLength(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size)
{
  uint16_t fault_code = getFaultCode(fault_code_index, amount_of_fault_codes, fault_code_buffer, fault_code_buffer_size);

  return getFaultDescriptionLength(fault_code);
}

size_t KLineKWP1281Lib::getFaultDescriptionLength(uint16_t fault_code)
{
  #ifdef KWP1281_FAULT_CODE_DESCRIPTION_SUPPORTED

  struct keyed_struct bsearch_key;
  bsearch_key.code = fault_code;

  struct keyed_struct *result = (struct keyed_struct *)bsearch(
    &bsearch_key, fault_description_table_entries, ARRAYSIZE(fault_description_table_entries),
                                                               sizeof(struct keyed_struct), compare_keyed_structs);

  if (result)
  {
    return strlen_P(result->text);
  }
  else
  {
    return 0;
  }

  #else

  (void)fault_code;

  return strlen("EN_dsc");

  #endif

  return 0;
}

uint8_t KLineKWP1281Lib::getFaultElaborationCode(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size)
{
  if (fault_code_buffer_size < amount_of_fault_codes * 3)
  {
    amount_of_fault_codes = fault_code_buffer_size / 3;
  }

  if (fault_code_index >= amount_of_fault_codes)
  {
    return 0;
  }

  return fault_code_buffer[3 * fault_code_index + 2];
}

char *KLineKWP1281Lib::getFaultElaboration(bool &is_intermittent, uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size, char *str, size_t string_size)
{
  uint8_t elaboration_code = getFaultElaborationCode(fault_code_index, amount_of_fault_codes, fault_code_buffer, fault_code_buffer_size);

  return getFaultElaboration(is_intermittent, elaboration_code, str, string_size);
}

char *KLineKWP1281Lib::getFaultElaboration(bool &is_intermittent, uint8_t elaboration_code, char *str, size_t string_size)
{
  if (!str || !string_size)
  {
    return nullptr;
  }

  is_intermittent = elaboration_code & 0x80;

  elaboration_code &= ~0x80;

  #ifdef KWP1281_FAULT_CODE_ELABORATION_SUPPORTED

  if (elaboration_code < ARRAYSIZE(fault_elaboration_table))
  {
    strncpy_P(str, (const char*)READ_POINTER_FROM_PROGMEM(fault_elaboration_table + elaboration_code), string_size);

    if (string_size)
    {
      str[string_size - 1] = '\0';
    }
  }
  else
  {
    str[0] = '\0';
  }

  #else

  strncpy(str, "EN_elb", string_size);
  str[string_size - 1] = '\0';

  #endif

  return str;
}

size_t KLineKWP1281Lib::getFaultElaborationLength(uint8_t fault_code_index, uint8_t amount_of_fault_codes, uint8_t *fault_code_buffer, size_t fault_code_buffer_size)
{
  uint8_t elaboration_code = getFaultElaborationCode(fault_code_index, amount_of_fault_codes, fault_code_buffer, fault_code_buffer_size);

  return getFaultElaborationLength(elaboration_code);
}

size_t KLineKWP1281Lib::getFaultElaborationLength(uint8_t elaboration_code)
{
  #ifdef KWP1281_FAULT_CODE_ELABORATION_SUPPORTED

  elaboration_code &= ~0x80;

  if (elaboration_code < ARRAYSIZE(fault_elaboration_table))
  {
    return strlen_P((const char*)READ_POINTER_FROM_PROGMEM(fault_elaboration_table + elaboration_code));
  }

  #else

  (void)elaboration_code;

  return strlen("EN_elb");

  #endif

  return 0;
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::clearFaults()
{
  if (!send_message(KWP_REQUEST_CLEAR_FAULTS))
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  size_t bytes_received;
  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_REFUSE:
      show_debug_info(CLEARING_FAULT_CODES_NOT_SUPPORTED);
      return FAIL;

    case TYPE_ACK:
      show_debug_info(CLEARING_FAULT_CODES_ACCEPTED);
      return SUCCESS;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::readAdaptation(uint8_t channel, uint16_t &value)
{
  uint8_t parameters[] = {channel};
  if (!send_message(KWP_REQUEST_ADAPTATION, parameters, sizeof(parameters)))
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  size_t bytes_received;
  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_REFUSE:
      show_debug_info(INVALID_ADAPTATION_CHANNEL);
      return FAIL;

    case TYPE_ADAPTATION:
      show_debug_info(ADAPTATION_RECEIVED);

      value = (receive_buffer[1] << 8) | receive_buffer[2];
      return SUCCESS;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::testAdaptation(uint8_t channel, uint16_t value)
{
  uint8_t value_high_byte = value >> 8;
  uint8_t value_low_byte = value & 0xFF;

  uint8_t parameters[] = {channel, value_high_byte, value_low_byte};
  if (!send_message(KWP_REQUEST_ADAPTATION_TEST, parameters, sizeof(parameters)))
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  size_t bytes_received;
  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_REFUSE:
      show_debug_info(INVALID_ADAPTATION_CHANNEL);
      return FAIL;

    case TYPE_ADAPTATION:
      if (uint16_t((receive_buffer[1] << 8) | receive_buffer[2]) == value)
      {
        show_debug_info(ADAPTATION_ACCEPTED);
        return SUCCESS;
      }
      else
      {
        show_debug_info(ADAPTATION_NOT_ACCEPTED);
        return FAIL;
      }
      break;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::adapt(uint8_t channel, uint16_t value, uint32_t workshop_code)
{
  uint8_t value_high_byte = value >> 8;
  uint8_t value_low_byte = value & 0xFF;

  uint8_t workshop_code_top_bit = (workshop_code >> 16) & 0x01;
  uint8_t workshop_code_high_byte = (workshop_code >> 8) & 0xFF;
  uint8_t workshop_code_low_byte = workshop_code & 0xFF;

  uint8_t parameters[] = {channel, value_high_byte, value_low_byte, workshop_code_top_bit, workshop_code_high_byte, workshop_code_low_byte};
  if (!send_message(KWP_REQUEST_ADAPTATION_SAVE, parameters, sizeof(parameters)))
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  size_t bytes_received;
  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_REFUSE:
      show_debug_info(INVALID_ADAPTATION_CHANNEL_OR_VALUE);
      return FAIL;

    case TYPE_ADAPTATION:
      show_debug_info(ADAPTATION_RECEIVED);
      return SUCCESS;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::basicSetting(uint8_t &amount_of_values, uint8_t group, uint8_t *basic_setting_buffer, size_t basic_setting_buffer_size)
{
  if (group)
  {
    uint8_t parameters[] = {group};
    if (!send_message(KWP_REQUEST_BASIC_SETTING, parameters, sizeof(parameters)))
    {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
  }
  else
  {
    if (!send_message(KWP_REQUEST_BASIC_SETTING_0))
    {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
  }

  size_t bytes_received;
  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_REFUSE:
      show_debug_info(INVALID_BASIC_SETTING_GROUP);
      return FAIL;

    case TYPE_ACK:
      show_debug_info(RECEIVED_EMPTY_BASIC_SETTING_GROUP);
      return FAIL;

    case TYPE_BASIC_SETTING:
    {
      show_debug_info(RECEIVED_BASIC_SETTING);

      amount_of_values = bytes_received;

      uint8_t max_values_in_receive_buffer = sizeof(receive_buffer);
      uint8_t max_values_in_array = basic_setting_buffer_size;
      uint8_t values_to_copy = min(amount_of_values, min(max_values_in_receive_buffer, max_values_in_array));

      memcpy(basic_setting_buffer, receive_buffer, values_to_copy);
      return SUCCESS;
    }
    break;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

uint8_t KLineKWP1281Lib::getBasicSettingValue(uint8_t value_index, uint8_t amount_of_values, uint8_t *basic_setting_buffer, size_t basic_setting_buffer_size)
{
  if (!basic_setting_buffer || !basic_setting_buffer_size)
  {
    return 0;
  }

  if (basic_setting_buffer_size < amount_of_values)
  {
    amount_of_values = basic_setting_buffer_size;
  }

  if (value_index >= amount_of_values)
  {
    return 0;
  }

  uint8_t value = basic_setting_buffer[value_index];
  return value;
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::readGroup(uint8_t &amount_of_measurements, uint8_t group, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  amount_of_measurements = 0;

  if (group)
  {
    uint8_t parameters[] = {group};
    if (!send_message(KWP_REQUEST_GROUP_READING, parameters, sizeof(parameters)))
    {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
  }
  else
  {
    if (!send_message(KWP_REQUEST_GROUP_READING_0))
    {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }
  }

  size_t bytes_received;
  switch (receive_message(&bytes_received, measurement_buffer, measurement_buffer_size, responseTimeout))
  {
    case TYPE_REFUSE:
      show_debug_info(INVALID_MEASUREMENT_GROUP);
      return FAIL;

    case TYPE_ACK:
      show_debug_info(RECEIVED_EMPTY_GROUP);
      return FAIL;

    case TYPE_GROUP_HEADER:
    {
      show_debug_info(RECEIVED_GROUP_HEADER);

      if (bytes_received > measurement_buffer_size)
      {
        show_debug_info(ARRAY_NOT_LARGE_ENOUGH);
        return ERROR;
      }

      size_t buffer_index = 0;
      while (buffer_index < bytes_received)
      {
        uint8_t formula = measurement_buffer[buffer_index];

        uint8_t table_length = measurement_buffer[buffer_index + 2];

        if (((formula == 0x8B || formula == 0x8C || formula == 0x93) && table_length != 17) || (formula == 0x8D && table_length == 0))
        {
          show_debug_info(INVALID_GROUP_HEADER_MAP_LENGTH, table_length);
          return ERROR;
        }

        amount_of_measurements++;

        buffer_index += (3 + table_length);
      }
    }
    return GROUP_HEADER;

    case TYPE_BASIC_SETTING:
    {
      show_debug_info(RECEIVED_GROUP_BODY_OR_BASIC_SETTING);

      if (bytes_received > measurement_buffer_size)
      {
        show_debug_info(ARRAY_NOT_LARGE_ENOUGH);
        return ERROR;
      }

      if (bytes_received == 10)
      {
        return GROUP_BASIC_SETTINGS;
      }
      else if (bytes_received <= 4)
      {
        amount_of_measurements = bytes_received;
        return GROUP_BODY;
      }
      else
      {
        show_debug_info(INVALID_GROUP_BODY_OR_BASIC_SETTING_LENGTH, bytes_received);
      }
    }
    return ERROR;

    case TYPE_GROUP_READING:
    {
      show_debug_info(RECEIVED_GROUP);

      if (bytes_received > measurement_buffer_size)
      {
        show_debug_info(ARRAY_NOT_LARGE_ENOUGH);
        return ERROR;
      }

      size_t buffer_index = 0;
      while (buffer_index < bytes_received)
      {
        uint8_t formula = measurement_buffer[buffer_index];

        uint8_t measurement_length = get_measurement_length(measurement_buffer, bytes_received, buffer_index);

        if (formula == 0x3F)
        {
          if (bytes_received >= measurement_buffer_size)
          {
            show_debug_info(ARRAY_NOT_LARGE_ENOUGH);
            return ERROR;
          }

          memmove(&measurement_buffer[buffer_index + 2], &measurement_buffer[buffer_index + 1], measurement_length - 1);

          measurement_buffer[buffer_index + 1] = measurement_length - 1;

          measurement_buffer[buffer_index] = 0x5F;
        }

        buffer_index += measurement_length;

        amount_of_measurements++;
      }
    }
    return SUCCESS;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

uint8_t KLineKWP1281Lib::getFormula(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return 0;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < measurement_buffer_size; current_measurement++)
  {
    uint8_t formula = measurement_buffer[buffer_index];

    if (current_measurement == measurement_index)
    {
      return formula;
    }

    buffer_index += get_measurement_length(measurement_buffer, 0, buffer_index);
  }

  return 0;
}

uint8_t KLineKWP1281Lib::getNWb(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return 0;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < measurement_buffer_size; current_measurement++)
  {
    uint8_t formula = measurement_buffer[buffer_index];

    if (current_measurement == measurement_index)
    {
      if (formula == 0x5F || formula == 0x76 || formula == 0xA0)
      {
        return 0;
      }
      else
      {
        return measurement_buffer[buffer_index + 1];
      }
    }

    buffer_index += get_measurement_length(measurement_buffer, 0, buffer_index);
  }

  return 0;
}

uint8_t KLineKWP1281Lib::getMWb(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return 0;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < measurement_buffer_size; current_measurement++)
  {
    uint8_t formula = measurement_buffer[buffer_index];

    if (current_measurement == measurement_index)
    {
      if (formula == 0x5F || formula == 0x76 || formula == 0xA0)
      {
        return 0;
      }
      else
      {
        return measurement_buffer[buffer_index + 2];
      }
    }

    buffer_index += get_measurement_length(measurement_buffer, 0, buffer_index);
  }

  return 0;
}

uint8_t *KLineKWP1281Lib::getMeasurementData(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return nullptr;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < measurement_buffer_size; current_measurement++)
  {
    uint8_t formula = measurement_buffer[buffer_index];

    if (current_measurement == measurement_index)
    {
      if (formula == 0x5F || formula == 0x76)
      {
        return &measurement_buffer[buffer_index + 2];
      }
      else
      {
        return &measurement_buffer[buffer_index + 1];
      }
    }

    buffer_index += get_measurement_length(measurement_buffer, 0, buffer_index);
  }

  return nullptr;
}

uint8_t KLineKWP1281Lib::getMeasurementDataLength(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return 0;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < measurement_buffer_size; current_measurement++)
  {
    uint8_t formula = measurement_buffer[buffer_index];

    uint8_t measurement_length = get_measurement_length(measurement_buffer, 0, buffer_index);

    if (current_measurement == measurement_index)
    {
      if (formula == 0x5F || formula == 0x76)
      {
        return measurement_buffer[buffer_index + 1];
      }
      else
      {
        return measurement_length - 1;
      }
    }

    buffer_index += measurement_length;
  }

  return 0;
}

KLineKWP1281Lib::measurementType KLineKWP1281Lib::getMeasurementType(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  uint8_t formula = getFormula(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  return getMeasurementType(formula);
}

KLineKWP1281Lib::measurementType KLineKWP1281Lib::getMeasurementType(uint8_t formula)
{
  if (formula > 0xB5)
  {
    return UNKNOWN;
  }

  switch (formula)
  {
    case 0x00:
    case 0x3F:
    case 0x6C:
    case 0x6D:
    case 0x6E:
      return UNKNOWN;

    case 0x5F:
    case 0x76:
    case 0x0A:
    case 0x10:
    case 0x88:
    case 0x11:
    case 0x8E:
    case 0x1D:
    case 0x25:
    case 0x7B:
    case 0x2C:
    case 0x6B:
    case 0x7F:
    case 0xA1:
      return TEXT;

    default:
      return VALUE;
  }
}

double KLineKWP1281Lib::getMeasurementValue(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  uint8_t formula = getFormula(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  uint8_t *measurement_data = getMeasurementData(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  uint8_t measurement_data_length = getMeasurementDataLength(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  return getMeasurementValue(formula, measurement_data, measurement_data_length);
}

double KLineKWP1281Lib::getMeasurementValue(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length)
{
  if (!measurement_data || !measurement_data_length)
  {
    return 0.0 / 0.0;
  }

  if (formula == 0xA0)
  {
    if (measurement_data_length != 5)
    {
      return 0.0 / 0.0;
    }

    int32_t WORD_BC = (measurement_data[1] << 8) | measurement_data[2];

    if ((measurement_data[3] & 0x80) && ((measurement_data[4] & 0x3F) != 0x23))
    {
      WORD_BC = -WORD_BC;
    }

    uint8_t BYTE_DL = measurement_data[3] & 0xF;
    uint8_t BYTE_DH = (measurement_data[3] >> 4) & 7;

    return measurement_data[0] * WORD_BC * BYTE_DL * negative_pow_10[BYTE_DH];
  }
  else if (measurement_data_length != 2)
  {
    return 0.0 / 0.0;
  }

  uint8_t NWb = measurement_data[0];
  uint8_t MWb = measurement_data[1];

  return getMeasurementValue(formula, NWb, MWb);
}

double KLineKWP1281Lib::getMeasurementValue(uint8_t formula, uint8_t NWb, uint8_t MWb)
{
  double NW = NWb, MW = MWb;
  switch (formula)
  {
    case 0x01: return MW * NW * 0.2;
    case 0x02: return MW * NW * 0.002;
    case 0x03: return MW * NW * 0.002;
    case 0x04: return (MW - 127) * NW * -0.01;
    case 0x05: return (MW - 100) * NW * 0.1;
    case 0x06: return MW * NW * 0.001;
    case 0x07: return MW * NW * 0.01;
    case 0x08: return MW * NW * 0.1;
    case 0x09: return (MW - 127) * NW * 0.02;
    case 0x0A: return (MW ? 1 : 0);
    case 0x0B: return 1 + ((MW - 128) * NW * 0.0001);
    case 0x0C: return MW * NW * 0.001;
    case 0x0D: return (MW - 127) * NW * 0.001;
    case 0x0E: return MW * NW * 0.005;
    case 0x0F: return MW * NW * 0.01;
    case 0x10: return MWb & NWb;
    case 0x11: return To16Bit(NW, MW);
    case 0x12: return MW * NW * 0.04;
    case 0x13: return MW * NW * 0.01;
    case 0x14: return (MW - 128) * NW * 1 / 128;
    case 0x15: return MW * NW * 0.001;
    case 0x16: return MW * NW * 0.001;
    case 0x17: return MW * NW * 1 / 256;
    case 0x18: return MW * NW * 0.001;
    case 0x19: return (256 * MW + NW) * 1 / 180;
    case 0x1A: return MW - NW;
    case 0x1B: return (MW - 128) * NW * 0.01;
    case 0x1C: return MW - NW;
    case 0x1D: return ((MW < NW) ? 1 : 2);
    case 0x1E: return MW * NW * 1 / 12;
    case 0x1F: return MW * NW * 1 / 2560;
    case 0x20: return ToSigned(MW);
    case 0x21: return (MW / NW) * 100;
    case 0x22: return (MW - 128) * NW * 0.01;
    case 0x23: return MW * NW * 0.01;
    case 0x24: return To16Bit(NW, MW) * 10;
    case 0x25: return To16Bit(NW, MW);
    case 0x26: return (MW - 128) * NW * 0.001;
    case 0x27: return NW * MW * 1 / 255;
    case 0x28: return ((NW * 255 + MW) - 4000) * 0.1;
    case 0x29: return NW * 255 + MW;
    case 0x2A: return ((NW * 255 + MW) - 4000) * 0.1;
    case 0x2B: return (NW * 255 + MW) * 0.1;
    case 0x2C: return MW / 100.0 + NW;
    case 0x2D: return MW * NW * 0.1;
    case 0x2E: return ((NW * 256) + MW - 32768) * 0.00275;
    case 0x2F: return (MW - 128) * NW;
    case 0x30: return NW * 255 + MW;
    case 0x31: return NW * MW * 0.025;
    case 0x32: return (MW - 128) * 100 / NW;
    case 0x33: return (MW - 128) * NW / 255;
    case 0x34: return (MW - 50) * NW * 0.02;
    case 0x35: return ((MW - 128) * 256 + NW) / 180;
    case 0x36: return NW * 256 + MW;
    case 0x37: return MW * NW * 0.005;
    case 0x38: return (NWb << 8) & MWb;
    case 0x39: return ((NWb << 8) & MWb) + 65536;
    case 0x3A: return ToSigned(MW) * 1.023;
    case 0x3B: return To16Bit(NW, MW) * 1 / 32768;
    case 0x3C: return To16Bit(NW, MW) * 1 / 100;
    case 0x3D: return (MW - 128) / NW;
    case 0x3E: return MW * NW * 256 / 1000;
    case 0x40: return MW + NW;
    case 0x41: return (MW - 127) * NW * 0.01;
    case 0x42: return MW * NW / 512;
    case 0x43: return ToSigned(NW, MW) * 2.5;
    case 0x44: return ToSigned(NW, MW) * 0.1358;
    case 0x45: return ToSigned(NW, MW) * 0.3255;
    case 0x46: return ToSigned(NW, MW) * 0.192;
    case 0x47: return MW * NW;
    case 0x48: return (NW * 255 + MW * (211 - NW)) / 4080;
    case 0x49: return NW * MW * 0.01;
    case 0x4A: return 0.1 * NW * MW;
    case 0x4B: return NW * 256 + MW;
    case 0x4C: return NW * 255 + MW;
    case 0x4D: return (255 * NW + MW * 60) / 4080;
    case 0x4E: return ToSigned(MW) * 1.819;
    case 0x4F: return MW;
    case 0x50: return To16Bit(NW, MW) / 100;
    case 0x51: return ToSigned(NW, MW) * 0.04375;
    case 0x52: return ToSigned(NW, MW) * 0.00981;
    case 0x53: return ToSigned(NW, MW) * 0.01;
    case 0x54: return ToSigned(NW, MW) * 0.0973;
    case 0x55: return ToSigned(NW, MW) * 0.002865;
    case 0x56: return MW * NW * 0.1;
    case 0x57: return NW * (MW - 128) * 0.1;
    case 0x58: return MW * NW * 0.01;
    case 0x59: return NW * 256 + MW;
    case 0x5A: return NW * MW * 0.1;
    case 0x5B: return NW * (MW - 128) * 0.1;
    case 0x5C: return NW * MW;
    case 0x5D: return (MW - 128) * NW * 0.001;
    case 0x5E: return NW * (MW - 128) * 0.1;
    case 0x60: return NW * MW * 0.1;
    case 0x61: return (MW - NW) * 5;
    case 0x62: return NW * MW * 0.1;
    case 0x63: return ToSigned(NW, MW);
    case 0x64: return NW * MW * 0.1;
    case 0x65: return MW * NW * 0.001;
    case 0x66: return MW * NW * 0.1;
    case 0x67: return NW + MW * 0.05;
    case 0x68: return (MW - 128) * NW * 0.2;
    case 0x69: return (MW - 128) * NW * 0.01;
    case 0x6A: return (MW - 128) * NW * 0.1;
    case 0x6B: return ToSigned(NW, MW);
    case 0x6F: return 0x6F0000 | (NWb << 8) | MWb;
    case 0x70: return (MW - 128) * NW * 0.001;
    case 0x71: return (MW - 128) * NW * 0.01;
    case 0x72: return (MW - 128) * NW;
    case 0x73: return ToSigned(NW, MW);
    case 0x74: return ToSigned(NW, MW);
    case 0x75: return (MW - 64) * NW * 0.01;
    case 0x77: return MW * NW * 0.01;
    case 0x78: return MW * NW * 1.41;
    case 0x79: return ((MW * 256) + NW) * 0.5;
    case 0x7A: return ((MW * 256) + NW - 32768) * 0.01;
    case 0x7B: return To16Bit(NW, MW);
    case 0x7C: return MW * NW * 0.1;
    case 0x7D: return NW - MW;
    case 0x7E: return NW * MW * 0.1;
    case 0x7F: return
      200000 + (MWb & 0x7F) * 100
      + (((NWb & 0x07) << 1) | ((MWb & 0x80) >> 7))
      + ((NWb & 0xF8) >> 3) * 0.01;
    case 0x80: return MW * NW;
    case 0x81: return MW * NW * 1 / 256;
    case 0x82: return MW * NW * 1 / 2560;
    case 0x83: return (MW * NW * 0.5) - 30;
    case 0x84: return MW * NW * 0.5;
    case 0x85: return MW * NW * 1 / 256;
    case 0x86: return MW * NW;
    case 0x87: return MW * NW;
    case 0x88: return MWb & NWb;
    case 0x89: return MW * NW * 0.01;
    case 0x8A: return MW * NW * 0.001;
    case 0x8E: return To16Bit(NW, MW);
    case 0x8F: return (MW - 128) * NW * 0.01;
    case 0x90: return MW * NW * 0.01;
    case 0x91: return MW * NW * 0.01;
    case 0x92: return 1 + ((MW - 128) * NW * 0.0001);
    case 0x94: return (MW - 128) * NW * 0.25;
    case 0x95: return (MW - 100) * NW * 0.1;
    case 0x96: return (256 * MW + NW) * 1 / 180;
    case 0x97: return (MW - 128) * NW * 0.01;
    case 0x98: return NW * MW * 0.025;
    case 0x99: return (MW - 128) * NW / 255;
    case 0x9A: return MW * NW;
    case 0x9B: return (NW * MW * 0.01) - 90;
    case 0x9C: return To16Bit(NW, MW);
    case 0x9D: return ToSigned(NW, MW);
    case 0x9E: return To16Bit(NW, MW) * 0.01;
    case 0x9F: return ((MW - 127) * 256 + NW) * 0.1;
    case 0xA1: return To16Bit(NW, MW);
    case 0xA2: return NW * MW * 0.448;
    case 0xA3: return MW / 100.0 + NW;
    case 0xA4: return ((MW <= 100) ? MW : ((MW > 100 && MW <= 200) ? (MW - 100) : MW));
    case 0xA5: return ((MW * 256) + NW - 32768);
    case 0xA6: return ToSigned(NW, MW) * 2.5;
    case 0xA7: return MW * NW * 0.001;
    case 0xA8: return (256 * NW + MW) * 0.01;
    case 0xA9: return MW * NW + 200;
    case 0xAA: return MW * NW * 1 / 2560;
    case 0xAB: return MW * NW * 1 / 2560;
    case 0xAC: return MW * NW;
    case 0xAD: return MW * NW * 0.1;
    case 0xAE: return MW * NW * 0.01;
    case 0xAF: return MW * NW * 0.005;
    case 0xB0: return MW * NW * 0.1;
    case 0xB1: return ((NW - 128) * 256 + MW);
    case 0xB2: return ((NW - 128) * 256 + MW) * ((double)6 / 51);
    case 0xB3: return (NW * 256 + MW) * 10;
    case 0xB4: return MW * NW * 1 / 256;
    case 0xB5: return MW * NW * 0.001;
  }

  return 0.0 / 0.0;
}

char *KLineKWP1281Lib::getMeasurementUnits(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size, char *str, size_t string_size)
{
  uint8_t formula = getFormula(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  uint8_t *measurement_data = getMeasurementData(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  uint8_t measurement_data_length = getMeasurementDataLength(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  return getMeasurementUnits(formula, measurement_data, measurement_data_length, str, string_size);
}

char *KLineKWP1281Lib::getMeasurementUnits(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length, char *str, size_t string_size)
{
  if (!measurement_data || !measurement_data_length || !str || !string_size)
  {
    return nullptr;
  }

  if (formula == 0xA0)
  {
    if (measurement_data_length != 5)
    {
      return nullptr;
    }

    bool has_units_prefix = true;
    switch (measurement_data[4] >> 6)
    {
      case 0: has_units_prefix = false; break;
      case 1: str[0] = 'k'; break;
      case 2: str[0] = 'm'; break;
      case 3: str[0] = 'u'; break;
    }

    const char *unit_pointer = nullptr;
    switch (measurement_data[4] & 0x3F)
    {
      case 0x01: unit_pointer = KWP_units_Voltage; break;
      case 0x02: unit_pointer = KWP_units_Vss; break;
      case 0x03: unit_pointer = KWP_units_Current; break;
      case 0x04: unit_pointer = KWP_units_Capacity; break;
      case 0x05: unit_pointer = KWP_units_Resistance; break;
      case 0x06: unit_pointer = KWP_units_Power; break;
      case 0x07: unit_pointer = KWP_units_Wm2; break;
      case 0x08: unit_pointer = KWP_units_Wcm2; break;
      case 0x09: unit_pointer = KWP_units_Wh; break;
      case 0x0A: unit_pointer = KWP_units_Ws; break;
      case 0x0B: unit_pointer = KWP_units_Distance; break;
      case 0x0C: unit_pointer = KWP_units_ms; break;
      case 0x0D: unit_pointer = KWP_units_Acceleration; break;
      case 0x0E: unit_pointer = KWP_units_Distance_c; break;
      case 0x0F: unit_pointer = KWP_units_Speed; break;
      case 0x10: unit_pointer = KWP_units_Volume; break;
      case 0x11: unit_pointer = KWP_units_Consumption_100km; break;
      case 0x12: unit_pointer = KWP_units_Fuel_Level_Factor; break;
      case 0x13: unit_pointer = KWP_units_Consumption_h; break;
      case 0x14: unit_pointer = KWP_units_lkm; break;
      case 0x15: unit_pointer = KWP_units_Time; break;
      case 0x16: unit_pointer = KWP_units_Time_h; break;
      case 0x17: unit_pointer = KWP_units_Time_mo; break;
      case 0x18: unit_pointer = KWP_units_Mass; break;
      case 0x19: unit_pointer = KWP_units_Mass_Flow; break;
      case 0x1A: unit_pointer = KWP_units_Mass_Per_Stroke_m; break;
      case 0x1B: unit_pointer = KWP_units_Torque; break;
      case 0x1C: unit_pointer = KWP_units_N; break;
      case 0x1D: unit_pointer = KWP_units_Pressure; break;
      case 0x1E: unit_pointer = KWP_units_Angle; break;
      case 0x1F: unit_pointer = KWP_units_angdeg; break;
      case 0x20: unit_pointer = KWP_units_Temperature; break;
      case 0x21: unit_pointer = KWP_units_degF; break;
      case 0x22: unit_pointer = KWP_units_Turn_Rate; break;
      case 0x23: break;
      case 0x24: unit_pointer = KWP_units_RPM; break;
      case 0x25: unit_pointer = KWP_units_Percentage; break;
      case 0x26: unit_pointer = KWP_units_Correction; break;
      case 0x27: unit_pointer = KWP_units_Correction; break;
      case 0x28: unit_pointer = KWP_units_Misfires; break;
      case 0x29: unit_pointer = KWP_units_Imp; break;
      case 0x2A: unit_pointer = KWP_units_Attenuation; break;
      default:
        str[0] = '\0';
        return str;
    }

    if ((measurement_data[4] & 0x3F) == 0x23)
    {
      if (measurement_data[3] & 0x80)
      {
        unit_pointer = KWP_units_Ignition_BTDC;
      }
      else
      {
        unit_pointer = KWP_units_Ignition_ATDC;
      }
    }

    strncpy_P(str + has_units_prefix, unit_pointer, string_size - has_units_prefix);
    str[string_size - 1] = '\0';
    return str;
  }
  else if (measurement_data_length != 2)
  {
    return nullptr;
  }

  uint8_t NWb = measurement_data[0];
  uint8_t MWb = measurement_data[1];

  return getMeasurementUnits(formula, NWb, MWb, str, string_size);
}

char *KLineKWP1281Lib::getMeasurementUnits(uint8_t formula, uint8_t NWb, uint8_t MWb, char *str, size_t string_size)
{
  if (!str || !string_size)
  {
    return nullptr;
  }

  if (getMeasurementType(formula) != VALUE)
  {
    return nullptr;
  }

  const char *unit_pointer = nullptr;
  switch (formula)
  {
    case 0x01:
    case 0x74:
    case 0x80:
    case 0x8B:
      unit_pointer = KWP_units_RPM;
      break;

    case 0x02:
    case 0x14:
    case 0x17:
    case 0x21:
    case 0x77:
    case 0x81:
    case 0x93:
    case 0xA4:
    case 0xA8:
      unit_pointer = KWP_units_Percentage;
      break;

    case 0x03:
    case 0x09:
    case 0x1E:
    case 0x43:
    case 0x51:
    case 0x5B:
    case 0x70:
    case 0x78:
    case 0x84:
    case 0x8F:
    case 0x9A:
    case 0x9B:
    case 0xA2:
      unit_pointer = KWP_units_Angle;
      break;

    case 0x04:
      unit_pointer = (MWb < 127) ? KWP_units_Ignition_BTDC : KWP_units_Ignition_ATDC;
      break;

    case 0x05:
    case 0x1A:
    case 0x61:
    case 0x75:
    case 0x8C:
    case 0x95:
    case 0x9F:
      unit_pointer = KWP_units_Temperature;
      break;

    case 0x06:
    case 0x15:
    case 0x2B:
    case 0x42:
    case 0x48:
    case 0x4D:
    case 0x67:
    case 0x85:
    case 0x8A:
      unit_pointer = KWP_units_Voltage;
      break;

    case 0x07:
    case 0x6A:
    case 0x86:
    case 0x9E:
      unit_pointer = KWP_units_Speed;
      break;

    case 0x0C:
    case 0x40:
    case 0x49:
      unit_pointer = KWP_units_Resistance;
      break;

    case 0x0D:
    case 0x41:
    case 0x66:
      unit_pointer = KWP_units_Distance_m;
      break;

    case 0x0E:
    case 0x45:
    case 0x53:
    case 0x64:
    case 0xAF:
      unit_pointer = KWP_units_Pressure;
      break;

    case 0x0F:
    case 0x16:
    case 0x2F:
    case 0x89:
      unit_pointer = KWP_units_Time_m;
      break;

    case 0x12:
    case 0x32:
    case 0x60:
      unit_pointer = KWP_units_Pressure_m;
      break;

    case 0x13:
      unit_pointer = KWP_units_Volume;
      break;

    case 0x18:
    case 0x28:
    case 0x56:
    case 0x82:
      unit_pointer = KWP_units_Current;
      break;

    case 0x19:
    case 0x35:
    case 0x96:
      unit_pointer = KWP_units_Mass_Flow;
      break;

    case 0x1B:
      unit_pointer = (MWb < 128) ? KWP_units_Ignition_ATDC : KWP_units_Ignition_BTDC;
      break;

    case 0x22:
      unit_pointer = KWP_units_Correction;
      break;

    case 0x23:
    case 0x90:
      unit_pointer = KWP_units_Consumption_h;
      break;

    case 0x24:
    case 0x5C:
    case 0x6F:
      unit_pointer = KWP_units_Distance_k;
      break;

    case 0x26:
    case 0x2E:
    case 0x97:
      unit_pointer = KWP_units_Segment_Correction;
      break;

    case 0x27:
    case 0x31:
    case 0x33:
    case 0x98:
    case 0x99:
      unit_pointer = KWP_units_Mass_Per_Stroke_m;
      break;

    case 0x29:
      unit_pointer = KWP_units_Capacity;
      break;

    case 0x2A:
      unit_pointer = KWP_units_Power_k;
      break;

    case 0x2D:
    case 0x91:
      unit_pointer = KWP_units_Consumption_100km;
      break;

    case 0x34:
    case 0x5D:
    case 0x5E:
    case 0xB1:
      unit_pointer = KWP_units_Torque;
      break;

    case 0x37:
    case 0x3C:
    case 0x3E:
      unit_pointer = KWP_units_Time;
      break;

    case 0x3A:
    case 0x4E:
      unit_pointer = KWP_units_Misfires;
      break;

    case 0x44:
    case 0x55:
    case 0x57:
    case 0xA6:
    case 0xB2:
      unit_pointer = KWP_units_Turn_Rate;
      break;

    case 0x46:
    case 0x52:
    case 0x54:
      unit_pointer = KWP_units_Acceleration;
      break;

    case 0x47:
    case 0x9C:
    case 0x9D:
      unit_pointer = KWP_units_Distance_c;
      break;

    case 0x4A:
      unit_pointer = KWP_units_Time_mo;
      break;

    case 0x4C:
    case 0x50:
    case 0x58:
      unit_pointer = KWP_units_Resistance_k;
      break;

    case 0x59:
    case 0xA3:
      unit_pointer = KWP_units_Time_h;
      break;

    case 0x5A:
      unit_pointer = KWP_units_Mass_k;
      break;

    case 0x62:
      unit_pointer = KWP_units_Impulses;
      break;

    case 0x65:
      unit_pointer = KWP_units_Fuel_Level_Factor;
      break;

    case 0x68:
      unit_pointer = KWP_units_Volume_m;
      break;

    case 0x69:
    case 0x72:
      unit_pointer = KWP_units_Distance;
      break;

    case 0x73:
      unit_pointer = KWP_units_Power;
      break;

    case 0x7C:
    case 0xA5:
      unit_pointer = KWP_units_Current_m;
      break;

    case 0x7D:
      unit_pointer = KWP_units_Attenuation;
      break;

    case 0x83:
      unit_pointer = ((float(NWb) * float(MWb) / 2.0 - 30.0) < 0) ? KWP_units_Ignition_BTDC : KWP_units_Ignition_ATDC;
      break;

    case 0xA7:
      unit_pointer = KWP_units_Resistance_m;
      break;

    case 0xA9:
      unit_pointer = KWP_units_Voltage_m;
      break;

    case 0xAA:
    case 0xAB:
      unit_pointer = KWP_units_Mass;
      break;

    case 0xAC:
      unit_pointer = KWP_units_Mass_Flow_km;
      break;

    case 0xAD:
    case 0xB5:
      unit_pointer = KWP_units_Mass_Flow_m;
      break;

    case 0xAE:
      unit_pointer = KWP_units_Consumption_1000km;
      break;

    case 0xB0:
      unit_pointer = KWP_units_Parts_Per_Million;
      break;

    case 0xB4:
      unit_pointer = KWP_units_Mass_Per_Stroke_k;
      break;

    default:
      str[0] = '\0';
      return str;
  }

  strncpy_P(str, unit_pointer, string_size);
  str[string_size - 1] = '\0';
  return str;
}

char *KLineKWP1281Lib::getMeasurementText(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size, char *str, size_t string_size)
{
  uint8_t formula = getFormula(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  uint8_t *measurement_data = getMeasurementData(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  uint8_t measurement_data_length = getMeasurementDataLength(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  return getMeasurementText(formula, measurement_data, measurement_data_length, str, string_size);
}

char *KLineKWP1281Lib::getMeasurementText(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length, char *str, size_t string_size)
{
  if (!measurement_data || !measurement_data_length || !str || !string_size)
  {
    return nullptr;
  }

  if (getMeasurementType(formula) != TEXT)
  {
    return nullptr;
  }

  str[0] = '\0';

  if (formula == 0x5F || formula == 0x76)
  {
    switch (formula)
    {
      case 0x5F:
      {
        size_t bytes_to_copy = (measurement_data_length < (string_size - 1) ? measurement_data_length : (string_size - 1));
        memcpy(str, measurement_data, bytes_to_copy);
        str[bytes_to_copy] = '\0';
      }
      break;

      case 0x76:
      {
        size_t max_hex_bytes_in_string = (string_size - 1) / 2;
        size_t bytes_to_copy = (measurement_data_length < max_hex_bytes_in_string) ? measurement_data_length : max_hex_bytes_in_string;

        for (size_t i = 0; i < bytes_to_copy; i++)
        {
          char hex_byte[3];
          sprintf(hex_byte, "%02X", measurement_data[i]);
          strcat(str, hex_byte);
        }
      }
      break;
    }

    return str;
  }
  else
  {
    uint8_t NWb = measurement_data[0], MWb = measurement_data[1];

    const char *unit_pointer = nullptr;
    switch (formula)
    {
      case 0x0A:
        unit_pointer = MWb ? KWP_units_Warm : KWP_units_Cold;
        break;

      case 0x10:
      case 0x88:
        for (uint8_t i = 0; i < ((string_size < 8) ? string_size : 8); i++)
        {
          if (NWb & (1 << (7 - i)))
          {
            if (MWb & (1 << (7 - i)))
            {
              str[i] = '1';
            }
            else
            {
              str[i] = '0';
            }
          }
          else
          {
            str[i] = ' ';
          }
        }

        if (string_size > 8)
        {
          str[8] = '\0';
        }
        else
        {
          str[string_size - 1] = '\0';
        }
        return str;

      case 0x11:
      case 0x8E:
        if (string_size > 0)
        {
          str[0] = NWb;
        }

        if (string_size > 1)
        {
          str[1] = MWb;
        }

        if (string_size > 2)
        {
          str[2] = '\0';
        }
        else
        {
          str[string_size - 1] = '\0';
        }
        return str;

      case 0x1D:
        unit_pointer = (MWb < NWb) ? KWP_units_Map1 : KWP_units_Map2;
        break;

      case 0x25:
      case 0x7B:
      {
        #ifdef KWP1281_TEXT_TABLE_SUPPORTED

        uint16_t code = (NWb << 8) | MWb;

        struct keyed_struct bsearch_key;
        bsearch_key.code = code;

        struct keyed_struct *result = (struct keyed_struct *)bsearch(
          &bsearch_key, text_table_entries, ARRAYSIZE(text_table_entries),
                                                                     sizeof(struct keyed_struct), compare_keyed_structs);

        if (result)
        {
          keyed_struct struct_from_PGM;
          memcpy_P((void *)&struct_from_PGM, result, sizeof(keyed_struct));

          strncpy_P(str, struct_from_PGM.text, string_size);
          str[string_size - 1] = '\0';
        }
        else
        {
          str[0] = '\0';
        }

        #else

        strncpy(str, "EN_f25", string_size);
        str[string_size - 1] = '\0';

        #endif
      }
      return str;

      case 0x2C:
        snprintf(str, string_size, "%02d:%02d", NWb, MWb);
        str[string_size - 1] = '\0';
        return str;

      case 0x6B:
        snprintf(str, string_size, "%02X %02X", NWb, MWb);
        str[string_size - 1] = '\0';
        return str;

      case 0x7F:
        snprintf(str, string_size, "%04d.%02d.%02d", 2000 + (MWb & 0x7F), ((NWb & 0x07) << 1) | ((MWb & 0x80) >> 7), (NWb & 0xF8) >> 3);
        str[string_size - 1] = '\0';
        return str;

      case 0xA1:
        snprintf(str, string_size, BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(NWb), BYTE_TO_BINARY(MWb));
        str[string_size - 1] = '\0';
        return str;

      default:
        str[0] = '\0';
        return str;
    }

    strncpy_P(str, unit_pointer, string_size);
    str[string_size - 1] = '\0';
  }

  return str;
}

size_t KLineKWP1281Lib::getMeasurementTextLength(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  uint8_t formula = getFormula(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  uint8_t *measurement_data = getMeasurementData(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);
  uint8_t measurement_data_length = getMeasurementDataLength(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  return getMeasurementTextLength(formula, measurement_data, measurement_data_length);
}

size_t KLineKWP1281Lib::getMeasurementTextLength(uint8_t formula, uint8_t *measurement_data, uint8_t measurement_data_length)
{
  if (!measurement_data || !measurement_data_length)
  {
    return 0;
  }

  if (getMeasurementType(formula) != TEXT)
  {
    return 0;
  }

  if (formula == 0x5F || formula == 0x76)
  {
    switch (formula)
    {
      case 0x5F:
        return measurement_data_length;

      case 0x76:
        return 2 * measurement_data_length;
    }

    return 0;
  }
  else
  {
    uint8_t NWb = measurement_data[0], MWb = measurement_data[1];

    const char *unit_pointer = nullptr;
    switch (formula)
    {
      case 0x0A:
        unit_pointer = MWb ? KWP_units_Warm : KWP_units_Cold;
        break;

      case 0x10:
      case 0x88:
        return 8;

      case 0x11:
      case 0x8E:
        return 2;

      case 0x1D:
        unit_pointer = (MWb < NWb) ? KWP_units_Map1 : KWP_units_Map2;
        break;

      case 0x25:
      case 0x7B:
      {
        #ifdef KWP1281_TEXT_TABLE_SUPPORTED

        uint16_t code = (NWb << 8) | MWb;

        struct keyed_struct bsearch_key;
        bsearch_key.code = code;

        struct keyed_struct *result = (struct keyed_struct *)bsearch(
          &bsearch_key, text_table_entries, ARRAYSIZE(text_table_entries),
                                                                     sizeof(struct keyed_struct), compare_keyed_structs);

        if (result)
        {
          keyed_struct struct_from_PGM;
          memcpy_P((void *)&struct_from_PGM, result, sizeof(keyed_struct));

          return strlen_P(struct_from_PGM.text);
        }
        else
        {
          return 0;
        }

        #else

        return strlen("EN_f25");

        #endif
      }
      return 0;

      case 0x2C:
        return 5;

      case 0x6B:
        return 5;

      case 0x7F:
        return 10;

      case 0xA1:
        return 17;

      default:
        return 0;
    }

    return strlen_P(unit_pointer);
  }

  return 0;
}

uint8_t KLineKWP1281Lib::getMeasurementDecimals(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *measurement_buffer, size_t measurement_buffer_size)
{
  uint8_t formula = getFormula(measurement_index, amount_of_measurements, measurement_buffer, measurement_buffer_size);

  return getMeasurementDecimals(formula);
}

uint8_t KLineKWP1281Lib::getMeasurementDecimals(uint8_t formula)
{
  if (formula < sizeof(KWP_decimals_table))
  {
    return KWP_decimals_table[formula];
  }

  return 0;
}

uint8_t KLineKWP1281Lib::getFormulaFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return 0;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < header_buffer_size; current_measurement++)
  {
    uint8_t table_length = header_buffer[buffer_index + 2];

    if (current_measurement == measurement_index)
    {
      return header_buffer[buffer_index];
    }

    buffer_index += (3 + table_length);
  }

  return 0;
}

uint8_t KLineKWP1281Lib::getNWbFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return 0;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < header_buffer_size; current_measurement++)
  {
    uint8_t table_length = header_buffer[buffer_index + 2];

    if (current_measurement == measurement_index)
    {
      return header_buffer[buffer_index + 1];
    }

    buffer_index += (3 + table_length);
  }

  return 0;
}

uint8_t KLineKWP1281Lib::getMWbFromBody(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *body_buffer, size_t body_buffer_size)
{
  if (measurement_index >= amount_of_measurements || amount_of_measurements > body_buffer_size)
  {
    return 0;
  }

  return body_buffer[measurement_index];
}

uint8_t *KLineKWP1281Lib::getDataTableFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return nullptr;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < header_buffer_size; current_measurement++)
  {
    uint8_t table_length = header_buffer[buffer_index + 2];

    if (current_measurement == measurement_index)
    {
      return &header_buffer[buffer_index + 3];
    }

    buffer_index += (3 + table_length);
  }

  return nullptr;
}

uint8_t KLineKWP1281Lib::getDataTableLengthFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size)
{
  if (measurement_index >= amount_of_measurements)
  {
    return 0;
  }

  size_t buffer_index = 0;
  for (uint8_t current_measurement = 0; current_measurement < amount_of_measurements && buffer_index < header_buffer_size; current_measurement++)
  {
    uint8_t table_length = header_buffer[buffer_index + 2];

    if (current_measurement == measurement_index)
    {
      return header_buffer[buffer_index + 2];
    }

    buffer_index += (3 + table_length);
  }

  return 0;
}

KLineKWP1281Lib::measurementType KLineKWP1281Lib::getMeasurementTypeFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size)
{
  uint8_t formula = getFormulaFromHeader(measurement_index, amount_of_measurements, header_buffer, header_buffer_size);

  return getMeasurementTypeFromHeader(formula);
}

KLineKWP1281Lib::measurementType KLineKWP1281Lib::getMeasurementTypeFromHeader(uint8_t formula)
{
  if (formula > 0xB5)
  {
    return UNKNOWN;
  }

  switch (formula)
  {
    case 0x00:
    case 0x3F:
    case 0x6C:
    case 0x6D:
    case 0x6E:
    case 0x5F:
    case 0x76:
    case 0xA0:
      return UNKNOWN;

    case 0x0A:
    case 0x10:
    case 0x88:
    case 0x11:
    case 0x8E:
    case 0x1D:
    case 0x25:
    case 0x7B:
    case 0x2C:
    case 0x6B:
    case 0x7F:
    case 0xA1:
    case 0x8D:
      return TEXT;

    default:
      return VALUE;
  }
}

double KLineKWP1281Lib::getMeasurementValueFromHeaderBody(uint8_t measurement_index, uint8_t amount_of_measurements_in_header, uint8_t *header_buffer, size_t header_buffer_size, uint8_t amount_of_measurements_in_body, uint8_t *body_buffer, size_t body_buffer_size)
{
  uint8_t formula = getFormulaFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);

  uint8_t NWb = getNWbFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);
  uint8_t MWb = getMWbFromBody(measurement_index, amount_of_measurements_in_body, body_buffer, body_buffer_size);

  uint8_t *data_table = getDataTableFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);
  uint8_t data_table_length = getDataTableLengthFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);

  return getMeasurementValueFromHeaderBody(formula, NWb, MWb, data_table, data_table_length);
}

double KLineKWP1281Lib::getMeasurementValueFromHeaderBody(uint8_t formula, uint8_t NWb, uint8_t MWb, uint8_t *data_table, uint8_t data_table_length)
{
  if (formula == 0x3F || formula == 0x5F || formula == 0x76 || formula == 0x3F || formula == 0xA0)
  {
    return 0.0 / 0.0;
  }

  if (((formula == 0x8B || formula == 0x8C || formula == 0x93) && (!data_table || data_table_length != 17)) || (formula == 0x8D && (!data_table || data_table_length == 0)))
  {
    return 0.0 / 0.0;
  }

  if (formula == 0x8B || formula == 0x8C || formula == 0x93)
  {
    uint8_t map_table_index = MWb / (data_table_length - 1);

    if (map_table_index > data_table_length - 2)
    {
      map_table_index = data_table_length - 2;
    }

    uint8_t map_byte = data_table[map_table_index];

    int16_t difference = data_table[map_table_index + 1] - map_byte;

    double result = map_byte + ((difference * (MWb % (data_table_length - 1))) / (data_table_length - 1));

    if (formula == 0x8B)
    {
      return result * NWb;
    }

    return result - NWb;
  }

  return getMeasurementValue(formula, NWb, MWb);
}

char *KLineKWP1281Lib::getMeasurementUnitsFromHeaderBody(uint8_t measurement_index, uint8_t amount_of_measurements_in_header, uint8_t *header_buffer, size_t header_buffer_size, uint8_t amount_of_measurements_in_body, uint8_t *body_buffer, size_t body_buffer_size, char *str, size_t string_size)
{
  uint8_t formula = getFormulaFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);

  uint8_t NWb = getNWbFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);
  uint8_t MWb = getMWbFromBody(measurement_index, amount_of_measurements_in_body, body_buffer, body_buffer_size);

  return getMeasurementUnitsFromHeaderBody(formula, NWb, MWb, str, string_size);
}

char *KLineKWP1281Lib::getMeasurementUnitsFromHeaderBody(uint8_t formula, uint8_t NWb, uint8_t MWb, char* str, size_t string_size)
{
  return getMeasurementUnits(formula, NWb, MWb, str, string_size);
}

char *KLineKWP1281Lib::getMeasurementTextFromHeaderBody(uint8_t measurement_index, uint8_t amount_of_measurements_in_header, uint8_t *header_buffer, size_t header_buffer_size, uint8_t amount_of_measurements_in_body, uint8_t *body_buffer, size_t body_buffer_size, char* str, size_t string_size)
{
  uint8_t formula = getFormulaFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);

  uint8_t NWb = getNWbFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);
  uint8_t MWb = getMWbFromBody(measurement_index, amount_of_measurements_in_body, body_buffer, body_buffer_size);

  uint8_t *data_table = getDataTableFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);
  uint8_t data_table_length = getDataTableLengthFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);

  return getMeasurementTextFromHeaderBody(formula, NWb, MWb, data_table, data_table_length, str, string_size);
}

char *KLineKWP1281Lib::getMeasurementTextFromHeaderBody(uint8_t formula, uint8_t NWb, uint8_t MWb, uint8_t *data_table, uint8_t data_table_length, char* str, size_t string_size)
{
  if (!str || !string_size)
  {
    return nullptr;
  }

  if (formula == 0x3F || formula == 0x5F || formula == 0x76 || formula == 0x3F || formula == 0xA0)
  {
    return nullptr;
  }

  if (formula == 0x8D && (!data_table || data_table_length == 0))
  {
    return nullptr;
  }

  if (formula == 0x8D)
  {
    uint8_t start_index = 0, end_index = 0;
    uint8_t string_counter = 0;

    for (uint8_t i = 0; i < data_table_length; i++)
    {
      if (data_table[i] == 0x03)
      {
        end_index = i;

        if (string_counter == MWb)
        {
          uint8_t string_length = end_index - start_index;

          uint8_t bytes_to_copy = (string_length < (string_size - 1)) ? string_length : (string_size - 1);

          memcpy(str, &data_table[start_index], bytes_to_copy);
          str[bytes_to_copy] = '\0';
          return str;
        }
        else
        {
          start_index = i + 1;
        }

        string_counter++;
      }
    }

    if ((end_index < start_index) && (MWb == string_counter))
    {
      uint8_t string_length = data_table_length - start_index;

      uint8_t bytes_to_copy = (string_length < (string_size - 1)) ? string_length : (string_size - 1);

      memcpy(str, &data_table[start_index], bytes_to_copy);
      str[bytes_to_copy] = '\0';
      return str;
    }

    str[0] = '\0';
    return str;
  }

  uint8_t dummy_measurement_data[] = {NWb, MWb};
  uint8_t dummy_measurement_data_length = 2;
  return getMeasurementText(formula, dummy_measurement_data, dummy_measurement_data_length, str, string_size);
}

size_t KLineKWP1281Lib::getMeasurementTextLengthFromHeaderBody(uint8_t measurement_index, uint8_t amount_of_measurements_in_header, uint8_t *header_buffer, size_t header_buffer_size, uint8_t amount_of_measurements_in_body, uint8_t *body_buffer, size_t body_buffer_size)
{
  uint8_t formula = getFormulaFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);

  uint8_t NWb = getNWbFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);
  uint8_t MWb = getMWbFromBody(measurement_index, amount_of_measurements_in_body, body_buffer, body_buffer_size);

  uint8_t *data_table = getDataTableFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);
  uint8_t data_table_length = getDataTableLengthFromHeader(measurement_index, amount_of_measurements_in_header, header_buffer, header_buffer_size);

  return getMeasurementTextLengthFromHeaderBody(formula, NWb, MWb, data_table, data_table_length);
}

size_t KLineKWP1281Lib::getMeasurementTextLengthFromHeaderBody(uint8_t formula, uint8_t NWb, uint8_t MWb, uint8_t *data_table, uint8_t data_table_length)
{
  if (formula == 0x3F || formula == 0x5F || formula == 0x76 || formula == 0x3F || formula == 0xA0)
  {
    return 0;
  }

  if (formula == 0x8D && (!data_table || data_table_length == 0))
  {
    return 0;
  }

  if (formula == 0x8D)
  {
    uint8_t start_index = 0, end_index = 0;
    uint8_t string_counter = 0;

    for (uint8_t i = 0; i < data_table_length; i++)
    {
      if (data_table[i] == 0x03)
      {
        end_index = i;

        if (string_counter == MWb)
        {
          return end_index - start_index;
        }
        else
        {
          start_index = i + 1;
        }

        string_counter++;
      }
    }

    if ((end_index < start_index) && (MWb == string_counter))
    {
      return data_table_length - start_index;
    }

    return 0;
  }

  uint8_t dummy_measurement_data[] = {NWb, MWb};
  uint8_t dummy_measurement_data_length = 2;
  return getMeasurementTextLength(formula, dummy_measurement_data, dummy_measurement_data_length);
}

uint8_t KLineKWP1281Lib::getMeasurementDecimalsFromHeader(uint8_t measurement_index, uint8_t amount_of_measurements, uint8_t *header_buffer, size_t header_buffer_size)
{
  uint8_t formula = getFormulaFromHeader(measurement_index, amount_of_measurements, header_buffer, header_buffer_size);

  return getMeasurementDecimalsFromHeader(formula);
}

uint8_t KLineKWP1281Lib::getMeasurementDecimalsFromHeader(uint8_t formula)
{
  if (formula < sizeof(KWP_decimals_table))
  {
    return KWP_decimals_table[formula];
  }

  return 0;
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::readROM(uint8_t chunk_size, uint16_t start_address, size_t &bytes_received, uint8_t *memory_buffer, uint8_t memory_buffer_size)
{
  uint8_t start_address_high_byte = start_address >> 8;
  uint8_t start_address_low_byte = start_address & 0xFF;

  uint8_t parameters[] = {chunk_size, start_address_high_byte, start_address_low_byte};
  if (!send_message(KWP_REQUEST_READ_ROM, parameters, sizeof(parameters)))
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  switch (receive_message(&bytes_received, memory_buffer, memory_buffer_size, responseTimeout))
  {
    case TYPE_ACK:
    case TYPE_REFUSE:
      show_debug_info(READ_ROM_NOT_SUPPORTED);
      return FAIL;

    case TYPE_ROM:
      show_debug_info(RECEIVED_ROM);
      return SUCCESS;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::outputTests(uint16_t &current_output_test)
{
  uint8_t parameters[] = {0x00};
  send_message(KWP_REQUEST_OUTPUT_TEST, parameters, sizeof(parameters));

  size_t bytes_received;
  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_REFUSE:
      show_debug_info(OUTPUT_TESTS_NOT_SUPPORTED);
      return FAIL;

    case TYPE_OUTPUT_TEST:
      show_debug_info(RECEIVED_OUTPUT_TEST);
      current_output_test = (receive_buffer[0] << 8) | receive_buffer[1];
      return SUCCESS;

    case TYPE_ACK:
      show_debug_info(END_OF_OUTPUT_TESTS);
      current_output_test = 0;
      return FAIL;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }
}

char *KLineKWP1281Lib::getOutputTestDescription(uint16_t output_test, char *str, size_t string_size)
{
  return getFaultDescription(output_test, str, string_size);
}

size_t KLineKWP1281Lib::getOutputTestDescriptionLength(uint16_t output_test)
{
  return getFaultDescriptionLength(output_test);
}

void KLineKWP1281Lib::bitbang_5baud(uint8_t module)
{
  _endFunction();

  pinMode(_tx_pin, OUTPUT);

  bool bits[10], parity = 1;
  for (uint8_t i = 0; i < sizeof(bits); i++)
  {
    switch (i)
    {
      case 0:
        bits[i] = 0;
        break;

      default:
        bits[i] = module & (1 << (i - 1));
        parity ^= bits[i];
        break;

      case 8:
        bits[i] = parity;
        break;

      case 9:
        bits[i] = 1;
        break;
    }
  }

  K_LOG_INFO("5-baud-init\n");
  for (uint8_t i = 0; i < sizeof(bits); i++)
  {
    digitalWrite(_tx_pin, bits[i]);

    if (_5baudWaitFunction)
    {
      _5baudWaitFunction();
    }
    else
    {
      delay(200);
    }
  }
}

KLineKWP1281Lib::executionStatus KLineKWP1281Lib::read_identification(bool request_extra_identification)
{
  size_t bytes_received;
  RETURN_TYPE ret_val = receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout);

  may_send_protocol_parameters_again = false;

  switch (ret_val)
  {
    case TYPE_ACK:
      show_debug_info(NO_PART_NUMBER_AVAILABLE);
      return SUCCESS;

    case TYPE_ID:
      show_debug_info(RECEIVED_PART_NUMBER);
      break;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }

  bool extra_identification_available = false;
  if (receive_buffer[0] & 0x80)
  {
    extra_identification_available = true;

    receive_buffer[0] &= ~(1 << 7);
  }

  memset(identification_data.part_number, 0, sizeof(identification_data.part_number));
  memcpy(identification_data.part_number, receive_buffer, bytes_received);

  if (!acknowledge())
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_ACK:
      show_debug_info(NO_ID_PART1_AVAILABLE);
      return SUCCESS;

    case TYPE_ID:
      show_debug_info(RECEIVED_ID_PART1);
      break;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }

  memset(identification_data.identification, 0, sizeof(identification_data.identification));
  memcpy(identification_data.identification, receive_buffer, bytes_received);
  size_t identification_string_index = bytes_received;

  if (!acknowledge())
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_ACK:
      show_debug_info(NO_ID_PART2_AVAILABLE);
      return SUCCESS;

    case TYPE_ID:
      show_debug_info(RECEIVED_ID_PART2);
      break;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }

  memcpy(identification_data.identification + identification_string_index, receive_buffer, bytes_received);

  if (!acknowledge())
  {
    show_debug_info(SEND_ERROR);
    return ERROR;
  }

  switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
  {
    case TYPE_ACK:
      show_debug_info(NO_CODING_WSC_AVAILABLE);
      break;

    case TYPE_ID:
      show_debug_info(RECEIVED_CODING_WSC);
      break;

    default:
      show_debug_info(UNEXPECTED_RESPONSE);
      return ERROR;
  }

  if (bytes_received == 5)
  {
    identification_data.coding = ((receive_buffer[1] << 8) | receive_buffer[2]) >> 1;
    identification_data.workshop_code = (uint32_t(receive_buffer[2] & 0x01) << 16) | uint16_t(receive_buffer[3] << 8) | receive_buffer[4];
  }
  else
  {
    identification_data.coding = identification_data.workshop_code = 0;
  }

  if (extra_identification_available && request_extra_identification)
  {
    show_debug_info(EXTRA_ID_AVAILABLE);

    if (!send_message(KWP_REQUEST_EXTRA_ID))
    {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }

    switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
    {
      case TYPE_ACK:
        show_debug_info(NO_EXTRA_ID_PART1_AVAILABLE);
        return FAIL;

      case TYPE_ID:
        show_debug_info(RECEIVED_EXTRA_ID_PART1);
        break;

      default:
        show_debug_info(UNEXPECTED_RESPONSE);
        return ERROR;
    }

    memset(identification_data.extra_identification, 0, sizeof(identification_data.extra_identification));
    memcpy(identification_data.extra_identification, receive_buffer, bytes_received);
    size_t extra_identification_string_index = bytes_received;

    if (!acknowledge())
    {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }

    switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
    {
      case TYPE_ACK:
        show_debug_info(NO_EXTRA_ID_PART2_AVAILABLE);
        return SUCCESS;

      case TYPE_ID:
        show_debug_info(RECEIVED_EXTRA_ID_PART2);
        break;

      default:
        show_debug_info(UNEXPECTED_RESPONSE);
        return ERROR;
    }

    memcpy(identification_data.extra_identification + extra_identification_string_index, receive_buffer, bytes_received);
    extra_identification_string_index += bytes_received;

    if (!acknowledge())
    {
      show_debug_info(SEND_ERROR);
      return ERROR;
    }

    switch (receive_message(&bytes_received, receive_buffer, sizeof(receive_buffer), responseTimeout))
    {
      case TYPE_ACK:
        show_debug_info(NO_EXTRA_ID_PART3_AVAILABLE);
        return SUCCESS;

      case TYPE_ID:
        show_debug_info(RECEIVED_EXTRA_ID_PART3);
        break;

      default:
        show_debug_info(UNEXPECTED_RESPONSE);
        return ERROR;
    }

    memcpy(identification_data.extra_identification + extra_identification_string_index, receive_buffer, bytes_received);
  }
  else
  {
    show_debug_info(NO_EXTRA_ID_AVAILABLE);
  }

  return SUCCESS;
}

bool KLineKWP1281Lib::receive_keywords(uint8_t *buffer)
{
  for (uint8_t i = 0; i < 2; i++)
  {
    if (!read_byte(&buffer[i], initResponseTimeout, false))
    {
      K_LOG_ERROR("Error reading KW");
      K_LOG_ERROR_(i + 1);
      K_LOG_ERROR_("\n");
      return false;
    }

    K_LOG_INFO("Received KW");
    K_LOG_INFO_(i + 1);
    K_LOG_INFO_(": ");
    K_LOG_INFO_(buffer[i] >> 4, HEX);
    K_LOG_INFO_(buffer[i] & 0xF, HEX);
    K_LOG_INFO_("\n");
  }

  return true;
}

bool KLineKWP1281Lib::read_byte(uint8_t *byte_out, unsigned long timeout_ms, bool must_send_complement)
{
  unsigned long timeout_timer = millis();

  while (!_receiveFunction(byte_out))
  {
    if (millis() - timeout_timer > timeout_ms)
    {
      K_LOG_ERROR("Reading byte timed out\n");

      if (_current_module)
      {
        error_function();
      }
      return false;
    }
  }

  last_byte_ms = millis();

  if (must_send_complement)
  {
    delay(complementDelay);
    send_complement(*byte_out);
  }

  return true;
}

void KLineKWP1281Lib::consume_UART_echo(uint8_t byte_in)
{
  unsigned long timeout_timer = millis();

  uint8_t echo_byte;
  while (!_receiveFunction(&echo_byte))
  {
    if (millis() - timeout_timer > echoTimeout)
    {
      K_LOG_ERROR("Error reading echo; check interface\n");
      return;
    }
  }

  if (echo_byte != byte_in)
  {
    K_LOG_ERROR("Echo mismatch; check interface/baud rate\n");
  }
}

void KLineKWP1281Lib::send_complement(uint8_t byte_in)
{
  uint8_t complement = byte_in ^ 0xFF;
  _sendFunction(complement);

  if (_full_duplex)
  {
    consume_UART_echo(complement);
  }
}

KLineKWP1281Lib::RETURN_TYPE KLineKWP1281Lib::receive_message(size_t *bytes_received_out, uint8_t *buffer_out, size_t buffer_size, unsigned long timeout_ms)
{
  while (true)
  {
    if (!read_byte(&receive_byte, timeout_ms, false))
    {
      K_LOG_ERROR("Response timed out\n");
      return ERROR_TIMEOUT;
    }

    if (receive_byte == 0x55 && may_send_protocol_parameters_again)
    {
      K_LOG_INFO("Got SYNC byte again\n");

      if (!receive_keywords(keyword_buffer))
      {
        break;
      }

      delay(initComplementDelay);
      send_complement(keyword_buffer[1]);
    }
    else
    {
      break;
    }
  }

  delay(complementDelay);
  send_complement(receive_byte);

  *bytes_received_out = receive_byte - 3;

  if (buffer_size < *bytes_received_out)
  {
    show_debug_info(ARRAY_NOT_LARGE_ENOUGH, *bytes_received_out);
  }

  if (!read_byte(&message_sequence, byteTimeout))
  {
    K_LOG_ERROR("Failed to receive message sequence\n");
    return ERROR_TIMEOUT;
  }

  uint8_t message_type;
  if (!read_byte(&message_type, byteTimeout))
  {
    K_LOG_ERROR("Failed to receive message type\n");
    return ERROR_TIMEOUT;
  }

  size_t buffer_index = 0;
  for (uint8_t i = 0; i < *bytes_received_out; i++)
  {
    if (!read_byte(&receive_byte, byteTimeout))
    {
      K_LOG_ERROR("Failed to receive data byte #");
      K_LOG_ERROR_(buffer_index);
      K_LOG_ERROR_("\n");
      return ERROR_TIMEOUT;
    }

    if (buffer_size - buffer_index)
    {
      buffer_out[buffer_index++] = receive_byte;
    }
  }

  if (!read_byte(&receive_byte, byteTimeout, false))
  {
    K_LOG_ERROR("Failed to receive message end byte\n");
    return ERROR_TIMEOUT;
  }
  else if (receive_byte != 0x03)
  {
    K_LOG_ERROR("Incorrect message end byte\n");
    return ERROR_TIMEOUT;
  }

  if (_debugFunction)
  {
    _debugFunction(1, message_sequence, message_type, buffer_out, *bytes_received_out);
  }

  show_debug_command_description(1, message_type);

  switch (message_type)
  {
    case KWP_RECEIVE_ID_DATA:
      return TYPE_ID;

    case KWP_ACKNOWLEDGE:
      return TYPE_ACK;

    case KWP_REFUSE:
      return TYPE_REFUSE;

    case KWP_RECEIVE_FAULT_CODES:
      return TYPE_FAULT_CODES;

    case KWP_RECEIVE_ADAPTATION:
      return TYPE_ADAPTATION;

    case KWP_RECEIVE_GROUP_HEADER:
      return TYPE_GROUP_HEADER;

    case KWP_RECEIVE_GROUP_READING:
      return TYPE_GROUP_READING;

    case KWP_RECEIVE_ROM:
      return TYPE_ROM;

    case KWP_RECEIVE_OUTPUT_TEST:
      return TYPE_OUTPUT_TEST;

    case KWP_RECEIVE_BASIC_SETTING:
      return TYPE_BASIC_SETTING;

    default:
      return ERROR_OK;
  }
}

bool KLineKWP1281Lib::acknowledge()
{
  return send_message(KWP_ACKNOWLEDGE);
}

bool KLineKWP1281Lib::send_byte(uint8_t byte_in, bool wait_for_complement)
{
  unsigned long elapsed_ms = millis() - last_byte_ms;
  if (elapsed_ms < byteDelay)
  {
    delay(byteDelay - elapsed_ms);
  }
  last_byte_ms = millis();

  _sendFunction(byte_in);

  if (_full_duplex)
  {
    consume_UART_echo(byte_in);
  }

  if (wait_for_complement)
  {
    uint8_t complement;
    if (!read_byte(&complement, complementResponseTimeout, false))
    {
      K_LOG_ERROR("Error reading complement\n");
      return false;
    }
    else if (complement != (byte_in ^ 0xFF))
    {
      K_LOG_ERROR("Incorrect complement received\n");
      return false;
    }
  }
  return true;
}

bool KLineKWP1281Lib::send_message(uint8_t message_type, uint8_t *parameters, size_t parameter_count)
{
  unsigned long elapsed_ms = millis() - last_byte_ms;
  if (elapsed_ms < blockDelay)
  {
    delay(blockDelay - elapsed_ms);
  }

  size_t message_length = 3 + parameter_count;

  if (!send_byte(message_length))
  {
    K_LOG_ERROR("Error sending message length\n");
    return false;
  }

  if (!send_byte(++message_sequence))
  {
    K_LOG_ERROR("Error sending message sequence\n");
    return false;
  }

  if (!send_byte(message_type))
  {
    K_LOG_ERROR("Error sending message type\n");
    return false;
  }

  if (parameters)
  {
    for (uint8_t i = 0; i < parameter_count; i++)
    {
      if (!send_byte(parameters[i]))
      {
        K_LOG_ERROR("Error sending data byte #");
        K_LOG_ERROR_(i);
        K_LOG_ERROR_("\n");
        return false;
      }
    }
  }

  send_byte(0x03, false);

  if (_debugFunction)
  {
    _debugFunction(0, message_sequence, message_type, parameters, parameter_count);
  }

  show_debug_command_description(0, message_type);
  return true;
}

void KLineKWP1281Lib::error_function()
{
  K_LOG_ERROR("The connection was terminated\n");

  if (!error_function_allowed)
  {
    _current_module = 0;
    return;
  }

  uint8_t reconnect_to_module = _current_module;
  _current_module = 0;

  if (_errorFunction)
  {
    K_LOG_INFO("Calling the custom error function\n");
    _errorFunction(reconnect_to_module, _current_baud_rate);
  }
  else
  {
    K_LOG_INFO("Automatically reconnecting to module ");
    K_LOG_INFO_(reconnect_to_module >> 4, HEX);
    K_LOG_INFO_(reconnect_to_module & 0xF, HEX);
    K_LOG_INFO_(" with baud rate ");
    K_LOG_INFO_(_current_baud_rate);
    K_LOG_INFO_("\n");
    connect(reconnect_to_module, _current_baud_rate, false, 0);
  }
}

void KLineKWP1281Lib::show_debug_info(DEBUG_TYPE type, uint8_t parameter)
{
  switch (type)
  {
    case UNEXPECTED_RESPONSE:
      K_LOG_ERROR("Unexpected response\n");
      break;

    case SEND_ERROR:
      K_LOG_ERROR("Sending error\n");
      break;

    case ARRAY_NOT_LARGE_ENOUGH:
      K_LOG_WARNING("The given buffer is too small for the incoming message (");
      K_LOG_WARNING_(parameter);
      K_LOG_WARNING_(")\n");
      break;

    case LOGIN_ACCEPTED:
      K_LOG_DEBUG("Login accepted\n");
      break;

    case LOGIN_NOT_ACCEPTED:
      K_LOG_DEBUG("Login not accepted\n");
      break;

    case FAULT_CODES_NOT_SUPPORTED:
      K_LOG_DEBUG("Fault codes not supported\n");
      break;

    case CLEARING_FAULT_CODES_NOT_SUPPORTED:
      K_LOG_DEBUG("Clearing fault codes not supported\n");
      break;

    case CLEARING_FAULT_CODES_ACCEPTED:
      K_LOG_DEBUG("Cleared fault codes\n");
      break;

    case INVALID_ADAPTATION_CHANNEL:
      K_LOG_ERROR("Invalid adaptation channel\n");
      break;

    case INVALID_ADAPTATION_CHANNEL_OR_VALUE:
      K_LOG_ERROR("Invalid adaptation channel or value\n");
      break;

    case ADAPTATION_RECEIVED:
      K_LOG_DEBUG("Received adaptation channel\n");
      break;

    case ADAPTATION_ACCEPTED:
      K_LOG_DEBUG("Adaptation accepted\n");
      break;

    case ADAPTATION_NOT_ACCEPTED:
      K_LOG_DEBUG("Adaptation not accepted\n");
      break;

    case INVALID_BASIC_SETTING_GROUP:
      K_LOG_ERROR("Basic settings not supported, or invalid group requested\n");
      break;

    case RECEIVED_EMPTY_BASIC_SETTING_GROUP:
      K_LOG_ERROR("Empty basic setting group\n");
      break;

    case RECEIVED_BASIC_SETTING:
      K_LOG_DEBUG("Received basic setting\n");
      break;

    case INVALID_MEASUREMENT_GROUP:
      K_LOG_ERROR("Invalid measurement group\n");
      break;

    case RECEIVED_EMPTY_GROUP:
      K_LOG_DEBUG("Empty measurement group\n");
      break;

    case RECEIVED_GROUP_HEADER:
      K_LOG_DEBUG("Received measurement group header\n");
      break;

    case INVALID_GROUP_HEADER_MAP_LENGTH:
      K_LOG_ERROR("Unexpected header table length (");
      K_LOG_ERROR_(parameter);
      K_LOG_ERROR_(")\n");
      break;

    case RECEIVED_GROUP_BODY_OR_BASIC_SETTING:
      K_LOG_DEBUG("Received measurement group body or basic setting\n");
      break;

    case INVALID_GROUP_BODY_OR_BASIC_SETTING_LENGTH:
      K_LOG_ERROR("Expected 10/<4 bytes, got ");
      K_LOG_ERROR_(parameter);
      K_LOG_ERROR_("\n");
      break;

    case RECEIVED_GROUP:
      K_LOG_DEBUG("Received measurement group\n");
      break;

    case READ_ROM_NOT_SUPPORTED:
      K_LOG_ERROR("Reading ROM not supported, or invalid parameters given\n");
      break;

    case RECEIVED_ROM:
      K_LOG_DEBUG("Received ROM reading\n");
      break;

    case NO_PART_NUMBER_AVAILABLE:
      K_LOG_DEBUG("No part number available\n");
      break;

    case RECEIVED_PART_NUMBER:
      K_LOG_DEBUG("Received part number\n");
      break;

    case NO_ID_PART1_AVAILABLE:
      K_LOG_DEBUG("No identification available (part 1)\n");
      break;

    case RECEIVED_ID_PART1:
      K_LOG_DEBUG("Received identification (part 1)\n");
      break;

    case NO_ID_PART2_AVAILABLE:
      K_LOG_DEBUG("No identification available (part 2)\n");
      break;

    case RECEIVED_ID_PART2:
      K_LOG_DEBUG("Received identification (part 2)\n");
      break;

    case NO_CODING_WSC_AVAILABLE:
      K_LOG_DEBUG("No coding/WSC available\n");
      break;

    case RECEIVED_CODING_WSC:
      K_LOG_DEBUG("Received coding/WSC\n");
      break;

    case EXTRA_ID_AVAILABLE:
      K_LOG_DEBUG("Extra identification available\n");
      break;

    case NO_EXTRA_ID_AVAILABLE:
      K_LOG_DEBUG("No extra identification available or requested\n");
      break;

    case NO_EXTRA_ID_PART1_AVAILABLE:
      K_LOG_DEBUG("No extra identification available (part 1)\n");
      break;

    case RECEIVED_EXTRA_ID_PART1:
      K_LOG_DEBUG("Received extra identification (part 1)\n");
      break;

    case NO_EXTRA_ID_PART2_AVAILABLE:
      K_LOG_DEBUG("No extra identification available (part 2)\n");
      break;

    case RECEIVED_EXTRA_ID_PART2:
      K_LOG_DEBUG("Received extra identification (part 2)\n");
      break;

    case NO_EXTRA_ID_PART3_AVAILABLE:
      K_LOG_DEBUG("No extra identification available (part 3)\n");
      break;

    case RECEIVED_EXTRA_ID_PART3:
      K_LOG_DEBUG("Received extra identification (part 3)\n");
      break;

    case OUTPUT_TESTS_NOT_SUPPORTED:
      K_LOG_DEBUG("Output tests not supported\n");
      break;

    case RECEIVED_OUTPUT_TEST:
      K_LOG_DEBUG("Performing output test\n");
      break;

    case END_OF_OUTPUT_TESTS:
      K_LOG_DEBUG("End of output tests\n");
      break;

    case DISCONNECT_INFO:
      K_LOG_DEBUG("A \"timed out\" error is normal here\n");
      break;
  }

  (void)parameter;
}

void KLineKWP1281Lib::show_debug_command_description(bool direction, uint8_t command)
{
  switch (command)
  {
    case KWP_ACKNOWLEDGE:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Acknowledgement\"\n");
      break;

    case KWP_REFUSE:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Refuse\"\n");
      break;

    case KWP_DISCONNECT:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Disconnect\"\n");
      break;

    case KWP_REQUEST_EXTRA_ID:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Request extra identification\"\n");
      break;

    case KWP_REQUEST_LOGIN:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Login\"\n");
      break;

    case KWP_REQUEST_RECODE:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Recode\"\n");
      break;

    case KWP_REQUEST_FAULT_CODES:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Request fault codes\"\n");
      break;

    case KWP_REQUEST_CLEAR_FAULTS:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Clear fault codes\"\n");
      break;

    case KWP_REQUEST_ADAPTATION:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Request adaptation value\"\n");
      break;

    case KWP_REQUEST_ADAPTATION_TEST:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Test adaptation value\"\n");
      break;

    case KWP_REQUEST_ADAPTATION_SAVE:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Save adaptation value\"\n");
      break;

    case KWP_REQUEST_BASIC_SETTING:
    case KWP_REQUEST_BASIC_SETTING_0:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Request basic setting\"\n");
      break;

    case KWP_REQUEST_GROUP_READING:
    case KWP_REQUEST_GROUP_READING_0:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Request group reading\"\n");
      break;

    case KWP_REQUEST_READ_ROM:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Request ROM reading\"\n");
      break;

    case KWP_REQUEST_OUTPUT_TEST:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Request output test\"\n");
      break;

    case KWP_RECEIVE_ID_DATA:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Provide identification\"\n");
      break;

    case KWP_RECEIVE_FAULT_CODES:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Provide fault codes\"\n");
      break;

    case KWP_RECEIVE_ADAPTATION:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Provide adaptation value\"\n");
      break;

    case KWP_RECEIVE_BASIC_SETTING:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Provide basic setting/group body\"\n");
      break;

    case KWP_RECEIVE_GROUP_HEADER:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Provide group header\"\n");
      break;

    case KWP_RECEIVE_GROUP_READING:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Provide group reading\"\n");
      break;

    case KWP_RECEIVE_ROM:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Provide ROM reading\"\n");
      break;

    case KWP_RECEIVE_OUTPUT_TEST:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Execute output test\"\n");
      break;

    default:
      if (direction) {K_LOG_DEBUG("Received ");} else {K_LOG_DEBUG("Sent ");}
      K_LOG_DEBUG_("\"Unknown\"\n");
      break;
  }

  (void)direction;
}

uint8_t KLineKWP1281Lib::get_measurement_length(uint8_t *buffer, uint8_t bytes_received, uint8_t buffer_index)
{
  uint8_t formula = buffer[buffer_index];

  if (formula == 0x5F || formula == 0x76)
  {
    return buffer[buffer_index + 1] + 2;
  }
  else if (formula == 0x3F)
  {
    if (bytes_received)
    {
      return bytes_received - buffer_index;
    }
    else
    {
      return 0;
    }
  }
  else if (formula == 0xA0)
  {
    return 6;
  }

  return 3;
}

double KLineKWP1281Lib::ToSigned(double MW)
{
  return MW <= 127 ? MW : MW - 256;
}

double KLineKWP1281Lib::ToSigned(double NW, double MW)
{
  double value = NW * 256 + MW;
  return value <= 32767 ? value : value - 65536;
}

double KLineKWP1281Lib::To16Bit(double NW, double MW)
{
  return NW * 256 + MW;
}

int KLineKWP1281Lib::compare_keyed_structs(const void *a, const void *b)
{
  keyed_struct struct_from_PGM;
  memcpy_P((void*)&struct_from_PGM, b, sizeof(keyed_struct));

  int idA = ((struct keyed_struct *)a)->code;
  int idB = struct_from_PGM.code;
  return (idA - idB);
}

const double KLineKWP1281Lib::negative_pow_10[] = {1, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001, 0.0000001};
