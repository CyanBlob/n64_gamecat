#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/vreg.h"
#include "n64_capture.pio.h"
#include "n64_respond_tokens.pio.h"
#include "n64_snoop.pio.h"

// RP2350 SDK default is 150 MHz, but the PIO programs and README timing
// budget assume 300 MHz (1 PIO cycle = 3.3ns). Running at the SDK default
// halves the C scheduler's real-world throughput against the token-
// scheduling turnaround the README already flags as the one tight margin
// in the design — likely a contributor to observed override/skip-count
// desync. Bump the core voltage before raising clk_sys, standard practice
// for RP2350 above stock speed.
#define SYSTEM_CLOCK_KHZ 300000

#define DEBUG_PRINT 0
// Snoop-based debug capture (header dump, msgwatch live-stream). Even with
// token scheduling moved ahead of it, per-burst capture overhead in the hot
// loop can still delay servicing of the *next* burst during rapid-fire
// gameplay reads, which is enough to desync override timing. Disable while
// verifying overrides actually work; re-enable only for offline text-hunting
// sessions where correctness of overrides isn't being tested simultaneously.
#define DEBUG_SNOOP 1

/* ========= Pin mapping ========= */
#define AD_BASE         0   // GPIO N..N+15 -> AD[0..15]
#define CART_EN_PIN     20   // sideset to CBT OE/EN (1=connected, 0=isolated)
#define KILL_PIN        21   // pull low to force pass-through
#define STATUS_LED_PIN  22   // D1 on the schematic (heartbeat)

#define ALE_L_PIN_REL   16
#define ALE_H_PIN_REL   17
#define READ_N_PIN_REL  18
#define WRITE_N_PIN_REL 19
#define ALE_L_GPIO      (AD_BASE + ALE_L_PIN_REL)
#define ALE_H_GPIO      (AD_BASE + ALE_H_PIN_REL)
#define READ_N_GPIO     (AD_BASE + READ_N_PIN_REL)
#define WRITE_N_GPIO    (AD_BASE + WRITE_N_PIN_REL)

/* ========= Heartbeat LED =========
 * Driven by a hardware-timer IRQ that fires twice per second.
 * Each callback is ~100 ns of CPU work, only when the IRQ fires —
 * average overhead is ~200 ns / second = 0.00004% of the CPU. The
 * worst-case latency it can add to the burst-scheduling hot path is
 * ~100 ns once per ~500 ms (only when the IRQ happens to overlap a
 * burst), which is well below the C scheduler's existing ~1 µs budget.
 *
 * The PIO state machines are entirely unaffected — they have their
 * own pipelines and run independently of CPU IRQs.
 */
#define HEARTBEAT_PERIOD_MS 500

static bool led_heartbeat_cb(repeating_timer_t* rt) {
    static bool state = false;
    state = !state;

    #if DEBUG_PRINT
    printf("heartbeat: %d\n", state);
    #endif
    gpio_put(STATUS_LED_PIN, state);
    return true; // keep firing
}

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
    //CIC_BYPASS_RULES

    // Inject PI bus-timing override 0x8037FF40 as the first header word.
    // (Wiki: setting this value slows all bus activity — useful for debugging.)
    //{ 0xFFFFFFFCu, 0x10000000u, 0x8037FF40u, false },

    // Demo: override a 64KB window with a recognisable value.
    //{ 0xFFFF0000u, 0x12000000u, 0xDEADBEEFu, true },

    // "fairy"->"butt " in the Deku Tree's Kokiri-forest speech, V1.1 (this
    // cart, confirmed via live CRC2 = 0x021E1E19). Addresses/values computed
    // directly from real bytes snooped live off the cart bus — not from a
    // baserom file, since no genuine V1.1 dump was available.
    //
    // The first attempt at these (now corrected) was wrong: the snoop
    // capture occasionally includes a spurious leading 0x0000 halfword
    // before the real burst data (a reset-timing race, not real ROM
    // content) — comparing two captures of the same address caught it, one
    // had it and one didn't. Values below are from the artifact-free
    // capture; cross-check any newly-added address the same way before
    // trusting it.
    { 0xFFFFFFFCu, 0x1091F338u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091F33Cu, 0x202E0E3Cu, true },
    { 0xFFFFFFFCu, 0x1091F378u, 0x61206275u, true },
    { 0xFFFFFFFCu, 0x1091F37Cu, 0x7474202Eu, true },

    // Below: the original 100 rules generated against the WRONG revision
    // (V1.2 baserom) — this cart is V1.1. Left disabled/for reference; do not
    // enable without regenerating against real V1.1 bytes first.
#if 0
    { 0xFFFFFFFCu, 0x1090D094u, 0x42757474u, true },
    { 0xFFFFFFFCu, 0x1090D098u, 0x2020536Cu, true },
    { 0xFFFFFFFCu, 0x1090D200u, 0x42757474u, true },
    { 0xFFFFFFFCu, 0x1090D204u, 0x2020426Fu, true },
    { 0xFFFFFFFCu, 0x1090DF30u, 0x05414275u, true },
    { 0xFFFFFFFCu, 0x1090DF34u, 0x74742005u, true },
    { 0xFFFFFFFCu, 0x1090DF9Cu, 0x6E792062u, true },
    { 0xFFFFFFFCu, 0x1090DFA0u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x1090E25Cu, 0x20054142u, true },
    { 0xFFFFFFFCu, 0x1090E260u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x10910F58u, 0x08427574u, true },
    { 0xFFFFFFFCu, 0x10910F5Cu, 0x74202773u, true },
    { 0xFFFFFFFCu, 0x10910F8Cu, 0x08054142u, true },
    { 0xFFFFFFFCu, 0x10910F90u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x1091192Cu, 0x61742042u, true },
    { 0xFFFFFFFCu, 0x10911930u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x10911998u, 0x61742042u, true },
    { 0xFFFFFFFCu, 0x1091199Cu, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x10911A04u, 0x61742042u, true },
    { 0xFFFFFFFCu, 0x10911A08u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x1091208Cu, 0x61742042u, true },
    { 0xFFFFFFFCu, 0x10912090u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x10913868u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091386Cu, 0x20010544u, true },
    { 0xFFFFFFFCu, 0x10918384u, 0x6F6E2D62u, true },
    { 0xFFFFFFFCu, 0x10918388u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x1091A7C8u, 0x20627574u, true },
    { 0xFFFFFFFCu, 0x1091A7CCu, 0x74202105u, true },
    { 0xFFFFFFFCu, 0x1091A848u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091A84Cu, 0x20212104u, true },
    { 0xFFFFFFFCu, 0x1091A85Cu, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091A860u, 0x20206361u, true },
    { 0xFFFFFFFCu, 0x1091A954u, 0x05446275u, true },
    { 0xFFFFFFFCu, 0x1091A958u, 0x74742005u, true },
    { 0xFFFFFFFCu, 0x1091AEA4u, 0x05446275u, true },
    { 0xFFFFFFFCu, 0x1091AEA8u, 0x74742005u, true },
    { 0xFFFFFFFCu, 0x1091AED4u, 0x61206275u, true },
    { 0xFFFFFFFCu, 0x1091AED8u, 0x74742005u, true },
    { 0xFFFFFFFCu, 0x1091AF18u, 0x75722062u, true },
    { 0xFFFFFFFCu, 0x1091AF1Cu, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x1091AF2Cu, 0x20612062u, true },
    { 0xFFFFFFFCu, 0x1091AF30u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x1091B044u, 0x44627574u, true },
    { 0xFFFFFFFCu, 0x1091B048u, 0x74202005u, true },
    { 0xFFFFFFFCu, 0x1091B4B4u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091B4B8u, 0x202E2E2Eu, true },
    { 0xFFFFFFFCu, 0x1091C04Cu, 0x20627574u, true },
    { 0xFFFFFFFCu, 0x1091C050u, 0x74202E2Eu, true },
    { 0xFFFFFFFCu, 0x1091C14Cu, 0x42757474u, true },
    { 0xFFFFFFFCu, 0x1091C150u, 0x20054021u, true },
    { 0xFFFFFFFCu, 0x1091C190u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091C194u, 0x202C2079u, true },
    { 0xFFFFFFFCu, 0x1091D030u, 0x68652062u, true },
    { 0xFFFFFFFCu, 0x1091D034u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x1091DA88u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091DA8Cu, 0x2020666Fu, true },
    { 0xFFFFFFFCu, 0x1091F278u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091F27Cu, 0x2005402Eu, true },
    { 0xFFFFFFFCu, 0x1091F45Cu, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091F460u, 0x20200540u, true },
    { 0xFFFFFFFCu, 0x1091F8ACu, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1091F8B0u, 0x202E0E3Cu, true },
    { 0xFFFFFFFCu, 0x1091F8ECu, 0x61206275u, true },
    { 0xFFFFFFFCu, 0x1091F8F0u, 0x7474202Eu, true },
    { 0xFFFFFFFCu, 0x10920B70u, 0x20627574u, true },
    { 0xFFFFFFFCu, 0x10920B74u, 0x74203F21u, true },
    { 0xFFFFFFFCu, 0x10921330u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x10921334u, 0x2020626Fu, true },
    { 0xFFFFFFFCu, 0x10923830u, 0x20612062u, true },
    { 0xFFFFFFFCu, 0x10923834u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x109238ECu, 0x2C016275u, true },
    { 0xFFFFFFFCu, 0x109238F0u, 0x74742020u, true },
    { 0xFFFFFFFCu, 0x10923B6Cu, 0x65206275u, true },
    { 0xFFFFFFFCu, 0x10923B70u, 0x74742020u, true },
    { 0xFFFFFFFCu, 0x10923C48u, 0x2C206275u, true },
    { 0xFFFFFFFCu, 0x10923C4Cu, 0x74742020u, true },
    { 0xFFFFFFFCu, 0x10923CB4u, 0x752C2062u, true },
    { 0xFFFFFFFCu, 0x10923CB8u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x10923D30u, 0x20627574u, true },
    { 0xFFFFFFFCu, 0x10923D34u, 0x74202062u, true },
    { 0xFFFFFFFCu, 0x1092A350u, 0x42757474u, true },
    { 0xFFFFFFFCu, 0x1092A354u, 0x20054020u, true },
    { 0xFFFFFFFCu, 0x1092A474u, 0x74204275u, true },
    { 0xFFFFFFFCu, 0x1092A478u, 0x74742027u, true },
    { 0xFFFFFFFCu, 0x1092A584u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x1092A588u, 0x20010540u, true },
    { 0xFFFFFFFCu, 0x1092A5D8u, 0x05416275u, true },
    { 0xFFFFFFFCu, 0x1092A5DCu, 0x74742020u, true },
    { 0xFFFFFFFCu, 0x1092A764u, 0x74204275u, true },
    { 0xFFFFFFFCu, 0x1092A768u, 0x74742005u, true },
    { 0xFFFFFFFCu, 0x1093031Cu, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x10930320u, 0x20206275u, true },
    { 0xFFFFFFFCu, 0x10940684u, 0x20054442u, true },
    { 0xFFFFFFFCu, 0x10940688u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x10940D20u, 0x20627574u, true },
    { 0xFFFFFFFCu, 0x10940D24u, 0x74200540u, true },
    { 0xFFFFFFFCu, 0x10940EECu, 0x20054462u, true },
    { 0xFFFFFFFCu, 0x10940EF0u, 0x75747420u, true },
    { 0xFFFFFFFCu, 0x10942C18u, 0x62757474u, true },
    { 0xFFFFFFFCu, 0x10942C1Cu, 0x2020626Fu, true },
#endif
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

/* DEBUG: capture snooped halfwords for the burst currently in flight, until
 * a per-halfword timeout elapses or max_hw is reached. Does NOT bail out
 * just because the capture SM has already latched a new address — that can
 * happen from unrelated bus activity interleaved between this burst's own
 * pulses without the responder's current token schedule actually being
 * reset (the reset only happens once the main loop itself processes that
 * queued address, on a later iteration, not while we're still in here).
 * Bailing on it early was truncating captures well before the burst's own
 * real activity was done. */
static int snoop_capture_burst(PIO pio_snoop, uint sm_snoop, PIO pio_cap, uint sm_cap,
                                uint16_t* hw, int max_hw, uint32_t timeout_us_per_hw) {
    int got = 0;
    for (int i = 0; i < max_hw; ++i) {
        uint64_t deadline = time_us_64() + timeout_us_per_hw;
        for (;;) {
            if (!pio_sm_is_rx_fifo_empty(pio_snoop, sm_snoop)) break;
            if (time_us_64() > deadline) return got;
        }
        hw[i] = (uint16_t)pio_sm_get(pio_snoop, sm_snoop);
        got = i + 1;
    }
    return got;
}

/* Render halfwords as hex + a best-effort ASCII strip (big-endian bytes per
 * halfword, matching how text is laid out in the ROM). */
static void snoop_print_hw(const uint16_t* hw, int got) {
    for (int i = 0; i < got; ++i) printf(" %04X", hw[i]);
    printf("  |");
    for (int i = 0; i < got; ++i) {
        uint8_t hi = hw[i] >> 8, lo = hw[i] & 0xFF;
        putchar((hi >= 32 && hi < 127) ? (char)hi : '.');
        putchar((lo >= 32 && lo < 127) ? (char)lo : '.');
    }
    printf("|\n");
}

int main_dumb() {
    stdio_init_all();
    gpio_init(CART_EN_PIN);
    gpio_set_dir(CART_EN_PIN, true);
    gpio_put(CART_EN_PIN, 0);   // CBT closed, cart connected

    // Heartbeat so we know it's alive
    gpio_init(STATUS_LED_PIN);
    gpio_set_dir(STATUS_LED_PIN, true);
    static repeating_timer_t t;
    add_repeating_timer_ms(2000, led_heartbeat_cb, NULL, &t);

    for (;;) tight_loop_contents();
}

int main() {
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(10); // let the core voltage settle before raising clk_sys
    set_sys_clock_khz(SYSTEM_CLOCK_KHZ, true);

    stdio_init_all();

    sleep_ms(1000); // wait for USB serial to connect

    #if DEBUG_PRINT
    printf("Started\n");
    #endif

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
    gpio_init(CART_EN_PIN); gpio_set_dir(CART_EN_PIN, true); gpio_put(CART_EN_PIN, 0);

    // Status LED (D1) — driven by hardware-timer IRQ (see led_heartbeat_cb).
    gpio_init(STATUS_LED_PIN);
    gpio_set_dir(STATUS_LED_PIN, true);
    gpio_put(STATUS_LED_PIN, 0);
    static repeating_timer_t heartbeat_timer;
    add_repeating_timer_ms(HEARTBEAT_PERIOD_MS, led_heartbeat_cb, NULL, &heartbeat_timer);

    // Dev-board push-buttons (kept from bring-up setup).
    gpio_init(6);   gpio_set_dir(6, false);   gpio_pull_down(6);
    gpio_init(32);  gpio_set_dir(32, false);  gpio_pull_down(32);

    #if DEBUG_PRINT
    printf("Initialized\n");
    #endif

    PIO  pio_cap, pio_rsp, pio_snoop;
    uint off_cap, off_rsp, off_snoop, sm_cap, sm_rsp, sm_snoop;

    bool ok_cap = pio_claim_free_sm_and_add_program_for_gpio_range(
        &n64_capture_program, &pio_cap, &sm_cap, &off_cap, AD_BASE, 20, true);
    if (!ok_cap) { printf("Failed to claim capture SM\n"); return 1; }

    bool ok_rsp = pio_claim_free_sm_and_add_program_for_gpio_range(
        &n64_respond_tokens_program, &pio_rsp, &sm_rsp, &off_rsp, AD_BASE, 20, true);
    if (!ok_rsp) { printf("Failed to claim respond SM\n"); return 1; }

    // DEBUG: passive snoop SM — captures whatever's actually on AD at each
    // /READ, regardless of who's driving. Used to read the real cart header
    // back over serial (see the base_addr == 0x10000000 block below).
    bool ok_snoop = pio_claim_free_sm_and_add_program_for_gpio_range(
        &n64_snoop_program, &pio_snoop, &sm_snoop, &off_snoop, AD_BASE, 20, true);
    if (!ok_snoop) { printf("Failed to claim snoop SM\n"); return 1; }

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
        // shift_right so `out x, 16` / `out y, 16` unpack SKIP then DATA.
        // autopull MUST be false: the program already does explicit `pull
        // block`, and our two `out` instructions consume exactly 32 bits
        // (the full threshold) every single loop — with autopull also on,
        // that triggers a SECOND, automatic pull right after our explicit
        // one, silently discarding every other token before the program
        // ever sees it. Confirmed via logic analyzer: token N+1 in a
        // back-to-back pair never drove; the pulse instead showed token
        // N+2's data, with ~250ns/~75 PIO-cycle margin ruling out a timing
        // race as the cause.
        sm_config_set_out_shift(&c, /*shift_right=*/true, /*autopull=*/false, 32);
        pio_sm_init(pio_rsp, sm_rsp, off_rsp, &c);
        pio_sm_set_enabled(pio_rsp, sm_rsp, true);
    }

    // Snoop SM config
    {
        pio_sm_config c = n64_snoop_program_get_default_config(off_snoop);
        sm_config_set_in_pins(&c, AD_BASE);
        sm_config_set_in_shift(&c, /*shift_right=*/false, /*autopush=*/false, 32);
        pio_sm_init(pio_snoop, sm_snoop, off_snoop, &c);
        pio_sm_set_enabled(pio_snoop, sm_snoop, true);
    }

    #if DEBUG_PRINT
    printf("PIO Started\n");
    printf("N64 Gamecat Switched started.\n");
    #endif


    #if DEBUG_PRINT
    printf("Killed: %d\n", !gpio_get(KILL_PIN));
    #endif

    // DEBUG: after a burst fires a rule, trace the raw sequence of every
    // subsequent latched base_addr (matching or not) for a while — to check
    // whether what we've been treating as "one continuous burst" is
    // actually interrupted by the console latching addresses in completely
    // unrelated ROM regions in between, which would explain why a debug
    // snoop capture anchored to the matched burst's own address kept
    // running dry after only ~5 halfwords regardless of timeout.
    int trace_remaining = 0;

    uint32_t burst_count = 0;
    for (;;) {
        if (pio_sm_is_rx_fifo_empty(pio_cap, sm_cap)) {
            tight_loop_contents();
            continue;
        }

        uint32_t base_addr = pio_sm_get(pio_cap, sm_cap);
        // Captured as early as possible (before any reset/scheduling work)
        // so it's the closest available anchor to when this burst's actual
        // /READ pulses happen on the bus — useful for lining up against a
        // logic analyzer capture's own (unrelated-origin) timebase via the
        // *gaps* between consecutive bursts, since the two clocks share no
        // common zero point but the relative spacing between events matches.
        uint64_t burst_t_us = time_us_64();

        // DEBUG: read back CART_EN's level (driven by the responder's
        // sideset) before we reset it. If the *previous* burst's responder
        // got stuck mid-drive (isolated, side1), this reads high right up
        // until the reset below clears it — telling us definitively whether
        // the previous burst left the cart isolated or safely idle
        // (side0/skip_loop), without needing another LA capture.
        bool prev_cart_isolated = gpio_get(CART_EN_PIN);

        if (trace_remaining > 0) {
            printf("[trace t=%llu us] addr=0x%08X  prev_cart_isolated=%d\n",
                   (unsigned long long)burst_t_us, base_addr, prev_cart_isolated);
            trace_remaining--;
        } else if (prev_cart_isolated) {
            printf("[trace t=%llu us] addr=0x%08X  prev_cart_isolated=%d (outside trace window)\n",
                   (unsigned long long)burst_t_us, base_addr, prev_cart_isolated);
        }

        // Every new address burst: reset the responder so its skip-counting
        // starts from the first upcoming /READ pulse, with no stale tokens.
        // NOTE: doesn't handle a burst the console abandons mid-stream before
        // reaching our last scheduled word — responder can sit stuck isolated
        // until the next real burst forces a reset here. Confirmed via logic
        // analyzer: CART_EN stuck high ~49ms in that scenario. Needs a fix
        // driven by real ALE_L behavior (still uncaptured), not a guessed
        // timeout or gap heuristic — both tried and reverted.
        pio_sm_set_enabled(pio_rsp, sm_rsp, false);
        pio_sm_clear_fifos(pio_rsp, sm_rsp);
        pio_sm_restart(pio_rsp, sm_rsp);
        pio_sm_set_enabled(pio_rsp, sm_rsp, true);

        #if DEBUG_SNOOP
        // Keep the snoop SM aligned to this burst too, so its FIFO only ever
        // holds halfwords belonging to the burst we're about to process.
        pio_sm_set_enabled(pio_snoop, sm_snoop, false);
        pio_sm_clear_fifos(pio_snoop, sm_snoop);
        pio_sm_restart(pio_snoop, sm_snoop);
        pio_sm_set_enabled(pio_snoop, sm_snoop, true);
        #endif

        // Token scheduling MUST happen immediately here, before any debug
        // snoop capture below — the responder's skip-counting assumes
        // token0 arrives with microsecond-scale latency after burst start.
        // Debug capture blocks for however long real /READ pulses take to
        // arrive (up to ~2ms/halfword timeouts), which previously ran
        // *before* this and desynced the skip count enough that overrides
        // landed on the wrong, later burst instead of the intended one.
        bool killed = !gpio_get(KILL_PIN);
        if (!killed) {
            uint32_t base_word  = base_addr & ~0x3u;
            // Verified against a live capture of the cart header: for a
            // word-aligned base_addr (bit1==0), the console reads the HIGH
            // halfword first (0x10000000 -> "8037" then "1240" == 0x80371240).
            // This was previously inverted (`!= 0`), silently swapping the
            // high/low halves of every multi-halfword override.
            bool     first_is_high = (base_addr & 0x2u) == 0;

            uint16_t half_idx_consumed = 0;  // halfreads our tokens already cover

            // Rule-fire log: just raw stores into a small stack array while
            // scheduling (a few cycles each, no formatting/IO) — the actual
            // printf pass happens below, only after every token for this
            // burst is already queued, so it can't delay the override itself.
            uint32_t fired_addr[16];
            uint32_t fired_resp[16];
            int      fired_count = 0;

            // DEBUG: raw (skip,data) pairs as actually pushed to the
            // responder's FIFO, so we can compare what C computed against
            // what the PIO actually appears to execute (from snoop data).
            uint16_t tok_skip[32];
            uint16_t tok_data[32];
            int      tok_count = 0;

            // Max 256 words between ALE_L phases per the bus spec.
            for (uint32_t w = 0; w < 256; ++w) {
                uint32_t addr_w = base_word + (w << 2);
                uint32_t resp32;
                if (!lookup_resp32(addr_w, &resp32)) continue;

                if (fired_count < 16) {
                    fired_addr[fired_count] = addr_w;
                    fired_resp[fired_count] = resp32;
                    fired_count++;
                }

                uint16_t lo = (uint16_t)(resp32 & 0xFFFFu);
                uint16_t hi = (uint16_t)(resp32 >> 16);

                uint16_t first_half_idx = (uint16_t)(2 * w);
                uint16_t skip0 = first_half_idx - half_idx_consumed;

                uint16_t first_half  = first_is_high ? hi : lo;
                uint16_t second_half = first_is_high ? lo : hi;

                pio_sm_put_blocking(pio_rsp, sm_rsp, make_token(skip0, first_half));
                pio_sm_put_blocking(pio_rsp, sm_rsp, make_token(0,     second_half));
                if (tok_count < 30) {
                    tok_skip[tok_count] = skip0; tok_data[tok_count] = first_half; tok_count++;
                    tok_skip[tok_count] = 0;     tok_data[tok_count] = second_half; tok_count++;
                }
                half_idx_consumed = first_half_idx + 2;
            }

            if (fired_count > 0) {
                trace_remaining = 60;
                printf("[burst t=%llu us addr=0x%08X] %d rule(s):",
                       (unsigned long long)burst_t_us, base_addr, fired_count);
                for (int i = 0; i < fired_count; ++i) {
                    printf("  0x%08X->0x%08X", fired_addr[i], fired_resp[i]);
                }
                printf("\n  tokens:");
                for (int i = 0; i < tok_count; ++i) {
                    printf("  (skip=%u,data=0x%04X)", tok_skip[i], tok_data[i]);
                }
                printf("\n");
            }
        }

        #if DEBUG_SNOOP
        // DEBUG: IPL2 only reads the first word of the header at boot, then
        // jumps straight to copying IPL3 (cart addr 0x10000040+) one word at
        // a time — the name/CRC/country/version fields aren't touched until
        // IPL3 itself starts running, which can be many bursts later. So:
        // watch indefinitely (not just the first few bursts), but only log
        // when the address actually falls inside the header, to avoid
        // flooding serial with thousands of boot-code-copy lines.
        static bool seen_version_word = false;
        if (!seen_version_word && base_addr >= 0x10000000u && base_addr <= 0x1000003Cu) {
            uint16_t hw[32] = {0};
            int got = snoop_capture_burst(pio_snoop, sm_snoop, pio_cap, sm_cap, hw, 32, 2000);
            printf("[header addr=0x%08X] got=%d halfwords:", base_addr, got);
            snoop_print_hw(hw, got);
            // Version word (game ID + country + version) may arrive either
            // as its own burst at exactly 0x1000003C, or as part of a larger
            // burst that started earlier and happens to reach that offset.
            int idx_3c = (int)((0x1000003Cu - base_addr) / 2);
            if (idx_3c >= 0 && idx_3c + 1 < got) {
                char game_id[3] = { (char)(hw[idx_3c] >> 8), (char)(hw[idx_3c] & 0xFF), 0 };
                char country     = (char)(hw[idx_3c+1] >> 8);
                uint8_t version  = (uint8_t)(hw[idx_3c+1] & 0xFF);
                printf("  ==> Game ID: %s   Country: %c (0x%02X)   Version: 0x%02X\n",
                       game_id, country, (uint8_t)country, version);
                seen_version_word = true;
            }
        }

        // DEBUG: live-stream whatever the game actually reads around the
        // active override addresses, to verify actual driven bytes against
        // expected. Narrowed to just the 0x1091F338-0x1091F37C rule window
        // (+/- some slack) rather than the old wide 0x108C0000-0x10960000
        // envelope: the override words sit 27-58 halfwords into these
        // bursts, and the previous 64-halfword/2ms-per-halfword budget was
        // running out (hitting the timeout) before reaching them, capturing
        // only the unrelated preceding text ("The ch...", "Howeve[r]...").
        // 200 halfwords / 60ms-per-halfword comfortably covers that offset
        // and the known ~49ms mid-burst pause without over-widening the
        // window to unrelated background reads.
        if (base_addr >= 0x1091F200u && base_addr < 0x1091F400u) {
            static uint16_t hw[200];
            int got = snoop_capture_burst(pio_snoop, sm_snoop, pio_cap, sm_cap, hw, 200, 60000);
            if (got > 0) {
                printf("[msgwatch addr=0x%08X] got=%d:", base_addr, got);
                snoop_print_hw(hw, got);
            }
        }
        #endif // DEBUG_SNOOP

        // Periodic heartbeat only — keep the hot path free of UART output.
        if ((++burst_count & 0xFFF) == 0) {
            printf("bursts=%u last=0x%08X\n", burst_count, base_addr);
        }
    }
}
