/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
// IMPORTANT
// PICO_USE_GPIO_COPROCESSOR from gpio.h in the pico sdk should be set to 0

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "Wire.h"
#include "gpio_cfg.h"
#include <cinttypes>
#include "state.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

//#define SIO_BASE            0xD0000000
#define GPIO_IN_OFFSET      0x04
#define GPIO_OUT_OFFSET     0x10
#define GPIO_OE_OFFSET      0x30

#define GPIO_IN             (*(volatile uint32_t *)(SIO_BASE + GPIO_IN_OFFSET))
#define GPIO_OUT            (*(volatile uint32_t *)(SIO_BASE + GPIO_OUT_OFFSET))
#define GPIO_OE             (*(volatile uint32_t *)(SIO_BASE + GPIO_OE_OFFSET))

#define GPIO_OE_BANK0   (*(volatile uint32_t *)(SIO_BASE + GPIO_OE_OFFSET))
#define GPIO_OE_BANK1   (*(volatile uint32_t *)(SIO_BASE + GPIO_OE_OFFSET + 0x04))

#define GPIO_IN_BANK0   (*(volatile uint32_t *)(SIO_BASE + GPIO_IN_OFFSET))
#define GPIO_IN_BANK1   (*(volatile uint32_t *)(SIO_BASE + GPIO_IN_OFFSET + 0x04)) // Adjust the offset as per the datasheet

#define GPIO_OUT_BANK0   (*(volatile uint32_t *)(SIO_BASE + GPIO_OUT_OFFSET))
#define GPIO_OUT_BANK1   (*(volatile uint32_t *)(SIO_BASE + GPIO_OUT_OFFSET + 0x04)) // Adjust the offset as per the datasheet

#define NUM_SAMPLES 5000


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
static uint64_t full_mask_co = 0;
static uint64_t full_mask_ca = 0;
static uint64_t flag_mask = 0;

static State state = State::IDLE;
static val_t addr  = {0};
static val_t data = {0};
static uint32_t regs_low = 0;
static uint32_t regs_high = 0;

volatile uint64_t curr_val = 0;

static bool console_talking = true;

// console "talking"
__attribute__((always_inline))
inline void set_console() {
    gpio_set_dir_in_masked64(console_mask);
    gpio_set_dir_out_masked64(cartridge_mask);
    console_talking = true;
}

// cartridge "talking"
__attribute__((always_inline))
inline void set_cartridge() {
    gpio_set_dir_out_masked64(console_mask);
    gpio_set_dir_in_masked64(cartridge_mask);
    console_talking = false;
}

__attribute__((always_inline))
inline void dump_to_console(uint64_t val) {
    gpio_put_masked64(console_mask, (val & cartridge_mask) << 16);
    gpio_put_masked64(flag_mask, (val & control_mask) << 6);
}

__attribute__((always_inline))
inline void dump_to_cartridge(uint64_t val) {
    gpio_put_masked64(flag_mask, (val & control_mask) << 6);

}

void init_gpio() {
    //configure_gpio_pins();
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

    flag_mask |= (uint64_t) 1 << FLAG_ALE_H;
    flag_mask |= (uint64_t) 1 << FLAG_ALE_L;
    flag_mask |= (uint64_t) 1 << FLAG_READ;
    flag_mask |= (uint64_t) 1 << FLAG_WRITE;

    gpio_init(ALE_H);
    gpio_init(ALE_L);
    gpio_init(READ);
    gpio_init(WRITE);

    gpio_init(FLAG_ALE_H);
    gpio_init(FLAG_ALE_L);
    gpio_init(FLAG_WRITE);
    gpio_init(FLAG_READ);

    printf("Console   mask: 0x%" PRIx64 "\n", console_mask);
    printf("Cartridge mask: 0x%" PRIx64 "\n", cartridge_mask);
    printf("Control   mask: 0x%" PRIx64 "\n", control_mask);

    printf("uint size: %d\n", sizeof(uint));

    gpio_set_dir_in_masked64(control_mask);

    //full_mask_co = cartridge_mask | control_mask;
    //full_mask_ca = console_mask   | control_mask;

    //full_mask_co |= ((uint64_t)1 << GPIO_FLAG);
    //full_mask_ca |= ((uint64_t)1 << GPIO_FLAG);

    printf("Full CO   mask: 0x%" PRIx64 "\n", full_mask_co);
    printf("Full CA   mask: 0x%" PRIx64 "\n", full_mask_ca);

    gpio_set_pulls(ALE_H, true, false);
    gpio_set_pulls(ALE_L, true, false);
    gpio_set_pulls(READ, true, false);
    gpio_set_pulls(WRITE, true, false);

    gpio_set_pulls(FLAG_ALE_H, true, false);
    gpio_set_pulls(FLAG_ALE_L, true, false);
    gpio_set_pulls(FLAG_WRITE, true, false);
    gpio_set_pulls(FLAG_READ, true, false);

    gpio_set_dir(FLAG_ALE_H, true);
    gpio_set_dir(FLAG_ALE_L, true);
    gpio_set_dir(FLAG_WRITE, true);
    gpio_set_dir(FLAG_READ, true);

    //gpio_set_mask64(console_mask);
}

void core1_loop() {
    printf("CORE1\n");
    while (true) {
        //if (console_talking) {
            dump_to_cartridge(curr_val);
        //} else {
            //dump_to_console(curr_val);
        //}
    }
}

int main() {
    //set_sys_clock_khz(320000, true);
    set_sys_clock_khz(280000, true);

    stdio_init_all();

    init_gpio();

    uint32_t control_signals = 0;

    multicore_reset_core1();
    multicore_launch_core1(core1_loop);

    set_console();

    while(true) {
        curr_val = gpio_get_all64() /*& 0xFFFFFEFFFFFFFFFC*/; // discard lowest 2 bits (UART) and bit 40 (flag)
        //control_signals = (val & control_mask) >> ALE_H;
        //dump_to_cartridge(val);

        //control_signals = (val & control_mask) >> ALE_H;
        //dump_to_cartridge(val);
        continue;

        switch (state) {
            case IDLE:
                dump_to_cartridge(curr_val);
                if ((control_signals & 0b1110) == 0b1110) {
                    //set_console();
                    state = ADDR_H;
                }
                else if ((control_signals & 0b0111) == 0b0111) {
                    //set_cartridge();
                    state = READ_H;
                    //dump_to_console(val);
                }
                break;
            case ADDR_H:
                //printf("1");
                dump_to_cartridge(curr_val);
                if ((control_signals & 0b1101) == 0b1101) {
                    state = ADDR_L;
                }
                break;
            case ADDR_L:
                //printf("2");
                dump_to_cartridge(curr_val);
                if ((control_signals & 0b0111) == 0b0111) {
                    //set_cartridge();
                    state = READ_H;
                }
                break;
            case READ_H:
                //printf("3");
                dump_to_console(curr_val);
                if ((control_signals & 0b1111) == 0b1111) {
                    state = READ_IDLE;
                }
                break;
            case READ_IDLE:
                //printf("4");
                dump_to_console(curr_val);
                if ((control_signals & 0b0111) == 0b0111) {
                    //set_cartridge();
                    state = READ_L;
                }
                break;
            case READ_L:
                //printf("5");
                dump_to_console(curr_val);
                if ((control_signals & 0b1111) == 0b1111) {
                    state = IDLE;
                }
                break;
            case WRITE_H:
                break;
            case WRITE_L:
                break;
        }
    }
}
#pragma clang diagnostic pop