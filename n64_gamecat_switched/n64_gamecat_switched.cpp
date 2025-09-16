#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "n64_capture.pio.h"
#include "n64_respond_tokens.pio.h"

/* ========= Pin mapping ========= */
#define AD_BASE         12   // GPIO N..N+15 -> AD[0..15]
#define CART_EN_PIN     10  // output to CBT OE/EN (1=connected, 0=isolated)
#define KILL_PIN        11  // pull low to force pass-through

#define ALE_L_PIN_REL   16
#define ALE_H_PIN_REL   17
#define READ_N_PIN_REL  18
#define WRITE_N_PIN_REL 19
#define ALE_H_GPIO      (AD_BASE + ALE_H_PIN_REL)
#define ALE_L_GPIO      (AD_BASE + ALE_L_PIN_REL)

/* ========= Cheats ========= */
typedef struct { uint32_t mask, match, resp32; bool enable; } rule_t;

static const rule_t rules[] = {
    { 0xFFFF0000u, 0x12000000u, 0xDEADBEEFu, true },
    { (0xFFFFFFFFu & ~0x2u), (0x8037FF40u & ~0x2u), 0x11223344u, true },
    // add more…
};
static const int NUM_RULES = sizeof(rules)/sizeof(rules[0]);

/* Priority lookup at WORD granularity (addr must be word-aligned) */
static bool lookup_resp32(uint32_t addr_word, uint32_t* resp32_out) {
    for (int i=0; i<NUM_RULES; ++i) {
        if (!rules[i].enable) continue;
        uint32_t maskw  = rules[i].mask  | 0x2u;  // ignore bit1 for compare
        uint32_t matchw = rules[i].match & ~0x2u;
        if ((addr_word & maskw) == matchw) {
            *resp32_out = rules[i].resp32;
            return true;
        }
    }
    return false;
}

/* Make a 32-bit token: [31:16]=skip_halfreads, [15:0]=data */
static inline uint32_t make_token(uint16_t skip, uint16_t data) {
    return ((uint32_t)skip << 16) | data;
}

int main() {
    stdio_init_all();

    // GPIO directions
    for (int i=0; i<16; ++i) { gpio_init(AD_BASE+i); gpio_set_dir(AD_BASE+i, false); }
    gpio_init(ALE_H_GPIO);          gpio_set_dir(ALE_H_GPIO, false);
    gpio_init(KILL_PIN);            gpio_pull_up(KILL_PIN);
    gpio_init(CART_EN_PIN);         gpio_set_dir(CART_EN_PIN, true); gpio_put(CART_EN_PIN, 1);

    // Dev-board push-buttons (jumper to ALE_L/ALE_H): set as inputs with pull-ups
    gpio_init(6);   gpio_set_dir(6, false);   gpio_pull_down(6);
    gpio_init(32);  gpio_set_dir(32, false);  gpio_pull_down(32);

    // PIO setup (auto-claim SM and load program for the actual GPIO range)
    PIO  pio_cap;
    PIO  pio_rsp;
    uint off_cap, off_rsp;
    uint sm_cap,  sm_rsp;

    // We use the AD bus plus 4 strobes starting at AD_BASE, so pass a 20-pin range
    bool ok_cap = pio_claim_free_sm_and_add_program_for_gpio_range(&n64_capture_program,
                        &pio_cap, &sm_cap, &off_cap, AD_BASE, 20, true);
    if (!ok_cap) {
        printf("Failed to claim PIO/SM for capture on GPIO base %u\n", AD_BASE);
        return 1;
    }

    bool ok_rsp = pio_claim_free_sm_and_add_program_for_gpio_range(&n64_respond_tokens_program,
                        &pio_rsp, &sm_rsp, &off_rsp, AD_BASE, 20, true);
    if (!ok_rsp) {
        printf("Failed to claim PIO/SM for respond on GPIO base %u\n", AD_BASE);
        return 1;
    }

    // Give PIO ownership of the AD pins (for driving) and cart_en sideset pin
    for (int i = 0; i < 16; ++i) {
        pio_gpio_init(pio_rsp, AD_BASE + i);
    }
    pio_gpio_init(pio_rsp, CART_EN_PIN);

    // Also hand the capture SM the pins it needs to WAIT/IN on (AD[0..15], ALE_L/ALE_H)
    for (int i = 0; i < 16; ++i) {
        pio_gpio_init(pio_cap, AD_BASE + i);
    }
    pio_gpio_init(pio_cap, AD_BASE + ALE_L_PIN_REL);   // ALE_L (AD_BASE+16)
    pio_gpio_init(pio_cap, AD_BASE + ALE_H_PIN_REL);   // ALE_H (AD_BASE+17)
    gpio_pull_down(ALE_L_GPIO);
    gpio_pull_down(ALE_H_GPIO);

    // And make sure the responder SM owns the /READ and /WRITE pins it WAITs on
    pio_gpio_init(pio_rsp, AD_BASE + READ_N_PIN_REL);  // /READ (AD_BASE+18)
    pio_gpio_init(pio_rsp, AD_BASE + WRITE_N_PIN_REL); // /WRITE (AD_BASE+19)

    // Capture SM
    {
        pio_sm_config c = n64_capture_program_get_default_config(off_cap);
        sm_config_set_in_pins(&c, AD_BASE);
        sm_config_set_in_shift(&c, true, false, 32);
        pio_sm_init(pio_cap, sm_cap, off_cap, &c);
        pio_sm_set_enabled(pio_cap, sm_cap, true);
    }
    // Responder SM
    {
        pio_sm_config c = n64_respond_tokens_program_get_default_config(off_rsp);
        sm_config_set_out_pins(&c, AD_BASE, 16);
        sm_config_set_set_pins(&c, AD_BASE, 16);
        sm_config_set_in_pins(&c, AD_BASE);
        sm_config_set_sideset_pins(&c, CART_EN_PIN);
        sm_config_set_out_shift(&c, true, true, 32); // autopull 32-bit tokens
        pio_sm_init(pio_rsp, sm_rsp, off_rsp, &c);
        pio_sm_set_enabled(pio_rsp, sm_rsp, true);
    }

    // Latch state
    uint16_t lat_lo = 0, lat_hi = 0; bool have_lo = false, have_hi = false;

    // For scheduling inside a burst (counts /READ edges since last ALE)
    uint16_t half_idx_consumed = 0;  // how many halfreads our tokens already cover

    printf("N64 Gamecat Switched started.\n");
    for (;;) {
        // Safety: force pass-through if kill is low
        /*if (!gpio_get(KILL_PIN)) {
            gpio_put(CART_EN_PIN, 1);
            // Hard reset responder SM + FIFOs (drop old tokens)
            pio_sm_set_enabled(pio_rsp, sm_rsp, false);
            pio_sm_clear_fifos(pio_rsp, sm_rsp);
            pio_sm_restart(pio_rsp, sm_rsp);
            pio_sm_set_enabled(pio_rsp, sm_rsp, true);
            have_lo = have_hi = false;
            half_idx_consumed = 0;
        }*/

        // Consume captures on ALE_L rising edges
        if (!pio_sm_is_rx_fifo_empty(pio_cap, sm_cap)) {
            uint32_t raw = pio_sm_get(pio_cap, sm_cap);
            uint16_t hi16 = (uint16_t)((raw >> 16) & 0xFFFFu);
            uint16_t lo16 = (uint16_t)(raw & 0xFFFFu);
            bool upper = gpio_get(ALE_H_GPIO);
            // Our test capture program pushes the constant into the UPPER 16 bits
            uint16_t sample = hi16;
            printf("CAP raw=%08X hi=%04X lo=%04X ALE_H=%d\n", raw, hi16, lo16, upper);
            if (upper) { lat_hi = sample; have_hi = true; printf("ALE_H: %04X\n", sample); }
            else       { lat_lo = sample; have_lo = true; printf("ALE_L: %04X\n", sample); }

            if (have_lo && have_hi) {
                // New address -> reset any pending schedule and restart SM
                pio_sm_set_enabled(pio_rsp, sm_rsp, false);
                pio_sm_clear_fifos(pio_rsp, sm_rsp);
                pio_sm_restart(pio_rsp, sm_rsp);
                pio_sm_set_enabled(pio_rsp, sm_rsp, true);
                gpio_put(CART_EN_PIN, 1);          // ensure pass-through
                half_idx_consumed = 0;

                uint32_t base_addr = ((uint32_t)lat_hi << 16) | lat_lo;
                uint32_t base_word = base_addr & ~0x3u; // so we can match words or half-words to the same base

                printf("base_addr=0x%08X\n", base_addr);

                // Which half comes first for each 32-bit word in this burst?
                bool first_is_high = (base_addr & 0x2u) != 0;

                // Pre-schedule up to 256 words worth of *hits* (sparse)
                for (uint32_t w = 0; w < 256; ++w) {
                    uint32_t addr_w = base_word + (w << 2);
                    uint32_t resp32;
                    if (!lookup_resp32(addr_w, &resp32)) continue;

                    uint16_t lo = (uint16_t)(resp32 & 0xFFFFu);
                    uint16_t hi = (uint16_t)(resp32 >> 16);

                    // Where in the halfread stream should the FIRST half of this word land?
                    // Each word = 2 halfreads. Index of first half = 2*w.
                    uint16_t first_half_idx = (uint16_t)(2*w);

                    // Skip count = pulses to ignore BEFORE patching first half
                    uint16_t skip0 = (first_half_idx >= half_idx_consumed)
                                   ? (first_half_idx - half_idx_consumed)
                                   : 0;

                    // Token #1: first half (depends on starting alignment)
                    uint16_t first_half = first_is_high ? hi : lo;
                    pio_sm_put_blocking(pio_rsp, sm_rsp, make_token(skip0, first_half));
                    half_idx_consumed = first_half_idx + 1;

                    // Token #2: second half follows immediately (skip=0)
                    uint16_t second_half = first_is_high ? lo : hi;
                    pio_sm_put_blocking(pio_rsp, sm_rsp, make_token(0, second_half));
                    half_idx_consumed = first_half_idx + 2;
                }

                // Ready for the next latch pair
                have_lo = have_hi = false;
            }
        }

        tight_loop_contents();
    }
}