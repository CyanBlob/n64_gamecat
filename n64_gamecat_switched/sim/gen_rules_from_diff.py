#!/usr/bin/env python3
"""
Generate Gamecat interceptor rules from a byte-level ROM diff.

Takes an original ROM and a set of find/replace byte strings (same length,
so message-box layout doesn't shift), builds an in-memory patched copy,
diffs it against the original, and emits word-aligned `rule_t` C entries
covering every changed byte.

Usage:
    python3 sim/gen_rules_from_diff.py baserom.z64 fairy:butt_ Fairy:Butt_
    (use '_' for a literal space in find/replace pairs, since argv can't
    carry trailing spaces cleanly)
"""

import struct
import sys
from pathlib import Path

CART_BASE = 0x10000000


def find_all(data: bytes, needle: bytes) -> list[int]:
    offs = []
    start = 0
    while True:
        i = data.find(needle, start)
        if i == -1:
            break
        offs.append(i)
        start = i + 1
    return offs


def apply_replacements(data: bytearray, pairs: list[tuple[bytes, bytes]]) -> list[int]:
    """Apply each find/replace pair at every occurrence. Returns list of
    byte offsets that were actually modified (for reporting)."""
    touched = []
    for find, repl in pairs:
        assert len(find) == len(repl), (
            f"find/replace length mismatch: {find!r} ({len(find)}) vs "
            f"{repl!r} ({len(repl)}) — must match to avoid shifting "
            f"subsequent message bytes"
        )
        for off in find_all(bytes(data), find):
            data[off:off + len(repl)] = repl
            touched.extend(range(off, off + len(repl)))
    return touched


def diff_to_rules(orig: bytes, patched: bytes) -> list[tuple[int, int, int]]:
    """Word-align every changed byte range and emit (mask, match, resp) triples."""
    assert len(orig) == len(patched)
    changed_words = set()
    for i in range(len(orig)):
        if orig[i] != patched[i]:
            changed_words.add(i & ~0x3)

    rules = []
    for word_off in sorted(changed_words):
        cart_addr = CART_BASE + word_off
        resp = struct.unpack(">I", patched[word_off:word_off + 4])[0]
        rules.append((0xFFFFFFFC, cart_addr, resp))
    return rules


def main() -> None:
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    rom_path = Path(sys.argv[1])
    orig = bytearray(rom_path.read_bytes())
    patched = bytearray(orig)

    pairs = []
    for arg in sys.argv[2:]:
        find_s, repl_s = arg.split(":", 1)
        find_b = find_s.replace("_", " ").encode("ascii")
        repl_b = repl_s.replace("_", " ").encode("ascii")
        pairs.append((find_b, repl_b))

    touched = apply_replacements(patched, pairs)
    rules = diff_to_rules(bytes(orig), bytes(patched))

    print(f"// {len(touched)} bytes changed -> {len(rules)} word-aligned rule(s)")
    print(f"// Generated from {rom_path.name}, pairs: "
          f"{[(f.decode(), r.decode()) for f, r in pairs]}")
    for mask, match, resp in rules:
        print(f"    {{ 0x{mask:08X}u, 0x{match:08X}u, 0x{resp:08X}u, true }},")


if __name__ == "__main__":
    main()
