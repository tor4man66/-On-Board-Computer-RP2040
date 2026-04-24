#ifndef _PICO_SLEEP_H_
#define _PICO_SLEEP_H_

#include "pico.h"
#include "hardware/timer.h"
#include "pico/aon_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DORMANT_SOURCE_NONE,
    DORMANT_SOURCE_XOSC,
    DORMANT_SOURCE_ROSC,
    DORMANT_SOURCE_LPOSC,
} dormant_source_t;

void sleep_run_from_dormant_source(dormant_source_t dormant_source);

static inline void sleep_run_from_xosc(void) {
    sleep_run_from_dormant_source(DORMANT_SOURCE_XOSC);
}

#if !PICO_RP2040
static inline void sleep_run_from_lposc(void) {
    sleep_run_from_dormant_source(DORMANT_SOURCE_LPOSC);
}
#endif

static inline void sleep_run_from_rosc(void) {
    sleep_run_from_dormant_source(DORMANT_SOURCE_ROSC);
}

void sleep_goto_sleep_until(struct timespec *ts, aon_timer_alarm_handler_t callback);
bool sleep_goto_sleep_for(uint32_t delay_ms, hardware_alarm_callback_t callback);
void sleep_goto_dormant_until(struct timespec *ts, aon_timer_alarm_handler_t callback);
void sleep_goto_dormant_until_pin(uint gpio_pin, bool edge, bool high);

static inline void sleep_goto_dormant_until_edge_high(uint gpio_pin) {
    sleep_goto_dormant_until_pin(gpio_pin, true, true);
}

static inline void sleep_goto_dormant_until_level_high(uint gpio_pin) {
    sleep_goto_dormant_until_pin(gpio_pin, false, true);
}

void sleep_power_up(void);

#ifdef __cplusplus
}
#endif

#endif