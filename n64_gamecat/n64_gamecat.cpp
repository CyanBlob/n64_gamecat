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

#define NUM_SAMPLES 1000


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

static State state = State::IDLE;
static val_t addr  = {0};
static val_t data = {0};
static uint32_t regs_low = 0;
static uint32_t regs_high = 0;

volatile bool timer_fired = false;

int64_t alarm_callback(alarm_id_t id, __unused void *user_data) {
    timer_fired = true;
    return 0;
}

void set_gpio_function(uint8_t pin, uint8_t function) {
    volatile auto *gpio_ctrl = (volatile uint32_t *)(IO_BANK0_BASE + 0x04 + (pin * 8));
    *gpio_ctrl = (*gpio_ctrl & ~0x1F) | (function & 0x1F);
}

void configure_gpio_pins() {
    for (uint8_t pin = 2; pin <= 40; pin++) {
        set_gpio_function(pin, 5); // 5 corresponds to SIO function
    }
}

void read_all_gpio_pins(uint32_t *bank0, uint32_t *bank1) {
    *bank0 = GPIO_IN_BANK0;
    *bank1 = GPIO_IN_BANK1;
}

void core1_uart_entry() {
    printf("CORE1 entry\n");

    uint32_t lastVal = UINT32_MAX;

    while (true) {
        auto val = multicore_fifo_pop_blocking();
        //if (val != lastVal) {
            printf("%lu\n", val);
            //lastVal = val;
        //}
    }
}

// console "talking"
void set_console() {
    gpio_set_dir_in_masked64(console_mask);
    gpio_set_dir_out_masked64(cartridge_mask);
    gpio_set_dir_in_masked64(control_mask);

    //GPIO_OE_BANK0 = full_mask_co;
    //GPIO_OE_BANK1 = full_mask_co >> 32;

    //printf("Output mask 0: 0x%" PRIx32 "\n", GPIO_OE_BANK0);
    //printf("Output mask 1: 0x%" PRIx32 "\n", GPIO_OE_BANK1);
}

// cartridge "talking"
void set_cartridge() {
    gpio_set_dir_out_masked64(console_mask);
    gpio_set_dir_in_masked64(cartridge_mask);
    gpio_set_dir_in_masked64(control_mask);

    //GPIO_OE_BANK0 = full_mask_ca;
    //GPIO_OE_BANK1 = full_mask_ca >> 32;

    //printf("Output mask 0: 0x%" PRIx32 "\n", GPIO_OE_BANK0);
    //printf("Output mask 1: 0x%" PRIx32 "\n", GPIO_OE_BANK1);
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

    gpio_init(ALE_H);
    gpio_init(ALE_L);
    gpio_init(READ);
    gpio_init(WRITE);

    gpio_init(GPIO_FLAG);

    printf("Console   mask: 0x%" PRIx64 "\n", console_mask);
    printf("Cartridge mask: 0x%" PRIx64 "\n", cartridge_mask);
    printf("Control   mask: 0x%" PRIx64 "\n", control_mask);

    printf("uint size: %d\n", sizeof(uint));

    gpio_set_dir_in_masked64(control_mask);

    full_mask_co = cartridge_mask | control_mask;
    full_mask_ca = console_mask   | control_mask;

    full_mask_co |= ((uint64_t)1 << GPIO_FLAG);
    full_mask_ca |= ((uint64_t)1 << GPIO_FLAG);

    printf("Full CO   mask: 0x%" PRIx64 "\n", full_mask_co);
    printf("Full CA   mask: 0x%" PRIx64 "\n", full_mask_ca);

    gpio_set_pulls(ALE_H, true, false);
    gpio_set_pulls(ALE_L, true, false);
    gpio_set_pulls(READ, true, false);
    gpio_set_pulls(WRITE, true, false);

    gpio_set_pulls(GPIO_FLAG, true, false);
    gpio_set_dir(GPIO_FLAG, true);

    //gpio_set_mask64(console_mask);
}

int main() {
    //set_sys_clock_khz(300000, true);
    set_sys_clock_khz(300000, true);

    stdio_init_all();

    //multicore_reset_core1();

    init_gpio();

    uint32_t control_signals = 0;

    stdio_flush();

    //multicore_launch_core1(core1_uart_entry);

    //multicore_fifo_push_blocking(0xDEADBEEF);
    //multicore_fifo_push_blocking(0xDEADBABE);
    //multicore_fifo_push_blocking(0xFFFFFFFF);
    //multicore_fifo_push_blocking(0x00000000);
    //multicore_fifo_push_blocking(1234);

    set_console();
    uint64_t tmp_data = 0;

    uint32_t lastUpper = UINT32_MAX;
    uint32_t lastLower = UINT32_MAX;

    bool flag = false;

    uint32_t buff[NUM_SAMPLES] = {0};
    uint32_t sample = 0;

    add_alarm_in_ms(5000, alarm_callback, NULL, false);

    while(true) {
        //control_signals = (gpio_get_all64() >> ALE_H) & control_mask;
        //control_signals = gpio_get_all64();
        uint64_t val = gpio_get_all64() & 0xFFFFFEFFFFFFFFFC; // discard lowest 2 bits (UART) and bit 40 (flag)

        regs_low = (val & 0xFFFFFFFC);
        regs_high = (val >> 32);

        if (regs_low != lastLower || regs_high != lastUpper) {
            buff[sample++] = regs_low;
            buff[sample++] = regs_high;
            //multicore_fifo_push_blocking(regs_low);
            //multicore_fifo_push_blocking(regs_high);

            //printf("%lu\n", regs_low);
            //printf("%lu\n", regs_high);

            lastLower = regs_low;
            lastUpper = regs_high;
            //printf("0x%" PRIx64 "\n", val);
        }
        flag = !flag;
        gpio_put(GPIO_FLAG, flag);

        if (timer_fired || sample >= NUM_SAMPLES) {
            printf("DONE!\n");
            for (int i = 0; i < sample; ++i) {
                printf("%lu\n", buff[i]);
            }

            while(true){
                flag = !flag;
                gpio_put(GPIO_FLAG, flag);
            }
        }

        //sleep_us(1000);
        continue;

        switch (state) {

            case IDLE:
                //multicore_fifo_push_blocking(IDLE);
                if ((control_signals & 0b1110) == 0b1110) {
                    set_console();
                    sleep_us(10);
                    state = ADDR_H;
                    addr.valParts.val_h = (gpio_get_all64() & console_mask) >> 2;
                    //multicore_fifo_push_blocking(addr.valParts.val_h);
                }
                else if ((control_signals & 0b0111) == 0b0111) {
                    set_cartridge();
                    state = READ_H;
                }
                break;
            case ADDR_H:
                multicore_fifo_push_blocking(ADDR_H);
                if ((control_signals & 0b1101) == 0b1101) {
                    state = ADDR_L;
                    addr.valParts.val_l = (gpio_get_all64() & console_mask) >> 2;

                    //multicore_fifo_push_blocking(addr.valParts.val_l);
                }
                break;
            case ADDR_L:
                multicore_fifo_push_blocking(ADDR_L);
                if ((control_signals & 0b0111) == 0b0111) {

                    set_cartridge();
                    state = READ_H;
                }
                break;
            case READ_H:
                multicore_fifo_push_blocking(READ_H);
                if ((control_signals & 0b1111) == 0b1111) {
                    state = READ_IDLE;
                    data.valParts.val_h = (gpio_get_all64() & cartridge_mask) >> 14; // TODO: Update for v0.8
                }
                break;
            case READ_IDLE:
                multicore_fifo_push_blocking(READ_IDLE);
                if ((control_signals & 0b0111) == 0b0111) {
                    set_cartridge();
                    state = READ_L;
                }
                break;
            case READ_L:
                multicore_fifo_push_blocking(READ_L);
                if ((control_signals & 0b1111) == 0b1111) {
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

#pragma clang diagnostic pop