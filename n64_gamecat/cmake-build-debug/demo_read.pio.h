// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// --------- //
// demo_read //
// --------- //

#define demo_read_wrap_target 0
#define demo_read_wrap 1
#define demo_read_pio_version 0

static const uint16_t demo_read_program_instructions[] = {
            //     .wrap_target
    0x80a0, //  0: pull   block                      
    0x6010, //  1: out    pins, 16                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program demo_read_program = {
    .instructions = demo_read_program_instructions,
    .length = 2,
    .origin = -1,
    .pio_version = 0,
#if PICO_PIO_VERSION > 0
    .used_gpio_ranges = 0x0
#endif
};

static inline pio_sm_config demo_read_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + demo_read_wrap_target, offset + demo_read_wrap);
    return c;
}

// this is a raw helper function for use by the user which sets up the GPIO output, and configures the SM to output on a particular pin
void demo_read_program_init(PIO pio, uint sm, uint offset, uint pin) {
    for (int i = 0; i <= 16; ++i)
    {
       pio_gpio_init(pio, pin + i);
    }
   pio_sm_set_consecutive_pindirs(pio, sm, pin, 16, true);
   pio_sm_config c = demo_read_program_get_default_config(offset);
   sm_config_set_out_pins(&c, pin, 16);
   pio_sm_init(pio, sm, offset, &c);
}

#endif

