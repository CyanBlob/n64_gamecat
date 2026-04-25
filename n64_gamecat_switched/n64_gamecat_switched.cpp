#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "n64_capture.pio.h"
#include "n64_respond_tokens.pio.h"

/* ========= Pin mapping ========= */
#define AD_BASE         0   // GPIO N..N+15 -> AD[0..15]
#define CART_EN_PIN     20   // sideset to CBT OE/EN (1=connected, 0=isolated)
#define KILL_PIN        21   // pull low to force pass-through

#define ALE_L_PIN_REL   16
#define ALE_H_PIN_REL   17
#define READ_N_PIN_REL  18
#define WRITE_N_PIN_REL 19
#define ALE_L_GPIO      (AD_BASE + ALE_L_PIN_REL)
#define ALE_H_GPIO      (AD_BASE + ALE_H_PIN_REL)
#define READ_N_GPIO     (AD_BASE + READ_N_PIN_REL)
#define WRITE_N_GPIO    (AD_BASE + WRITE_N_PIN_REL)

/* ========= Bootcode CRC bypass =========
 *
 * The N64 bootcode (running from cart ROM after CIC validation) computes a
 * CRC over ROM offsets 0x1000..0x101000 (cart-bus 0x10001000..0x10101000)
 * and checks it against CRC1/CRC2 in the header. Any patch we make inside
 * that window changes the computed CRC, and the console refuses to start.
 *
 * Per the en64 wiki ("Bypassing bad checksum"), NOPing two specific BNE
 * instructions in the bootcode makes the CRC mismatch branch a no-op, so
 * any subsequent patch — anywhere in ROM — is fine. The two ROM offsets
 * depend on the CIC chip; the bootcode itself lives at 0x40..0x1000 which
 * is *outside* the CRC window, so NOPing those two words doesn't itself
 * change the CRC.
 *
 * To enable, set N64_CIC_TYPE to your cart's CIC family below (or via a
 * -DN64_CIC_TYPE=NNNN compile flag in CMakeLists.txt). Setting it wrong
 * for the inserted cart will NOP unrelated bootcode instructions and the
 * game won't boot — match this to the actual cart. Common pairings:
 *
 *   N64_CIC_TYPE=0      No bypass (use if all patches are outside CRC window)
 *   N64_CIC_TYPE=6101   Star Fox 64 only
 *   N64_CIC_TYPE=6102   "Standard" NTSC: SM64, Mario Kart 64, Banjo-Kazooie,
 *                       most NTSC games        (PAL equivalent: 7101)
 *   N64_CIC_TYPE=6103   Paper Mario, etc.       (PAL equivalent: 7103)
 *   N64_CIC_TYPE=6105   Zelda OoT, Majora's Mask (PAL equivalent: 7105)
 *   N64_CIC_TYPE=6106   Yoshi's Story, F-Zero X — bypass offsets not yet
 *                       documented in the en64 wiki; leave at 0 and patch
 *                       outside the CRC window for these games.
 */
#ifndef N64_CIC_TYPE
#define N64_CIC_TYPE 6105
#endif

#if N64_CIC_TYPE == 0
    #define CIC_BYPASS_RULES /* none */
#elif N64_CIC_TYPE == 6101
    #define CIC_BYPASS_RULES \
        { 0xFFFFFFFCu, 0x10000670u, 0x00000000u, true }, \
        { 0xFFFFFFFCu, 0x1000067Cu, 0x00000000u, true },
#elif N64_CIC_TYPE == 6102 || N64_CIC_TYPE == 7101
    #define CIC_BYPASS_RULES \
        { 0xFFFFFFFCu, 0x1000066Cu, 0x00000000u, true }, \
        { 0xFFFFFFFCu, 0x10000678u, 0x00000000u, true },
#elif N64_CIC_TYPE == 6103 || N64_CIC_TYPE == 7103
    #define CIC_BYPASS_RULES \
        { 0xFFFFFFFCu, 0x1000063Cu, 0x00000000u, true }, \
        { 0xFFFFFFFCu, 0x10000648u, 0x00000000u, true },
#elif N64_CIC_TYPE == 6105 || N64_CIC_TYPE == 7105
    #define CIC_BYPASS_RULES \
        { 0xFFFFFFFCu, 0x1000077Cu, 0x00000000u, true }, \
        { 0xFFFFFFFCu, 0x10000788u, 0x00000000u, true },
#else
    #error "Unsupported N64_CIC_TYPE: must be 0, 6101, 6102/7101, 6103/7103, or 6105/7105"
#endif

/* ========= Cheats =========
 * mask / match compare at 32-bit word granularity.
 * resp32 is the value served for matched words.
 */
typedef struct { uint32_t mask, match, resp32; bool enable; } rule_t;


// NOTE: 0x10001000–0x10101000 (first 1MB of cart address space) is CRCed
// sim/check_rules.py checks this
static const rule_t rules[] = {
    // Bootcode CRC bypass (gated by N64_CIC_TYPE above; expands to nothing
    // when N64_CIC_TYPE == 0).
    CIC_BYPASS_RULES

    // Inject PI bus-timing override 0x8037FF40 as the first header word.
    // (Wiki: setting this value slows all bus activity — useful for debugging.)
    { 0xFFFFFFFCu, 0x10000000u, 0x8037FF40u, false },

    // Demo: override a 64KB window with a recognisable value.
    { 0xFFFF0000u, 0x12000000u, 0xDEADBEEFu, true },
};
static const int NUM_RULES = sizeof(rules)/sizeof(rules[0]);

static bool lookup_resp32(uint32_t addr_word, uint32_t* resp32_out) {
    for (int i = 0; i < NUM_RULES; ++i) {
        if (!rules[i].enable) continue;
        if ((addr_word & rules[i].mask) == (rules[i].match & rules[i].mask)) {
            *resp32_out = rules[i].resp32;
            return true;
        }
    }
    return false;
}

/* Token layout matches n64_respond_tokens.pio with shift_right OUT:
 *   bits [15: 0] = SKIP, bits [31:16] = DATA.
 */
static inline uint32_t make_token(uint16_t skip, uint16_t data) {
    return ((uint32_t)data << 16) | skip;
}

int main() {
    stdio_init_all();

    // AD pins: inputs by default; PIO will flip pindirs during an override drive.
    for (int i = 0; i < 16; ++i) {
        gpio_init(AD_BASE + i);
        gpio_set_dir(AD_BASE + i, false);
    }

    // Strobes are always inputs from the console.
    gpio_init(ALE_L_GPIO);  gpio_set_dir(ALE_L_GPIO, false);  gpio_pull_down(ALE_L_GPIO);
    gpio_init(ALE_H_GPIO);  gpio_set_dir(ALE_H_GPIO, false);  gpio_pull_down(ALE_H_GPIO);
    gpio_init(READ_N_GPIO); gpio_set_dir(READ_N_GPIO, false);
    gpio_init(WRITE_N_GPIO); gpio_set_dir(WRITE_N_GPIO, false);

    gpio_init(KILL_PIN);    gpio_pull_up(KILL_PIN);
    gpio_init(CART_EN_PIN); gpio_set_dir(CART_EN_PIN, true); gpio_put(CART_EN_PIN, 1);

    // Dev-board push-buttons (kept from bring-up setup).
    gpio_init(6);   gpio_set_dir(6, false);   gpio_pull_down(6);
    gpio_init(32);  gpio_set_dir(32, false);  gpio_pull_down(32);

    PIO  pio_cap, pio_rsp;
    uint off_cap, off_rsp, sm_cap, sm_rsp;

    bool ok_cap = pio_claim_free_sm_and_add_program_for_gpio_range(
        &n64_capture_program, &pio_cap, &sm_cap, &off_cap, AD_BASE, 20, true);
    if (!ok_cap) { printf("Failed to claim capture SM\n"); return 1; }

    bool ok_rsp = pio_claim_free_sm_and_add_program_for_gpio_range(
        &n64_respond_tokens_program, &pio_rsp, &sm_rsp, &off_rsp, AD_BASE, 20, true);
    if (!ok_rsp) { printf("Failed to claim respond SM\n"); return 1; }

    // Responder owns AD (for driving) and CART_EN (for sideset).
    for (int i = 0; i < 16; ++i) pio_gpio_init(pio_rsp, AD_BASE + i);
    pio_gpio_init(pio_rsp, CART_EN_PIN);

    // CART_EN must be an output from the PIO's perspective; AD starts as input
    // (cart drives until we isolate and override).
    pio_sm_set_consecutive_pindirs(pio_rsp, sm_rsp, CART_EN_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(pio_rsp, sm_rsp, AD_BASE, 16, false);

    // Capture SM config
    {
        pio_sm_config c = n64_capture_program_get_default_config(off_cap);
        sm_config_set_in_pins(&c, AD_BASE);
        // shift_left so two `in pins, 16` give ISR = (upper << 16) | lower.
        sm_config_set_in_shift(&c, /*shift_right=*/false, /*autopush=*/false, 32);
        pio_sm_init(pio_cap, sm_cap, off_cap, &c);
        pio_sm_set_enabled(pio_cap, sm_cap, true);
    }

    // Responder SM config
    {
        pio_sm_config c = n64_respond_tokens_program_get_default_config(off_rsp);
        sm_config_set_out_pins(&c, AD_BASE, 16);
        sm_config_set_in_pins(&c, AD_BASE);
        sm_config_set_sideset_pins(&c, CART_EN_PIN);
        // shift_right + autopull so `out x, 16` / `out y, 16` unpack SKIP then DATA.
        sm_config_set_out_shift(&c, /*shift_right=*/true, /*autopull=*/true, 32);
        pio_sm_init(pio_rsp, sm_rsp, off_rsp, &c);
        pio_sm_set_enabled(pio_rsp, sm_rsp, true);
    }

    printf("N64 Gamecat Switched started.\n");

    uint32_t burst_count = 0;
    for (;;) {
        if (pio_sm_is_rx_fifo_empty(pio_cap, sm_cap)) {
            tight_loop_contents();
            continue;
        }

        uint32_t base_addr = pio_sm_get(pio_cap, sm_cap);

        // Every new address burst: reset the responder so its skip-counting
        // starts from the first upcoming /READ pulse, with no stale tokens.
        pio_sm_set_enabled(pio_rsp, sm_rsp, false);
        pio_sm_clear_fifos(pio_rsp, sm_rsp);
        pio_sm_restart(pio_rsp, sm_rsp);
        pio_sm_set_enabled(pio_rsp, sm_rsp, true);

        // Kill switch: skip all token scheduling, leaving the responder idle
        // at `pull block side 1` (cart permanently connected).
        if (!gpio_get(KILL_PIN)) continue;

        uint32_t base_word  = base_addr & ~0x3u;
        bool     first_is_high = (base_addr & 0x2u) != 0;

        uint16_t half_idx_consumed = 0;  // halfreads our tokens already cover

        // Max 256 words between ALE_L phases per the bus spec.
        for (uint32_t w = 0; w < 256; ++w) {
            uint32_t addr_w = base_word + (w << 2);
            uint32_t resp32;
            if (!lookup_resp32(addr_w, &resp32)) continue;

            uint16_t lo = (uint16_t)(resp32 & 0xFFFFu);
            uint16_t hi = (uint16_t)(resp32 >> 16);

            uint16_t first_half_idx = (uint16_t)(2 * w);
            uint16_t skip0 = first_half_idx - half_idx_consumed;

            uint16_t first_half  = first_is_high ? hi : lo;
            uint16_t second_half = first_is_high ? lo : hi;

            pio_sm_put_blocking(pio_rsp, sm_rsp, make_token(skip0, first_half));
            pio_sm_put_blocking(pio_rsp, sm_rsp, make_token(0,     second_half));
            half_idx_consumed = first_half_idx + 2;
        }

        // Periodic heartbeat only — keep the hot path free of UART output.
        if ((++burst_count & 0xFFF) == 0) {
            printf("bursts=%u last=0x%08X\n", burst_count, base_addr);
        }
    }
}
