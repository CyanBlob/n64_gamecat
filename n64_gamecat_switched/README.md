# N64 Gamecat (Switched)

An RP2350-based interposer that sits between an N64 cartridge and the console,
watches reads on the cart bus, and injects override values for chosen
addresses. Useful for cheats, patching protected boot words, or serving
synthetic data to the console.

This document covers the hardware topology, the N64 cartridge bus protocol as
it appears on the wire, and how the firmware's two PIO state machines plus the
C main loop cooperate to pull it off in real time.

---

## 1. Hardware topology

The board uses a bidirectional bus switch (CBT family) that can electrically
isolate the cartridge from the console's multiplexed address/data bus. The
`CART_EN` GPIO is wired directly to the CBT's active-low `/OE`, so
`CART_EN = 0` closes the switch (cart and console talk directly; the RP2350
just observes) and `CART_EN = 1` opens it (cart is cut off; the RP2350 drives
AD[15:0] for a single /READ pulse).

```
             console edge                              cartridge edge
                  │                                         │
      ALE_L ─────►┼───────────────────────────────────────► │
      ALE_H ─────►┼───────────────────────────────────────► │
      /READ ─────►┼───────────────────────────────────────► │
     /WRITE ─────►┼───────────────────────────────────────► │
                  │                                         │
                  │          ┌───────────────────┐          │
   AD[15:0] ◄════►┼═════════►│  CBT bus switch   │◄════════►│ AD[15:0]
                  │          │   OE = CART_EN    │          │
                  │          └─────────┬─────────┘          │
                  │                    │                    │
                  │            tap to RP2350 GPIO           │
                  │                    │                    │
                  ▼                    ▼                    ▼
            ┌────────────────────────────────────────────────────┐
            │                      RP2350                        │
            │                                                    │
            │   GPIO12..27 ─ AD[0..15]  (bidirectional)          │
            │   GPIO28     ─ ALE_L   (input)                     │
            │   GPIO29     ─ ALE_H   (input)                     │
            │   GPIO30     ─ /READ   (input)                     │
            │   GPIO31     ─ /WRITE  (input)                     │
            │   GPIO10     ─ CART_EN (output, sideset)           │
            │   GPIO11     ─ KILL    (input w/ pull-up)          │
            └────────────────────────────────────────────────────┘
```

Key properties this depends on:

- The CBT switch's propagation delay is single-digit ns, so toggling
  `CART_EN` effectively hands off the bus in one PIO cycle.
- AD pins are driven by the RP2350 *only* while `CART_EN = 0`, so there is no
  window where both sides drive the same net.
- KILL is a dead-man switch: pulling it low makes the firmware stop issuing
  overrides, leaving the cart in permanent pass-through.

---

## 2. The N64 cartridge bus, briefly

The cart bus is a multiplexed 16-bit bus. A single "burst" (one transaction)
looks like:

```
  Address phase                 Data phase (up to 256 words)
  ┌──────────────┐   ┌──┐   ┌──┐   ┌──┐
  │  latch addr  │   │RD│   │RD│   │RD│  ...
  └──────────────┘   └──┘   └──┘   └──┘

                  ▲       ▲       ▲
                  └─ each /READ low pulse delivers one 16-bit halfword.
                     A 32-bit word takes two consecutive /READ pulses.
```

The two ALE signals encode which 16 bits of the 32-bit address are on AD
during the address phase:

| ALE_L | ALE_H | AD[15:0] means                   |
| :---: | :---: | :------------------------------- |
|   1   |   1   | upper half of address (bits 31..16) |
|   1   |   0   | lower half of address (bits 15..0)  |

ALE_L stays high across both sub-phases; only ALE_H toggles within. The wiki
tells us AD is valid on the **falling** edge of each ALE — that's our sampling
rule.

After the address phase, the console issues /READ pulses in sequence. Each
pulse reads one 16-bit halfword from `address + 2*N` where N is the pulse
index within the burst. So an override for the 32-bit word at address A
requires driving AD twice: once on pulse 2*N (upper or lower half, depending
on bit 1 of A) and once on pulse 2*N+1 (the other half).

---

## 3. Firmware architecture at a glance

Three cooperating pieces:

```
  ┌─────────────────────┐     addr      ┌────────────────────┐
  │  n64_capture.pio    │ ─────────────►│  C main loop       │
  │  (watches ALE)      │   RX FIFO     │  (rule engine +    │
  └─────────────────────┘               │   token scheduler) │
                                        └─────────┬──────────┘
                                                  │ tokens
                                                  ▼      TX FIFO
                                        ┌────────────────────────┐
                                        │ n64_respond_tokens.pio │
                                        │ (drives AD + CART_EN)  │
                                        └────────────────────────┘
```

- **Capture SM** watches the ALE pins and pushes a 32-bit address into an
  RX FIFO every time a new burst starts.
- **C main loop** pops the address, looks up which of the next 256 words
  have override rules, and streams `(skip, data)` tokens to the responder.
- **Responder SM** consumes tokens; for each it passes N /READ pulses
  through transparently, then hijacks the next pulse to drive the override
  value on AD with the cartridge electrically disconnected.

The PIO state machines own the nanosecond-level timing; the C loop only
has to keep up at bus-burst granularity.

---

## 4. Capture state machine

### Program (`n64_capture.pio`)

```
wrap_target:
    wait 0 pin 17          ; ALE_H low  (we're between bursts, or in lower phase)
    wait 1 pin 17          ; ALE_H rising: upper-address phase has begun
    wait 0 pin 17          ; ALE_H falling: upper 16 bits of AD are valid
    in   pins, 16          ; ISR[15:0]  = AD[15:0]  (upper half of address)
    wait 0 pin 16          ; ALE_L falling: lower 16 bits of AD are valid
    in   pins, 16          ; ISR[31:16] = prev, ISR[15:0] = AD (lower half)
    push block             ; hand 32-bit address to CPU via RX FIFO
```

`in pins, 16` reads 16 consecutive GPIOs starting at `IN_BASE = AD_BASE`, i.e.
AD[15:0]. With the SM configured for shift-left, two successive `in pins, 16`
instructions leave ISR holding `(upper << 16) | lower` — the full 32-bit
address as the CPU expects it.

### Why these two edges

The wiki tells us AD is valid on the **falling** edge of each ALE, and the
bus timing diagram makes the shape clear:

```
           ┌─────────────────────────────────────────────┐
  ALE_L ───┘                                             └────────
                     ┌─────┐
  ALE_H ─────────────┘     └───────────────────────────────────────
                           ▲                             ▲
                           │                             │
                           A: upper addr valid here      B: lower addr valid here
                              (ALE_H falling)               (ALE_L falling)
```

So within one address burst we need **two** sample moments, and they're on
**different** pins: ALE_H goes high briefly and falls first (point A, upper
half); ALE_L stays high across the whole address phase and falls later
(point B, lower half).

The capture SM therefore waits on both pins, in order:

1. `wait 0 pin 17` / `wait 1 pin 17` — resync by watching ALE_H settle low
   and then rise at burst start. (Using ALE_H for resync is convenient
   because it's the pin that pulses; ALE_L just sits high across the whole
   address phase.)
2. `wait 0 pin 17` — ALE_H's falling edge = point A, upper half captured.
3. `wait 0 pin 16` — ALE_L's falling edge = point B, lower half captured.

Sampling on only one of the two pins would miss half the address — ALE_H
tells us nothing about when the lower half arrives, and ALE_L only gives us
one falling edge per burst.

---

## 5. Responder state machine

### Token format

The C code pushes 32-bit tokens into the responder's TX FIFO. Each token
describes how to handle exactly one /READ pulse:

```
   31                             16 15                             0
  ┌────────────────────────────────┬────────────────────────────────┐
  │          DATA (16 bits)        │          SKIP (16 bits)        │
  └────────────────────────────────┴────────────────────────────────┘

  SKIP = number of /READ pulses to let the real cart answer before we drive
  DATA = the 16-bit value to put on AD during the pulse AFTER those N skips
```

A 32-bit word override is therefore two tokens: one with a skip distance and
the upper-or-lower halfword, followed by one with `skip=0` and the other
halfword (which will land on the immediately-following /READ pulse).

### Program (`n64_respond_tokens.pio`)

```
wrap_target:
    pull block           side 1   ; wait for a token; CART_EN = 1 (cart connected)
    out  x, 16           side 1   ; X = SKIP
    out  y, 16           side 1   ; Y = DATA

    jmp  !x drive_now    side 1   ; SKIP == 0 → drive the very next /READ
    jmp  x-- skip_loop   side 1   ; pre-decrement so body runs exactly SKIP times

skip_loop:                         ; pass SKIP /READ pulses through to the cart
    wait 0 pin 18        side 1
    wait 1 pin 18        side 1
    jmp  x-- skip_loop   side 1

drive_now:
    wait 0 pin 18        side 0   ; /READ falling: cut the cart off the bus
    mov  osr, !null      side 0   ; OSR = 0xFFFFFFFF
    out  pindirs, 16     side 0   ; AD[0..15] → outputs
    mov  osr, y          side 0
    out  pins, 16        side 0   ; drive DATA on AD
    wait 1 pin 18        side 0   ; hold until /READ rises (console latches here)
    mov  osr, null       side 0
    out  pindirs, 16     side 0   ; AD[0..15] → inputs (release bus)
    nop                  side 1   ; reconnect cart and loop back for next token
```

### Flow per token

```
   ┌──────────┐    SKIP=0        ┌──────────┐
   │  wait    │─────────────────►│ drive    │
   │  for     │                  │ this     │
   │  token   │◄─── wrap ────────│ /READ    │
   └────┬─────┘                  └──────────┘
        │ SKIP > 0                    ▲
        ▼                             │
   ┌──────────┐   each /READ          │
   │skip_loop │────(cart answers)────►│
   │          │   X-- until 0         │
   └──────────┘                       │
        └──────── fall through ───────┘
```

### Why `jmp x-- skip_loop` appears twice

`jmp x--` is *post*-decrement: it tests X, takes the branch if X ≠ 0, then
decrements. That means loading X = N and entering a `jmp x-- loop` body runs
the body N+1 times, not N. The first `jmp x-- skip_loop` outside the loop
body is a pre-decrement trick: it always jumps (X ≥ 1 here because we already
handled SKIP = 0 above) and reduces X by 1 on the way in, so the loop body
then executes exactly SKIP times. Small instruction, easy to get wrong — the
initial version of this firmware had an off-by-one here.

### Why 16-bit drive needs `out pindirs`, not `set pindirs`

PIO's `set` instruction writes at most 5 pins at a time. Our AD bus is 16,
so `set pindirs, …` would silently drive only part of the bus and leave the
rest floating. Loading `OSR = 0xFFFFFFFF` and issuing `out pindirs, 16`
instead gives us a proper 16-bit-wide pindir flip, using the OUT pin
configuration set up from C.

---

## 6. The C main loop

Per-burst algorithm:

```
  1. Block until capture FIFO has an address.
  2. Read the 32-bit base address.
  3. Reset the responder SM + clear its FIFO + re-enable it.
     (This aligns the responder's skip-counting with the first /READ
      pulse of the new burst, and drops any leftover tokens.)
  4. If KILL is asserted, skip all scheduling → cart pass-through.
  5. For w in 0..255:
        addr_w = base_word + w*4
        if a rule matches addr_w:
            compute how many halfread pulses since our last token
            push token #1: (skip, first-half-of-word)
            push token #2: (0,    second-half-of-word)
```

The "first half" vs "second half" depends on bit 1 of the base address
(which half of a 32-bit word the console is reading first in this burst).
The scheduler only pushes tokens for *matching* rules, so a burst with no
rule hits produces no tokens and the responder sits at `pull block`
harmlessly — the cart answers every read unchanged.

### Rules

Rules are a small const table compared in `lookup_resp32`:

```c
typedef struct { uint32_t mask, match, resp32; bool enable; } rule_t;

static const rule_t rules[] = {
    { 0xFFFFFFFCu, 0x10000000u, 0x8037FF40u, false },  // first header word
    { 0xFFFF0000u, 0x12000000u, 0xDEADBEEFu, true  },  // 64KB window demo
};
```

A rule matches if `(addr_word & mask) == (match & mask)`. The mask is
compared at 32-bit word granularity, so the scheduler can iterate per-word
and the PIO side doesn't need to know about rules at all.

---

## 7. Timing budget

Numbers below are at the current 300 MHz system clock (1 PIO cycle = 3.3 ns).

| Event                                          | Budget (bus)   | Actual (RP2350)   | Margin |
| :--------------------------------------------- | :------------- | :---------------- | :----- |
| /READ falling → RP2350 drives AD                | ~600 ns        | ~5 PIO cycles ≈ 17 ns + sync latency | ~35× |
| Address latched → token0 ready in FIFO          | ~1 µs (fast bus) | scheduler-bound, single-digit µs worst case | tight-but-fine if word 0 isn't a hit |
| Cart isolate → cart release on bus              | —              | CBT switch prop ~5 ns | safe |

The PIO side has roughly an order of magnitude of headroom on every
critical edge. The only tight window is the C scheduler's turnaround
between the end of address latching and the first /READ pulse, and only
if word 0 of that burst is a rule hit. If that ever becomes an issue,
options are: precompute per-address token lists, skip the full SM restart
and use a burst-generation counter instead, or hand the scheduler to core
1.

---

## 8. File map

| File                          | What it is                                              |
| :---------------------------- | :------------------------------------------------------ |
| `n64_capture.pio`             | PIO program that latches the 32-bit cart bus address    |
| `n64_respond_tokens.pio`      | PIO program that skips N reads then drives one override |
| `n64_gamecat_switched.cpp`    | Rule table, token scheduler, PIO setup                  |
| `CMakeLists.txt`              | Pico SDK build config (board: solderparty_rp2350_stamp_xl) |

---

## 9. Bring-up checklist

- [ ] Scope ALE_L, ALE_H, /READ with no firmware running — confirm the cart
      bus is behaving as the wiki describes.
- [ ] Flash firmware, set one rule to match some boring cart address, scope
      CART_EN: it should briefly drop once per matching halfread.
- [ ] Add a rule whose first read is visible in-game (e.g. a known texture or
      string) and confirm the override shows up.
- [ ] Pull KILL low and confirm overrides stop immediately.
