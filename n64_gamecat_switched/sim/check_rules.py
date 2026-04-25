#!/usr/bin/env python3
"""
Static check for n64_gamecat_switched cheat rules.

Parses the rules[] table out of the C source and warns about any enabled
rule whose match address falls inside the bootcode CRC window
(0x10001000 .. 0x10101000), unless N64_CIC_TYPE is set to a CIC family with
a documented bypass NOP at that exact address.

Usage:
    python3 sim/check_rules.py n64_gamecat_switched.cpp
    python3 sim/check_rules.py n64_gamecat_switched.cpp --cic 6105
"""

import argparse
import re
import sys
from pathlib import Path

# Cart-bus addresses covered by the bootcode CRC.
CRC_WINDOW_START = 0x10001000
CRC_WINDOW_END   = 0x10101000   # exclusive

# Per-CIC bypass-rule offsets (cart-bus addresses), from the en64 wiki.
CIC_BYPASS = {
    6101:  (0x10000670, 0x1000067C),
    6102:  (0x1000066C, 0x10000678),
    7101:  (0x1000066C, 0x10000678),
    6103:  (0x1000063C, 0x10000648),
    7103:  (0x1000063C, 0x10000648),
    6105:  (0x1000077C, 0x10000788),
    7105:  (0x1000077C, 0x10000788),
}


def parse_rules(src: str):
    """Pull every `{ MASK, MATCH, RESP, ENABLE }` four-tuple out of the source."""
    rule_re = re.compile(
        r"\{\s*"
        r"(0x[0-9A-Fa-f]+)u?\s*,\s*"
        r"(0x[0-9A-Fa-f]+)u?\s*,\s*"
        r"(0x[0-9A-Fa-f]+)u?\s*,\s*"
        r"(true|false)\s*"
        r"\}",
    )
    return [
        {
            "mask": int(m.group(1), 16),
            "match": int(m.group(2), 16),
            "resp": int(m.group(3), 16),
            "enable": m.group(4) == "true",
        }
        for m in rule_re.finditer(src)
    ]


def detect_cic(src: str) -> int:
    """Find the project's compile-time N64_CIC_TYPE default."""
    m = re.search(r"#define\s+N64_CIC_TYPE\s+(\d+)", src)
    return int(m.group(1)) if m else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("source", help="Path to n64_gamecat_switched.cpp")
    ap.add_argument(
        "--cic", type=int, default=None,
        help="Override N64_CIC_TYPE (e.g. 6105 for OoT). "
             "If omitted, parses the default from the source.",
    )
    args = ap.parse_args()

    src = Path(args.source).read_text()
    rules = parse_rules(src)
    cic = args.cic if args.cic is not None else detect_cic(src)

    print(f"Found {len(rules)} rule(s).  N64_CIC_TYPE = {cic}")
    print()

    bypass_addrs = set(CIC_BYPASS.get(cic, ()))
    bypass_present = bool(bypass_addrs) and bypass_addrs.issubset(
        {r["match"] for r in rules if r["enable"]}
    )

    if cic == 0:
        print("ℹ  N64_CIC_TYPE = 0: no bootcode CRC bypass active.")
        print(f"   Any enabled rule in 0x{CRC_WINDOW_START:08X}..0x{CRC_WINDOW_END:08X} will trip the CRC.")
    elif not CIC_BYPASS.get(cic):
        print(f"⚠  CIC {cic} has no documented bypass offsets in this checker.")
        print(f"   Treating in-window rules as unsafe.")
    elif bypass_present:
        print(f"✓ CIC {cic} bypass NOPs present — in-window patches are OK.")
    else:
        print(f"⚠ CIC {cic} bypass NOPs declared but not all enabled in rules[].")
    print()

    fails = 0
    for i, r in enumerate(rules):
        in_window = CRC_WINDOW_START <= r["match"] < CRC_WINDOW_END
        is_bypass_rule = r["match"] in bypass_addrs
        en_str = "on " if r["enable"] else "off"
        line = (
            f"  [{i:2d}] match=0x{r['match']:08X} mask=0x{r['mask']:08X} "
            f"resp=0x{r['resp']:08X} {en_str}"
        )

        if is_bypass_rule:
            print(f"✓ {line}  (CIC {cic} bypass NOP)")
        elif in_window and r["enable"] and not bypass_present:
            print(f"✗ {line}  ⚠ INSIDE CRC WINDOW")
            fails += 1
        elif in_window and r["enable"]:
            print(f"  {line}  (in CRC window — covered by bypass)")
        elif in_window:
            print(f"  {line}  (in window but disabled)")
        else:
            print(f"  {line}  (outside CRC window)")

    print()
    if fails:
        print(f"FAIL: {fails} enabled rule(s) inside the CRC window with no bypass active.")
        if cic == 0:
            print("Hint: set N64_CIC_TYPE for your cart, or pass --cic to this checker.")
        return 1
    print("OK.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
