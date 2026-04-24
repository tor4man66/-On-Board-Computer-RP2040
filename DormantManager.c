#include <stdio.h>
#include <inttypes.h>

#include "pico.h"
#include "pico/stdlib.h"
#include "DormantManager.h"

#include "hardware/pll.h"
#include "hardware/regs/clocks.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/xosc.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/sync.h"
#include "pico/runtime_init.h"

static dormant_source_t _dormant_source;

void sleep_run_from_dormant_source(dormant_source_t dormant_source) {
    _dormant_source = dormant_source;
    uint src_hz = (dormant_source == DORMANT_SOURCE_XOSC) ? XOSC_HZ : 6500 * KHZ;
    uint clk_ref_src = (dormant_source == DORMANT_SOURCE_XOSC) ? 
                       CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC : 
                       CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH;

    clock_configure(clk_ref, clk_ref_src, 0, src_hz, src_hz);
    clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, src_hz, src_hz);
    clock_stop(clk_adc);
    clock_stop(clk_usb);
    clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, src_hz, src_hz);
    pll_deinit(pll_sys);
    pll_deinit(pll_usb);
}

static void processor_deep_sleep(void) {
    scb_hw->scr |= M0PLUS_SCR_SLEEPDEEP_BITS;
}

static void _go_dormant(void) {
    if (_dormant_source == DORMANT_SOURCE_XOSC) {
        xosc_dormant();
    }
}

void sleep_goto_dormant_until_pin(uint gpio_pin, bool edge, bool high) {
    bool low = !high;
    bool level = !edge;
    uint32_t event = 0;
    if (level && low) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_LOW_BITS;
    if (level && high) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_HIGH_BITS;
    if (edge && high) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_HIGH_BITS;
    if (edge && low) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_LOW_BITS;

    gpio_init(gpio_pin);
    gpio_set_input_enabled(gpio_pin, true);
    gpio_set_dormant_irq_enabled(gpio_pin, event, true);
    _go_dormant();
    gpio_acknowledge_irq(gpio_pin, event);
    gpio_set_input_enabled(gpio_pin, false);
}

void sleep_power_up(void) {
    clocks_hw->sleep_en0 |= ~(0u);
    clocks_hw->sleep_en1 |= ~(0u);
    clocks_init();
}