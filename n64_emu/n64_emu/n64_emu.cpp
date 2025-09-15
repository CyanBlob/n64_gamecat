#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/dma.h"

#include "blink.pio.h"
#include "counter.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// Pin groups requested by user
static const uint CA_BASE = 2;   // pins 2..17 (we drive 8 pins starting at base)
static const uint CO_BASE = 18;  // pins 18..33
static const uint SIG_BASE = 34; // pins 34..43

// PIO / DMA configuration
// We'll allocate SMs potentially across multiple PIO instances
static PIO group_pio[3];
static uint group_sm[3];
static uint group_offset[3];
static int dma_chan[3];

// 256-entry buffer (values 0..255)
static uint32_t dma_buf[256];

// Helper to initialize a PIO SM for 8-bit out on chosen PIO/sm
static void init_out_sm(PIO pio_choice, uint sm, uint offset, uint base_pin) {
    pio_sm_config cfg = out_byte_program_get_default_config(offset);
    sm_config_set_out_pins(&cfg, base_pin, 8);
    sm_config_set_out_shift(&cfg, true, true, 8);
    for (uint i = 0; i < 8; ++i) pio_gpio_init(pio_choice, base_pin + i);
    pio_sm_set_consecutive_pindirs(pio_choice, sm, base_pin, 8, true);
    pio_sm_init(pio_choice, sm, offset, &cfg);
    pio_sm_set_enabled(pio_choice, sm, true);
}

// DMA IRQ handler: restart completed channels
void dma_handler() {
    // Acknowledge IRQ0
    uint32_t irq_status = dma_hw->ints0;
    (void)irq_status;

    for (int i = 0; i < 3; ++i) {
        if (!dma_channel_is_busy(dma_chan[i])) {
            dma_channel_acknowledge_irq0(dma_chan[i]);
            dma_channel_config c = dma_channel_get_default_config(dma_chan[i]);
            channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
            channel_config_set_read_increment(&c, true);
            channel_config_set_write_increment(&c, false);
            channel_config_set_dreq(&c, pio_get_dreq(group_pio[i], group_sm[i], true));
            channel_config_set_ring(&c, true, 8); // wrap read addr every 256 bytes
            dma_channel_configure(dma_chan[i], &c,
                &group_pio[i]->txf[group_sm[i]],
                dma_buf,
                256,
                true
            );
        }
    }
}

int main()
{
    stdio_init_all();

    // Initialize dma buffer with 0..255
    for (uint i = 0; i < 256; ++i) dma_buf[i] = i;

    // For each group claim SM on the correct PIO and set up
    uint bases[3] = { CA_BASE, CO_BASE, SIG_BASE };
    for (int i = 0; i < 3; ++i) {
        // Use SDK helper to claim an SM and add the PIO program for the GPIO range
        // Signature: bool pio_claim_free_sm_and_add_program_for_gpio_range(const pio_program_t *program, PIO *pio, uint *sm, uint *offset, uint gpio_base, uint gpio_count, bool set_gpio_base);
        bool ok = pio_claim_free_sm_and_add_program_for_gpio_range(&out_byte_program, &group_pio[i], &group_sm[i], &group_offset[i], bases[i], 8, true);
        if (!ok) {
            printf("Failed to claim PIO/SM for group %d (base %u)\n", i, bases[i]);
            while (true) tight_loop_contents();
        }
        printf("Group %d -> PIO %p SM %u offset %u\n", i, group_pio[i], group_sm[i], group_offset[i]);
        init_out_sm(group_pio[i], group_sm[i], group_offset[i], bases[i]);
    }

    // Claim three DMA channels
    for (int i = 0; i < 3; ++i) dma_chan[i] = dma_claim_unused_channel(true);

    // Configure and start DMA channels
    for (int i = 0; i < 3; ++i) {
        dma_channel_config cfg = dma_channel_get_default_config(dma_chan[i]);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
        channel_config_set_read_increment(&cfg, true);
        channel_config_set_write_increment(&cfg, false);
        channel_config_set_dreq(&cfg, pio_get_dreq(group_pio[i], group_sm[i], true));
        channel_config_set_ring(&cfg, true, 8);
        dma_channel_configure(dma_chan[i], &cfg,
            &group_pio[i]->txf[group_sm[i]],
            dma_buf,
            256,
            true
        );
    }

    // Enable DMA IRQ0 and set handler to restart transfers when done
    for (int i = 0; i < 3; ++i) dma_channel_set_irq0_enabled(dma_chan[i], true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Basic UART init kept from original code
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_puts(UART_ID, " Hello, UART!\n");

    // Main loop simply sleeps; DMA + PIO run autonomously
    while (true) {
        tight_loop_contents();
    }
}
