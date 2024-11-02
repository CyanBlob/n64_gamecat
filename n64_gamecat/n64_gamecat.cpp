/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "blink.pio.h"
//#include "cmake-build-release/blink.pio.h"
//#include "cmake-build-release/demo.pio.h"
//#include "cmake-build-release/demo_read.pio.h"
#include "cmake-build-debug/blink.pio.h"
#include "cmake-build-debug/demo.pio.h"
#include "cmake-build-debug/demo_read.pio.h"
#include "Wire.h"
#include "gpio_cfg.h"
#include <cinttypes>
#include "state.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq);

// by default flash leds on gpios 3-4
#ifndef PIO_BLINK_LED1_GPIO
#define PIO_BLINK_LED1_GPIO 2
#endif
#define READ_LED_START 16

// and flash leds on gpios 5-6
// or if the device supports more than 32 gpios, flash leds on 32-33
#ifndef PIO_BLINK_LED3_GPIO
#if NUM_BANK0_GPIOS <= 32
#define PIO_BLINK_LED3_GPIO 20
#else
#define PIO_BLINK_LED3_GPIO 32
#endif
#endif

typedef struct {
    uint16_t val_h;
    uint16_t val_l;
} val_parts_t;

typedef union {
    val_parts_t valParts;
    uint32_t val;

} val_t;

static uint64_t console_mask = 0;
static uint64_t cartridge_mask = 0;
static uint64_t control_mask = 0;

static State state = State::IDLE;
static val_t addr  = {0};
static val_t data = {0};

std::vector<Wire> InitWires();

// console "talking"
void set_console() {
    gpio_set_dir_in_masked64(control_mask);
    gpio_set_dir_out_masked64(cartridge_mask);
}

// cartridge "talking"
void set_cartridge() {
    gpio_set_dir_out_masked64(control_mask);
    gpio_set_dir_in_masked64(cartridge_mask);
}

void init_gpio() {
    for (uint i = 0; i < 16; ++i) {
        console_mask   |= (uint64_t) 1 << (uint64_t)console_pins[i];
        cartridge_mask |= (uint64_t) 1 << (uint64_t)cartridge_pins[i];

        gpio_init(console_pins[i]);
        gpio_init(cartridge_pins[i]);

        gpio_set_pulls(console_pins[i], true, false);
        gpio_set_pulls(cartridge_pins[i], true, false);
    }

    control_mask |= (uint64_t) 1 << ALE_H;
    control_mask |= (uint64_t) 1 << ALE_L;
    control_mask |= (uint64_t) 1 << READ;
    control_mask |= (uint64_t) 1 << WRITE;

    gpio_init(ALE_H);
    gpio_init(ALE_L);
    gpio_init(READ);
    gpio_init(WRITE);

    printf("Console   mask: 0x%" PRIx64 "\n", console_mask);
    printf("Cartridge mask: 0x%" PRIx64 "\n", cartridge_mask);
    printf("Control   mask: 0x%" PRIx64 "\n", control_mask);

    printf("uint size: %d\n", sizeof(uint));

    gpio_set_dir_in_masked64(control_mask);
    set_console();

    gpio_set_pulls(ALE_H, true, false);
    gpio_set_pulls(ALE_L, true, false);
    gpio_set_pulls(READ, true, false);
    gpio_set_pulls(WRITE, true, false);

    gpio_set_mask64(console_mask);
}

int main() {
    set_sys_clock_khz(300000, true);

    setup_default_uart();

    init_gpio();

    uint32_t control_signals = 0;
    while(true) {
        control_signals = (gpio_get_all64() >> ALE_H) & control_mask;

        switch (state) {

            case IDLE:
                if ((control_signals & 0b1110) == 0) {
                    set_console();
                    sleep_us(10);
                    state = ADDR_H;
                    addr.valParts.val_h = (gpio_get_all64() & console_mask) >> 2; // TODO: Update for v0.8
                }
                else if ((control_signals & 0b0111) == 0) {
                    state = READ_H;
                }
            case ADDR_H:
                if ((control_signals & 0b1101) > 0) {
                    state = ADDR_L;
                    addr.valParts.val_l = (gpio_get_all64() & console_mask) >> 2; // TODO: Update for v0.8

                    set_cartridge();
                }
                break;
            case ADDR_L:
                if ((control_signals & 0b0111) == 0) {
                    state = READ_H;
                }
                break;
            case READ_H:
                if ((control_signals & 0b1111) == 0) {
                    state = READ_IDLE;
                    data.valParts.val_h = (gpio_get_all64() & cartridge_mask) >> 14; // TODO: Update for v0.8
                }
                break;
            case READ_IDLE:
                if ((control_signals & 0b0111) == 0) {
                    state = READ_L;
                }
                break;
            case READ_L:
                if ((control_signals & 0b1111) == 0) {
                    state = IDLE;
                    data.valParts.val_l = (gpio_get_all64() & cartridge_mask) >> 14; // TODO: Update for v0.8
                }
                break;
            case WRITE_H:
                break;
            case WRITE_L:
                break;
        }
    }

    while(true) {
        set_console();
        sleep_ms(50);
        printf("GPIOs (CON): 0x%" PRIx64 "\n", gpio_get_all64());
        set_cartridge();
        sleep_ms(50);
        printf("GPIOs (CAR): 0x%" PRIx64 "\n", gpio_get_all64());
    }

    return 0;
}

std::vector<Wire> InitWires()
{
    std::vector<Wire> wires;

    Wire ale_l;
    auto ale_l_timings = std::vector<uint32_t>();

    ale_l_timings.push_back(0);
    ale_l_timings.push_back(300);
    ale_l_timings.push_back(550);
    ale_l.SetTimings(ale_l_timings);

    wires.push_back(ale_l);

    Wire ale_h;
    auto ale_h_timings = std::vector<uint32_t>();

    ale_h_timings.push_back(425);
    ale_h.SetTimings(ale_h_timings);

    wires.push_back(ale_h);

    Wire read;
    auto read_timings = std::vector<uint32_t>();

    read_timings.push_back(1600);
    read_timings.push_back(1900);
    read_timings.push_back(1960);
    read_timings.push_back(2260);
    read_timings.push_back(2320);
    read_timings.push_back(2620);
    read.SetTimings(read_timings);

    wires.push_back(read);
    return wires;
}
