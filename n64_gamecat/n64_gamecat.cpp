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
#include "cmake-build-debug/blink.pio.h"
#include "cmake-build-debug/demo.pio.h"
#include "cmake-build-debug/demo_read.pio.h"
#include "Wire.h"

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

std::vector<Wire> InitWires();

int main() {
    set_sys_clock_khz(266000, true);

    setup_default_uart();

    assert(PIO_BLINK_LED1_GPIO < 31);
    assert(PIO_BLINK_LED3_GPIO < 31 || PIO_BLINK_LED3_GPIO >= 32);

    uint offset = pio_add_program(pio0, &demo_program);
    demo_program_init(pio0, 0, offset, PIO_BLINK_LED1_GPIO);
    pio_sm_set_enabled(pio0, 0, true);

    printf("Done setting up first PIO\n");

    offset = pio_add_program(pio1, &demo_program);
    demo_read_program_init(pio1, 0, offset, READ_LED_START);
    pio_sm_set_enabled(pio1, 0, true);

    printf("Done setting up second PIO\n");

    printf("Init complete\n");

    auto wires = InitWires();

    //gpio_init(28);
    //gpio_set_dir(28, true);
    //gpio_put(28, true);

    uint32_t max_time = 2500;

    gpio_pull_up(28);

    for (uint32_t i = 0; i < UINT32_MAX; ++i) {
        while (pio_sm_is_tx_fifo_full(pio0, 0)) {
            printf("FULL 0\n");
        }
        while (pio_sm_is_tx_fifo_full(pio1, 0)) {
            printf("FULL 1\n");
        }
        pio_sm_put(pio0, 0, i);

        for(auto &wire : wires)
        {
            wire.Tick();
        }

        uint32_t data = 0;
        uint8_t index = 0;
        for(auto &wire : wires)
        {
            data |= wire.GetState() << index++;
        }

        uint32_t val = 0xFFFF;
        data |= (val << 4);

        //printf("%lu: %lu\n", i, data);

        pio_sm_put(pio1, 0, data);

        if (i > max_time)
        {
            printf("RESET\n");
            i = 0;
            for(auto &wire : wires)
            {
                wire.Reset();
            }
        }

        sleep_ms(10);
    }
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
