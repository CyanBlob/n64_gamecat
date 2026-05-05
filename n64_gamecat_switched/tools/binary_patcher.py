#!/usr/bin/env python3
"""
binary_patcher.py — N64 ROM string/binary patcher for n64_gamecat_switched

Searches an N64 ROM for patterns defined in a JSON rules file, writes a
patched ROM, and generates a C header containing patch_rule_t entries that
the MCU firmware can #include to intercept the matching addresses at runtime.

The patched ROM is for reference / emulator testing; the C header is the
artifact the MCU actually uses — it intercepts reads at the original ROM
addresses and returns the replacement data without modifying flash.

Usage:
    python3 tools/binary_patcher.py ROM.z64 RULES.json [options]
    python3 tools/binary_patcher.py ROM.z64 RULES.json -o patched.z64 --header patches.h

Rules file format (JSON array):
    [
      {
        "type":    "text",            -- "text" (ASCII) or "hex" (hex string)
        "search":  "ZELDA",           -- pattern to locate in the ROM
        "replace": "HACKS!",          -- replacement (≤ len(search); null-padded if shorter)
        "comment": "title patch"      -- optional label used in the header comments
      },
      {
        "type":    "hex",
        "search":  "DEADBEEF01234567",
        "replace": "CAFEBABE87654321"
      }
    ]
"""

import argparse
import json
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

# N64 cart-bus base address for ROM (PI domain 1 / 0x10000000–0x1FFFFFFF).
CART_BASE = 0x10000000

# ROM format magic values (first 4 bytes).
MAGIC_Z64 = bytes([0x80, 0x37, 0x12, 0x40])  # big-endian (native)
MAGIC_V64 = bytes([0x37, 0x80, 0x40, 0x12])  # byteswapped (16-bit word swap)
MAGIC_N64 = bytes([0x40, 0x12, 0x37, 0x80])  # little-endian (32-bit word swap)


# ---------------------------------------------------------------------------
# ROM format detection & normalisation
# ---------------------------------------------------------------------------

def detect_rom_format(data: bytes) -> str:
    """Return 'z64', 'v64', or 'n64' based on the ROM magic, or raise."""
    magic = data[:4]
    if magic == MAGIC_Z64:
        return "z64"
    if magic == MAGIC_V64:
        return "v64"
    if magic == MAGIC_N64:
        return "n64"
    raise ValueError(
        f"Unrecognised ROM magic {magic.hex().upper()} — "
        "expected a .z64 / .v64 / .n64 file"
    )


def normalise_to_z64(data: bytes, fmt: str) -> bytes:
    """Return a big-endian (z64) copy of the ROM, converting if needed."""
    if fmt == "z64":
        return bytes(data)
    if fmt == "v64":
        # Swap every pair of bytes within each 16-bit word.
        arr = bytearray(data)
        for i in range(0, len(arr) - 1, 2):
            arr[i], arr[i + 1] = arr[i + 1], arr[i]
        return bytes(arr)
    if fmt == "n64":
        # Reverse byte order within each 32-bit word.
        arr = bytearray(data)
        for i in range(0, len(arr) - 3, 4):
            arr[i], arr[i + 1], arr[i + 2], arr[i + 3] = (
                arr[i + 3], arr[i + 2], arr[i + 1], arr[i]
            )
        return bytes(arr)
    raise ValueError(f"Unknown format '{fmt}'")


# ---------------------------------------------------------------------------
# Rule parsing
# ---------------------------------------------------------------------------

@dataclass
class PatchRule:
    search: bytes
    replace: bytes
    comment: str


def parse_rules(entries: list, encoding: str) -> list[PatchRule]:
    rules = []
    for idx, entry in enumerate(entries):
        kind = entry.get("type", "text").lower()
        comment = entry.get("comment", "")
        label = comment or repr(entry.get("search", f"rule[{idx}]"))
        try:
            if kind == "text":
                search = entry["search"].encode(encoding)
                replace = entry["replace"].encode(encoding)
            elif kind == "hex":
                search = bytes.fromhex(entry["search"])
                replace = bytes.fromhex(entry["replace"])
            else:
                raise ValueError(f"unknown type '{kind}' (must be 'text' or 'hex')")
        except (KeyError, ValueError) as exc:
            raise ValueError(f"Rule {idx} ({label}): {exc}") from exc

        if len(replace) > len(search):
            raise ValueError(
                f"Rule {idx} ({label}): replacement ({len(replace)} B) is longer "
                f"than search pattern ({len(search)} B)"
            )
        # Null-pad replacement to match search length so the ROM stays the same size.
        replace = replace.ljust(len(search), b"\x00")
        rules.append(PatchRule(search=search, replace=replace, comment=label))
    return rules


# ---------------------------------------------------------------------------
# ROM search & patch application
# ---------------------------------------------------------------------------

@dataclass
class Match:
    offset: int      # byte offset in ROM
    rule: PatchRule


def find_matches(rom: bytes, rules: list[PatchRule]) -> list[Match]:
    matches = []
    for rule in rules:
        start = 0
        while True:
            idx = rom.find(rule.search, start)
            if idx == -1:
                break
            matches.append(Match(offset=idx, rule=rule))
            start = idx + 1
    matches.sort(key=lambda m: m.offset)
    return matches


def apply_matches(rom: bytes, matches: list[Match]) -> bytearray:
    """Return patched ROM; raise if two matches write different bytes to the same offset."""
    patched = bytearray(rom)
    ownership: dict[int, str] = {}  # byte_offset → comment of the rule that owns it

    for m in matches:
        for i, byte_val in enumerate(m.rule.replace):
            abs_off = m.offset + i
            if rom[abs_off] == byte_val:
                continue  # no actual change; skip conflict check
            if abs_off in ownership and patched[abs_off] != byte_val:
                raise ValueError(
                    f"Conflict at ROM offset 0x{abs_off:08X}: "
                    f"'{ownership[abs_off]}' and '{m.rule.comment}' write different bytes"
                )
            ownership[abs_off] = m.rule.comment
            patched[abs_off] = byte_val

    return patched


# ---------------------------------------------------------------------------
# MCU rule generation
# ---------------------------------------------------------------------------

def matches_to_mcu_rules(rom_orig: bytes, patched: bytes, matches: list[Match]) -> list[dict]:
    """
    Produce one MCU rule_t entry per 4-byte aligned word that has ≥1 changed byte.

    resp32 is the big-endian 32-bit value the MCU should return for that word,
    assembled from the patched ROM so partial-word replacements carry unchanged bytes.
    """
    # Collect every byte offset that actually changed.
    changed_offsets: set[int] = set()
    for m in matches:
        for i, (orig, new) in enumerate(zip(m.rule.search, m.rule.replace)):
            if orig != new:
                changed_offsets.add(m.offset + i)

    # Group changed offsets into 4-byte aligned words.
    word_offsets = sorted({off & ~3 for off in changed_offsets})

    mcu_rules = []
    for word_off in word_offsets:
        resp_bytes = patched[word_off: word_off + 4]
        resp32 = struct.unpack(">I", resp_bytes)[0]  # big-endian

        # Best-effort comment: grab the label of the rule whose first match
        # falls nearest this word.
        comment = ""
        for m in matches:
            if m.offset <= word_off + 3 and m.offset + len(m.rule.search) > word_off:
                comment = m.rule.comment
                break

        mcu_rules.append({
            "mask":    0xFFFFFFFC,
            "match":   CART_BASE + word_off,
            "resp32":  resp32,
            "enable":  True,
            "comment": comment,
        })

    return mcu_rules


# ---------------------------------------------------------------------------
# C header emission
# ---------------------------------------------------------------------------

def emit_header(
    mcu_rules: list[dict],
    rom_path: Path,
    rules_path: Path,
) -> str:
    stem = rom_path.stem.upper().replace("-", "_").replace(" ", "_").replace(".", "_")
    guard = stem + "_PATCHES_H"

    lines = [
        f"/* Auto-generated by binary_patcher.py — do not edit by hand.",
        f" * ROM:   {rom_path.name}",
        f" * Rules: {rules_path.name}",
        f" * {len(mcu_rules)} rule(s) generated.",
        f" *",
        f" * To use: #include this header and wire patch_rules / NUM_PATCH_RULES",
        f" * into the n64_gamecat_switched rules table.",
        f" */",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#include <stdint.h>",
        "#include <stdbool.h>",
        "",
        "/* One rule per 4-byte aligned cart-bus word that needs patching.",
        " * mask=0xFFFFFFFC selects an exact 32-bit word address.",
        " * resp32 is the full 32-bit big-endian value the MCU serves on a match. */",
        "typedef struct { uint32_t mask, match, resp32; bool enable; } patch_rule_t;",
        "",
        "static const patch_rule_t patch_rules[] = {",
    ]

    for r in mcu_rules:
        comment_str = f"  /* {r['comment']} */" if r["comment"] else ""
        lines.append(
            f"    {{ 0x{r['mask']:08X}u, 0x{r['match']:08X}u, "
            f"0x{r['resp32']:08X}u, {'true' if r['enable'] else 'false'} }},{comment_str}"
        )

    lines += [
        "};",
        "static const int NUM_PATCH_RULES =",
        "    (int)(sizeof(patch_rules) / sizeof(patch_rules[0]));",
        "",
        f"#endif /* {guard} */",
        "",
    ]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(
        description="Patch an N64 ROM and generate an n64_gamecat MCU rule header.",
        epilog=(
            "Example:\n"
            "  python3 tools/binary_patcher.py game.z64 rules.json\n"
            "  python3 tools/binary_patcher.py game.z64 rules.json "
            "--output patched.z64 --header patches.h"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("rom",   help="N64 ROM file (.z64 / .v64 / .n64)")
    ap.add_argument("rules", help="JSON patch-rules file")
    ap.add_argument("-o", "--output",   default=None,
                    help="Patched ROM output path (default: <rom>_patched.z64)")
    ap.add_argument("--header",         default=None,
                    help="C header output path (default: <rom>_patches.h)")
    ap.add_argument("--encoding",       default="ascii",
                    help="Text encoding for 'type:text' rules (default: ascii)")
    ap.add_argument("--dry-run", action="store_true",
                    help="Show what would change without writing any files")
    args = ap.parse_args()

    rom_path   = Path(args.rom)
    rules_path = Path(args.rules)

    # --- Validate inputs -------------------------------------------------------
    for p in (rom_path, rules_path):
        if not p.exists():
            print(f"error: file not found: {p}", file=sys.stderr)
            return 1

    try:
        raw_rom = rom_path.read_bytes()
        fmt = detect_rom_format(raw_rom)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    if fmt != "z64":
        print(f"ROM format: {fmt} — converting to z64 (big-endian) for processing.")
    else:
        print(f"ROM format: z64 (big-endian, native)")

    rom_z64 = normalise_to_z64(raw_rom, fmt)
    print(f"ROM size:   {len(rom_z64):,} bytes  ({len(rom_z64) // 1024 // 1024} MiB)")

    try:
        rule_entries = json.loads(rules_path.read_text())
    except json.JSONDecodeError as exc:
        print(f"error: invalid JSON in rules file: {exc}", file=sys.stderr)
        return 1

    if not isinstance(rule_entries, list):
        print("error: rules JSON must be a top-level array", file=sys.stderr)
        return 1

    try:
        patch_rules = parse_rules(rule_entries, args.encoding)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    # --- Default output paths --------------------------------------------------
    out_rom = Path(args.output) if args.output else rom_path.with_name(
        rom_path.stem + "_patched.z64"
    )
    out_header = Path(args.header) if args.header else rom_path.with_name(
        rom_path.stem + "_patches.h"
    )

    # --- Search ----------------------------------------------------------------
    print()
    matches = find_matches(rom_z64, patch_rules)

    # Report results per rule.
    for rule in patch_rules:
        rule_matches = [m for m in matches if m.rule is rule]
        if not rule_matches:
            print(f"  WARNING: pattern not found — {rule.comment}", file=sys.stderr)
        else:
            for m in rule_matches:
                cart_addr = CART_BASE + m.offset
                print(
                    f"  FOUND  {rule.comment!r:36s}"
                    f"  ROM 0x{m.offset:08X}  cart 0x{cart_addr:08X}"
                )

    if not matches:
        print("No patterns found; nothing to patch.")
        return 0

    # --- Apply patches ---------------------------------------------------------
    try:
        patched = apply_matches(rom_z64, matches)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    changed_words = len({(m.offset & ~3) for m in matches})
    print(f"\n  {len(matches)} match(es) → {changed_words} 32-bit word(s) changed")

    # --- Generate MCU rules ----------------------------------------------------
    mcu_rules = matches_to_mcu_rules(rom_z64, patched, matches)
    print(f"\nMCU rules ({len(mcu_rules)}):")
    for r in mcu_rules:
        print(
            f"  mask=0x{r['mask']:08X}  match=0x{r['match']:08X}  "
            f"resp=0x{r['resp32']:08X}  /* {r['comment']} */"
        )

    header_text = emit_header(mcu_rules, rom_path, rules_path)

    if args.dry_run:
        print("\n--- C header (dry run) ---")
        print(header_text)
        return 0

    # --- Write outputs ---------------------------------------------------------
    out_rom.write_bytes(patched)
    print(f"\nPatched ROM  → {out_rom}")

    out_header.write_text(header_text)
    print(f"C header     → {out_header}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
