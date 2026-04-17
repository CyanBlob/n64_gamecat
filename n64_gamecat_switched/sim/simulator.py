#!/usr/bin/env python3
"""
Hybrid simulator for the N64 Gamecat interposer.

What it does:
  - Parses the project's two .pio files (subset of PIO assembly).
  - Runs both state machines cycle-by-cycle at a configurable PIO clock.
  - Models the C main loop as a bounded-latency agent that polls the capture
    FIFO and feeds tokens into the responder FIFO.
  - Generates a synthetic N64 cartridge-bus trace: address latch + /READ burst.
  - Writes a VCD file viewable in GTKWave, PulseView, or any waveform viewer.

What it does NOT do:
  - Cycle-accurate Cortex-M33 execution. The C main loop is approximated with
    fixed per-operation latencies. Tune CHost.LATENCY_* to match measurements
    once you have real hardware.
  - Signal-integrity effects (ringing, glitches, metastability). Every pin is
    either 0 or 1.
  - Real arbitration between Pico and cart on AD. We approximate: when
    CART_EN=1 (isolated), the Pico drives; otherwise the bus does.

Usage:
    python3 simulator.py
    gtkwave gamecat.vcd   # or pulseview, surfer, ...
"""

import re
from collections import deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Tuple


# ============================================================================
# Configuration
# ============================================================================

PIO_CLOCK_HZ = 300_000_000
NS_PER_CYCLE = 1e9 / PIO_CLOCK_HZ  # ~3.33 ns

# Pin layout matches the firmware (AD_BASE = 0 on Pico 2)
AD_BASE       = 0
ALE_L_PIN     = 16
ALE_H_PIN     = 17
READ_N_PIN    = 18
WRITE_N_PIN   = 19
CART_EN_PIN   = 20
NUM_PINS      = 32


# ============================================================================
# PIO assembly parser (subset)
# ============================================================================

@dataclass
class Insn:
    op: str                  # 'wait', 'in', 'out', 'jmp', 'push', 'pull', 'mov', 'nop'
    args: tuple = ()
    sideset: Optional[int] = None
    label: Optional[str] = None


def parse_pio(path: Path) -> List[Insn]:
    """Parse the subset of PIO assembly the project actually uses."""
    raw_lines = []
    for line in Path(path).read_text().splitlines():
        line = re.sub(r";.*$", "", line).strip()
        if line:
            raw_lines.append(line)

    labels: dict[str, int] = {}
    insns: List[Insn] = []

    for line in raw_lines:
        if line.startswith(".") or line in ("wrap", "wrap_target"):
            # Directives / markers we don't need to model
            if line == "wrap_target":
                labels["wrap_target"] = len(insns)
            continue

        m = re.match(r"^(\w+):\s*(.*)$", line)
        if m:
            labels[m.group(1)] = len(insns)
            rest = m.group(2).strip()
            if not rest:
                continue
            line = rest

        insns.append(parse_insn_line(line))

    # If wrap_target wasn't seen, default to index 0
    labels.setdefault("wrap_target", 0)

    # Resolve jump targets
    for insn in insns:
        if insn.op == "jmp":
            if insn.label not in labels:
                raise ValueError(f"Unknown label: {insn.label}")
            insn.args = (*insn.args, labels[insn.label])

    return insns


def parse_insn_line(line: str) -> Insn:
    # Peel off `side N`
    sideset = None
    m = re.search(r"side\s+(\d+)\s*$", line)
    if m:
        sideset = int(m.group(1))
        line = line[: m.start()].strip()

    toks = line.split()
    op = toks[0].lower()

    if op == "wait":
        # wait POL pin IDX
        pol = int(toks[1])
        idx = int(toks[3])
        return Insn("wait", (pol, idx), sideset)

    if op == "in":
        # in pins, N
        src = toks[1].rstrip(",").lower()
        n = int(toks[2])
        return Insn("in", (src, n), sideset)

    if op == "out":
        dst = toks[1].rstrip(",").lower()
        n = int(toks[2])
        return Insn("out", (dst, n), sideset)

    if op == "jmp":
        if len(toks) == 2:
            cond, label = None, toks[1]
        else:
            cond, label = toks[1].lower(), toks[2]
        insn = Insn("jmp", (cond,), sideset)
        insn.label = label
        return insn

    if op == "push":
        mod = toks[1] if len(toks) > 1 else "block"
        return Insn("push", (mod,), sideset)

    if op == "pull":
        mod = toks[1] if len(toks) > 1 else "block"
        return Insn("pull", (mod,), sideset)

    if op == "mov":
        dst = toks[1].rstrip(",").lower()
        src_raw = " ".join(toks[2:]).strip()
        invert = src_raw.startswith("!")
        if invert:
            src_raw = src_raw[1:].strip()
        return Insn("mov", (dst, src_raw.lower(), invert), sideset)

    if op == "nop":
        return Insn("nop", (), sideset)

    raise ValueError(f"Unsupported op: {op}")


# ============================================================================
# Shared pin state
# ============================================================================

class PinState:
    """
    Tracks pin drivers. Three sources:
        bus[i]        - what the N64 console/cart is driving (None if not driving)
        pico_drive[i] - what the Pico last wrote via `out pins`
        pico_dir[i]   - 1 if the Pico has this as an output
    Arbitration for AD pins uses CART_EN to pick Pico (isolated) or bus.
    """

    def __init__(self):
        self.bus: List[Optional[int]] = [None] * NUM_PINS
        self.pico_drive: List[int] = [0] * NUM_PINS
        self.pico_dir: List[int] = [0] * NUM_PINS

    def _resolve_ctrl(self, pin: int) -> int:
        """Non-AD pin: bus wins if driving, else Pico, else 0."""
        if self.bus[pin] is not None:
            return self.bus[pin]
        if self.pico_dir[pin]:
            return self.pico_drive[pin]
        return 0

    def resolve(self, pin: int) -> int:
        # The Pico is tapped on the *console side* of the CBT, so its view
        # of each pin is: "Pico drives if pindir=1, else whatever the bus
        # (console OR cart via CBT) is driving, else 0."
        #
        # Cart-en arbitration is modeled implicitly: when the responder
        # raises cart_en, it also flips AD pindirs to output, so the pico_dir
        # check wins naturally.
        if self.pico_dir[pin]:
            return self.pico_drive[pin]
        if self.bus[pin] is not None:
            return self.bus[pin]
        return 0

    def read_packed(self, base: int, count: int) -> int:
        v = 0
        for i in range(count):
            v |= self.resolve(base + i) << i
        return v

    def write_pins(self, base: int, count: int, val: int) -> None:
        for i in range(count):
            self.pico_drive[base + i] = (val >> i) & 1

    def write_pindirs(self, base: int, count: int, val: int) -> None:
        for i in range(count):
            self.pico_dir[base + i] = (val >> i) & 1

    def set_sideset(self, base: int, val: int) -> None:
        # Our PIO uses 1-bit sideset on CART_EN
        self.pico_drive[base] = val & 1
        self.pico_dir[base] = 1


# ============================================================================
# PIO state machine
# ============================================================================

FIFO_DEPTH = 8


class PioSm:
    def __init__(
        self,
        program: List[Insn],
        name: str,
        in_base: int = 0,
        out_base: int = 0,
        out_count: int = 16,
        sideset_base: Optional[int] = None,
        shift_right_in: bool = False,
        shift_right_out: bool = True,
        autopull: bool = True,
        autopush: bool = False,
        shift_threshold: int = 32,
    ):
        self.program = program
        self.name = name
        self.in_base = in_base
        self.out_base = out_base
        self.out_count = out_count
        self.sideset_base = sideset_base
        self.shift_right_in = shift_right_in
        self.shift_right_out = shift_right_out
        self.autopull = autopull
        self.autopush = autopush
        self.shift_threshold = shift_threshold

        self.pc = 0
        self.x = 0
        self.y = 0
        self.isr = 0
        self.isr_count = 0
        self.osr = 0
        self.osr_count = 32          # 32 means OSR is "empty" for autopull purposes
        self.rx_fifo: deque = deque(maxlen=FIFO_DEPTH)
        self.tx_fifo: deque = deque(maxlen=FIFO_DEPTH)

        self.stalled_reason: Optional[str] = None

    # ---- execute one cycle ----
    def step(self, pins: PinState) -> None:
        insn = self.program[self.pc % len(self.program)]

        # Sideset is asserted every cycle we're on this instruction, even while stalled
        if insn.sideset is not None and self.sideset_base is not None:
            pins.set_sideset(self.sideset_base, insn.sideset)

        op = insn.op

        if op == "wait":
            pol, idx = insn.args
            if pins.resolve(self.in_base + idx) == pol:
                self.pc += 1
                self.stalled_reason = None
            else:
                self.stalled_reason = f"wait pin{self.in_base + idx}={pol}"

        elif op == "in":
            src, n = insn.args
            assert src == "pins", "only `in pins, N` supported"
            val = pins.read_packed(self.in_base, n)
            if self.shift_right_in:
                self.isr = ((self.isr >> n) | (val << (32 - n))) & 0xFFFFFFFF
            else:
                self.isr = ((self.isr << n) | val) & 0xFFFFFFFF
            self.isr_count = min(32, self.isr_count + n)
            self.pc += 1
            if self.autopush and self.isr_count >= self.shift_threshold:
                if len(self.rx_fifo) < FIFO_DEPTH:
                    self.rx_fifo.append(self.isr)
                    self.isr = 0
                    self.isr_count = 0

        elif op == "out":
            dst, n = insn.args
            # Autopull: refill OSR if empty (at or past threshold)
            if self.autopull and self.osr_count >= self.shift_threshold:
                if self.tx_fifo:
                    self.osr = self.tx_fifo.popleft()
                    self.osr_count = 0
                else:
                    self.stalled_reason = "autopull waiting for TX"
                    return
            if self.shift_right_out:
                val = self.osr & ((1 << n) - 1)
                self.osr >>= n
            else:
                val = (self.osr >> (32 - n)) & ((1 << n) - 1)
                self.osr = (self.osr << n) & 0xFFFFFFFF
            self.osr_count = min(32, self.osr_count + n)

            if dst == "x":
                self.x = val
            elif dst == "y":
                self.y = val
            elif dst == "pins":
                pins.write_pins(self.out_base, self.out_count, val)
            elif dst == "pindirs":
                pins.write_pindirs(self.out_base, self.out_count, val)
            else:
                raise ValueError(f"unsupported out dst: {dst}")
            self.pc += 1
            self.stalled_reason = None

        elif op == "jmp":
            cond, target = insn.args
            take = False
            if cond is None:
                take = True
            elif cond == "!x":
                take = (self.x == 0)
            elif cond == "x--":
                take = (self.x != 0)
                self.x = (self.x - 1) & 0xFFFFFFFF
            elif cond == "!y":
                take = (self.y == 0)
            elif cond == "y--":
                take = (self.y != 0)
                self.y = (self.y - 1) & 0xFFFFFFFF
            else:
                raise ValueError(f"unsupported jmp cond: {cond}")
            self.pc = target if take else self.pc + 1
            self.stalled_reason = None

        elif op == "push":
            if len(self.rx_fifo) < FIFO_DEPTH:
                self.rx_fifo.append(self.isr)
                self.isr = 0
                self.isr_count = 0
                self.pc += 1
                self.stalled_reason = None
            else:
                self.stalled_reason = "push RX full"

        elif op == "pull":
            if self.tx_fifo:
                self.osr = self.tx_fifo.popleft()
                self.osr_count = 0
                self.pc += 1
                self.stalled_reason = None
            else:
                self.stalled_reason = "pull TX empty"

        elif op == "mov":
            dst, src, invert = insn.args
            if src == "null":
                v = 0
            elif src == "y":
                v = self.y
            elif src == "x":
                v = self.x
            elif src == "osr":
                v = self.osr
            elif src == "isr":
                v = self.isr
            else:
                raise ValueError(f"unsupported mov src: {src}")
            if invert:
                v = (~v) & 0xFFFFFFFF
            if dst == "osr":
                self.osr = v
                self.osr_count = 0       # refilled; autopull won't re-trigger
            elif dst == "isr":
                self.isr = v
                self.isr_count = 0
            elif dst == "x":
                self.x = v
            elif dst == "y":
                self.y = v
            else:
                raise ValueError(f"unsupported mov dst: {dst}")
            self.pc += 1
            self.stalled_reason = None

        elif op == "nop":
            self.pc += 1
            self.stalled_reason = None

        else:
            raise ValueError(f"unsupported op: {op}")

    # ---- reset (used by the C host on new burst) ----
    def soft_reset(self) -> None:
        self.pc = 0
        self.x = 0
        self.y = 0
        self.isr = 0
        self.isr_count = 0
        self.osr = 0
        self.osr_count = 32
        self.rx_fifo.clear()
        self.tx_fifo.clear()
        self.stalled_reason = None


# ============================================================================
# C main loop model (bounded latency)
# ============================================================================

@dataclass
class Rule:
    mask: int
    match: int
    resp: int
    enable: bool = True


class CHost:
    """
    Approximates the Cortex-M33 main loop. Tune these numbers to match what
    you actually measure on hardware — or make them worst-case conservative
    to be sure the design holds up.
    """

    # Latencies in cycles (at PIO_CLOCK_HZ). Conservative-ish starting values.
    LATENCY_FIFO_POLL   = 5    # one RX empty check + branch
    LATENCY_BURST_RESET = 300  # SM disable/clear/restart (~1 µs at 300 MHz)
    LATENCY_LOOP_ITER   = 12   # per-word scan when no match
    LATENCY_RULE_MATCH  = 40   # extra when a rule matches (work + 2× put)

    def __init__(self, rules: List[Rule], pio_cap: PioSm, pio_rsp: PioSm):
        self.rules = rules
        self.pio_cap = pio_cap
        self.pio_rsp = pio_rsp

        self.wait = 0
        self.state = "idle"
        self.pending_tokens: list[int] = []
        self.bursts_seen = 0

    def tick(self) -> None:
        if self.wait > 0:
            self.wait -= 1
            return

        if self.pending_tokens:
            # Drain queued tokens into the responder FIFO
            if len(self.pio_rsp.tx_fifo) < FIFO_DEPTH:
                self.pio_rsp.tx_fifo.append(self.pending_tokens.pop(0))
                self.wait = 8   # `pio_sm_put_blocking` overhead
            else:
                self.wait = 2   # spin until space
            return

        if self.state == "idle":
            self.wait = self.LATENCY_FIFO_POLL
            if self.pio_cap.rx_fifo:
                addr = self.pio_cap.rx_fifo.popleft()
                self.bursts_seen += 1
                print(f"[CHost] burst #{self.bursts_seen}: addr=0x{addr:08X}")
                # Reset responder so it aligns with the first /READ of this burst
                self.pio_rsp.soft_reset()
                self.pending_tokens = self._build_tokens(addr)
                self.wait = self.LATENCY_BURST_RESET

    def _build_tokens(self, addr: int) -> list[int]:
        base = addr & ~0x3
        first_is_high = (addr & 0x2) != 0
        half_consumed = 0
        tokens: list[int] = []
        latency = 0
        for w in range(256):
            addr_w = (base + (w << 2)) & 0xFFFFFFFF
            resp = None
            for r in self.rules:
                if not r.enable:
                    continue
                if (addr_w & r.mask) == (r.match & r.mask):
                    resp = r.resp
                    break
            latency += self.LATENCY_LOOP_ITER
            if resp is None:
                continue
            lo = resp & 0xFFFF
            hi = (resp >> 16) & 0xFFFF
            first_h = hi if first_is_high else lo
            second_h = lo if first_is_high else hi
            first_idx = 2 * w
            skip = (first_idx - half_consumed) & 0xFFFF
            # Token layout: (data << 16) | skip
            tokens.append((first_h << 16) | skip)
            tokens.append((second_h << 16) | 0)
            half_consumed = first_idx + 2
            latency += self.LATENCY_RULE_MATCH
        # Fold scheduling loop latency into the initial reset wait
        self.wait += latency
        return tokens


# ============================================================================
# N64 bus trace generator
# ============================================================================

class N64Bus:
    """
    Drives ALE_L, ALE_H, READ_N, WRITE_N, and AD as a synthetic cartridge bus.
    A "burst" consists of:
      - ALE_L rises, ALE_H rises    (both t0, upper half on AD)
      - ALE_H falls at t0+200 ns
      - lower half on AD
      - ALE_L falls at t0+1040 ns
      - after a gap, N /READ pulses with cart data on AD during each
    """

    # Timing (nanoseconds)
    T_ADDR_START      = 1_000
    T_ALE_H_FALL_OFS  = 230     # from burst start
    T_ALE_L_FALL_OFS  = 1_040
    T_LOWER_DRIVE_OFS = 260     # when lower half appears on AD
    T_FIRST_READ_OFS  = 2_000   # first /READ falling edge after burst start
    T_READ_LOW_NS     = 500
    T_READ_PERIOD_NS  = 1_000

    def __init__(self, pins: PinState):
        self.pins = pins
        self.events: list[Tuple[float, callable]] = []
        self.idx = 0
        # Idle levels for strobes
        self._drive(ALE_L_PIN, 0)
        self._drive(ALE_H_PIN, 0)
        self._drive(READ_N_PIN, 1)
        self._drive(WRITE_N_PIN, 1)

    def _drive(self, pin: int, val: int) -> None:
        self.pins.bus[pin] = val

    def _drive_ad(self, val: int) -> None:
        for i in range(16):
            self.pins.bus[AD_BASE + i] = (val >> i) & 1

    def _release_ad(self) -> None:
        for i in range(16):
            self.pins.bus[AD_BASE + i] = None

    def queue_burst(self, t_start: float, address: int, cart_data: list[int]) -> None:
        upper = (address >> 16) & 0xFFFF
        lower = address & 0xFFFF

        def at(t, fn):
            self.events.append((t, fn))

        # Address phase
        at(t_start,                           lambda: self._drive(ALE_L_PIN, 1))
        at(t_start,                           lambda: self._drive(ALE_H_PIN, 1))
        at(t_start + 20,                      lambda: self._drive_ad(upper))
        at(t_start + self.T_ALE_H_FALL_OFS,   lambda: self._drive(ALE_H_PIN, 0))
        at(t_start + self.T_LOWER_DRIVE_OFS,  lambda: self._drive_ad(lower))
        at(t_start + self.T_ALE_L_FALL_OFS,   lambda: self._drive(ALE_L_PIN, 0))
        at(t_start + self.T_ALE_L_FALL_OFS + 20, self._release_ad)

        # /READ burst
        for i, data in enumerate(cart_data):
            pulse_t = t_start + self.T_FIRST_READ_OFS + i * self.T_READ_PERIOD_NS
            at(pulse_t - 100, lambda d=data: self._drive_ad(d))
            at(pulse_t,                         lambda: self._drive(READ_N_PIN, 0))
            at(pulse_t + self.T_READ_LOW_NS,    lambda: self._drive(READ_N_PIN, 1))
            at(pulse_t + self.T_READ_LOW_NS + 50, self._release_ad)

        self.events.sort(key=lambda e: e[0])

    def tick(self, now_ns: float) -> None:
        while self.idx < len(self.events) and self.events[self.idx][0] <= now_ns:
            self.events[self.idx][1]()
            self.idx += 1


# ============================================================================
# VCD writer
# ============================================================================

class VcdWriter:
    def __init__(self, path: Path, signals: list[tuple[str, int]]):
        self.file = open(path, "w")
        self.signals = signals        # [(name, width)]
        self.ids = {name: chr(33 + i) for i, (name, _) in enumerate(signals)}
        self.last: dict[str, int] = {}
        self._write_header()

    def _write_header(self) -> None:
        f = self.file
        f.write("$timescale 1ns $end\n")
        f.write("$scope module gamecat $end\n")
        for name, width in self.signals:
            f.write(f"$var wire {width} {self.ids[name]} {name} $end\n")
        f.write("$upscope $end\n")
        f.write("$enddefinitions $end\n")
        f.write("#0\n$dumpvars\n")
        for name, width in self.signals:
            v = 0
            if width == 1:
                f.write(f"0{self.ids[name]}\n")
            else:
                f.write(f"b0 {self.ids[name]}\n")
            self.last[name] = v
        f.write("$end\n")

    def sample(self, t_ns: float, values: dict[str, int]) -> None:
        pending = []
        for name, v in values.items():
            if self.last.get(name) != v:
                pending.append((name, v))
                self.last[name] = v
        if not pending:
            return
        self.file.write(f"#{int(t_ns)}\n")
        widths = dict(self.signals)
        for name, v in pending:
            c = self.ids[name]
            w = widths[name]
            if w == 1:
                self.file.write(f"{v & 1}{c}\n")
            else:
                bits = bin(v & ((1 << w) - 1))[2:]
                self.file.write(f"b{bits} {c}\n")

    def close(self) -> None:
        self.file.close()


# ============================================================================
# Main
# ============================================================================

def main() -> None:
    here = Path(__file__).resolve().parent
    project = here.parent

    cap_prog = parse_pio(project / "n64_capture.pio")
    rsp_prog = parse_pio(project / "n64_respond_tokens.pio")

    pins = PinState()

    pio_cap = PioSm(
        cap_prog, name="capture",
        in_base=AD_BASE,
        shift_right_in=False,    # shift_left so (upper<<16)|lower
        autopush=False,
    )

    pio_rsp = PioSm(
        rsp_prog, name="responder",
        in_base=AD_BASE,
        out_base=AD_BASE, out_count=16,
        sideset_base=CART_EN_PIN,
        shift_right_out=True,
        autopull=True,
    )

    # Cheat rule: match one specific 32-bit word → serve 0xDEADBEEF.
    # Word at base+4 (i.e. the 2nd word of the burst) will get overridden;
    # everything else passes through.
    rules = [
        Rule(mask=0xFFFFFFFC, match=0x12000004, resp=0xDEADBEEF),
    ]
    chost = CHost(rules, pio_cap, pio_rsp)

    # Bus traces: two bursts.
    bus = N64Bus(pins)
    # Burst 1: address 0x12000000, read 4 words (= 8 halfreads).
    # The 2nd word (halfreads 2 and 3) should come back as 0xBEEF then 0xDEAD,
    # instead of the cart's 0x3333 / 0x4444.
    cart1 = [0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888]
    bus.queue_burst(t_start=1_000,  address=0x12000000, cart_data=cart1)
    # Burst 2: address 0x10000000, no match — all cart values pass through.
    cart2 = [0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD]
    bus.queue_burst(t_start=20_000, address=0x10000000, cart_data=cart2)

    # VCD signals to record
    signals: list[tuple[str, int]] = [
        ("ALE_L",    1),
        ("ALE_H",    1),
        ("READ_N",   1),
        ("WRITE_N",  1),
        ("CART_EN",  1),
        ("AD",      16),
    ]
    vcd = VcdWriter(here / "gamecat.vcd", signals)

    total_ns = 50_000
    cycles = int(total_ns / NS_PER_CYCLE)

    for cycle in range(cycles):
        t = cycle * NS_PER_CYCLE
        bus.tick(t)
        pio_cap.step(pins)
        pio_rsp.step(pins)
        chost.tick()

        vcd.sample(t, {
            "ALE_L":   pins.resolve(ALE_L_PIN),
            "ALE_H":   pins.resolve(ALE_H_PIN),
            "READ_N":  pins.resolve(READ_N_PIN),
            "WRITE_N": pins.resolve(WRITE_N_PIN),
            "CART_EN": pins.resolve(CART_EN_PIN),
            "AD":      pins.read_packed(AD_BASE, 16),
        })

    vcd.close()
    print(f"Simulated {total_ns / 1000:.1f} µs ({cycles} cycles at {PIO_CLOCK_HZ/1e6:.0f} MHz).")
    print(f"Wrote VCD to: {here / 'gamecat.vcd'}")
    print()
    print("View with:  gtkwave sim/gamecat.vcd")
    print("         or surfer sim/gamecat.vcd")


if __name__ == "__main__":
    main()
