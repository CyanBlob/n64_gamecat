# N64 ROM Hacking Notes

A reference for what we figured out together while bringing up the Gamecat
interceptor. Organized by topic so you can jump in.

---

## 1. Tools

### Emulators (Mac-friendly)

| Tool | Strength | When to use |
| :--- | :--- | :--- |
| **Ares** | Cycle-accurate; built-in memory editor, breakpoints, disassembler, tracer | Default for everything on Mac. Both debugging *and* hardware-accuracy validation. |
| **simple64** | Accuracy, no debugger | Sanity-check that a patched ROM plays correctly. |
| **Project64 + debugger** | Best ROM-hacking UX, but Windows-only | Run via Whisky (Wine wrapper) on Mac if a tutorial demands its specific UI. |
| **Whisky** | Wine wrapper for Mac with Apple Game Porting Toolkit | Lighter than a VM for old Windows tools like PJ64. |
| **UTM / Parallels** | Full Windows VM | Last resort. UTM is free; Parallels is paid but more polished. |

**Rule of thumb:** start with Ares. It does both jobs natively.

### Static analysis

- **Ghidra** — disassembler/decompiler. Use language `MIPS:BE:64:default` (big-endian, 64-bit, original MIPS — *not* MIPS R6).
- **N64 Ghidra Loader** (e.g. `zeroKilo/N64LoaderWV`) — auto-maps ROM segments and the cart header. Versions of the loader must match your Ghidra version exactly. If no prebuilt match exists, install JDK 17+ and `gradle` and build from source: `export GHIDRA_INSTALL_DIR=/Applications/ghidra_X.Y_PUBLIC; gradle`.
- **OoT decomp (`zeldaret/oot`)** — for OoT specifically, this is more powerful than any disassembler. Working C source built byte-identical to the ROM, with full symbol names.

### CLI / scripting

- **`xxd`** — hex dump and pattern search.
- **`mips-linux-gnu-objdump`** — disassembler. For OoT, the bundled binary is at `tools/egcs/macos/objdump`.
- **`strings -tx <file>`** — print all readable ASCII strings with file offset.
- **Python** — for ROM file patching, see snippet in §6.

---

## 2. ROM Identification

The cart header (first 0x40 bytes) tells you everything:

| Offset | Bytes | Meaning |
| :----- | :---- | :------ |
| `0x00` | 4 | Endianness indicator (`80 37 12 40` for canonical `.z64` big-endian) |
| `0x10` | 4 | CRC1 |
| `0x14` | 4 | CRC2 |
| `0x20` | 20 | Internal name (e.g. `"THE LEGEND OF ZELDA "`) |
| `0x3B` | 1 | Media type (`N`=cart, `C`=expandable cart, `D`=64DD) |
| `0x3C` | 2 | Game ID (e.g. `"ZL"` for Zelda) |
| `0x3E` | 1 | **Country code** (`E`=US, `J`=Japan, `P`=PAL, etc.) |
| `0x3F` | 1 | **Version** (`0x00`=v1.0, `0x01`=v1.1, `0x02`=v1.2) |

**Quick identification commands:**
```bash
xxd -s 0x20 -l 20 rom.z64        # internal name
xxd -s 0x3B -l 5 rom.z64          # media + game ID + country + version
xxd -s 0x10 -l 8 rom.z64          # CRC1 + CRC2
md5 rom.z64                        # for hash comparison against known dumps
```

**Endianness gotcha:** OoT and most N64 ROMs come in three formats:
- `.z64` — big-endian, "correct"; first 4 bytes = `80 37 12 40`
- `.v64` — byte-swapped (Doctor V64 backup format)
- `.n64` — word-swapped

Tools like the OoT decomp expect `.z64`. Convert with `tool64`, `n64chksum`, or `ucon64`.

---

## 3. The N64 Cart Bus & Boot Process

### Two distinct security checks

1. **CIC ↔ PIF challenge-response** — hardware-level, between cart's CIC chip and console's PIF. Continuous. Cannot be bypassed from cart-bus side. As long as the inserted cart's CIC works, this is handled.

2. **Bootcode CRC** — software check. The bootcode CRCs the first 1MB of ROM data (`0x10001000` to `0x10101000` on the cart bus, exclusive) and compares to `CRC1`/`CRC2` in the header. This is the one we work around.

### CIC types (relevant for bypass)

| CIC | Examples | Bypass NOP offsets |
| :-- | :------- | :----------------- |
| 6101 | Star Fox 64 | 0x670, 0x67C |
| 6102/7101 | SM64, Mario Kart 64, most NTSC | 0x66C, 0x678 |
| 6103/7103 | Paper Mario | 0x63C, 0x648 |
| **6105/7105** | **Zelda OoT, Majora's Mask** | **0x77C, 0x788** |
| 6106/7106 | Yoshi's Story, F-Zero X | (not in en64 wiki) |

### CRC bypass

NOP the two `BNE` instructions in the bootcode at the offsets above. Both live in the bootcode region (0x40–0x1000), which is *outside* the CRC window — so NOPing them doesn't itself change the CRC. Once NOP'd, you can patch anywhere in cart ROM.

In the Gamecat firmware, this is `N64_CIC_TYPE` macro. `sim/check_rules.py` validates rules vs the CRC window.

### Compression — the big constraint

OoT (and most N64 games) compress most cart segments with **Yaz0**:
- Cart bus reads = compressed bytes
- Boot stub decompresses into RAM
- Game runs from RAM

**An interceptor sitting on the cart bus cannot meaningfully patch values inside compressed segments** — there's no byte-level mapping between compressed bytes and the decompressed values they produce. Patching one compressed byte corrupts the entire stream from that point.

Find which segments are compressed:
```bash
# In the OoT decomp directory:
awk '/^beginseg/,/^endseg/' spec/spec | \
    awk '/^beginseg/{seg=""; comp="no"} /name/{seg=$2} /compress/{comp="yes"} /^endseg/{print comp, seg}' | \
    sort | uniq
```

For OoT specifically:
- ✓ **Patchable:** `boot`, `dmadata`, `makerom`, all `*_message_data_static` (dialog!), `Audiobank`/`Audioseq`/`Audiotable`, all `vr_*_pal_static` (skybox palettes), map textures
- ✗ **Off-limits:** `code` (game logic, save defaults), all `ovl_*` (actor/scene code), `nintendo_rogo_static`, `title_static`, all `anime_*` (Link's animations), all `vr_*_static` without `_pal` (skybox images)

---

## 4. Finding Patches: Investigation Workflows

### Workflow A — Decomp source grep (best for OoT)

```bash
# Find the C code for the value you want to change
cd oot
grep -rn "healthCapacity\|playerData" src/code/ | head
```

If the value is in `static` data, it'll be in something like `z_sram.c` as an initializer:
```c
static SavePlayerData sNewSavePlayerData = {
    ...
    0x30,    // healthCapacity   <- this is your target
    ...
};
```

### Workflow B — Memory write breakpoint (for runtime values)

In Ares (or PJ64 + debugger):
1. Start the game, get the value to its initial state.
2. In the memory editor, find the RAM address holding the value.
3. Set a memory **write** breakpoint on that address.
4. Trigger the action that changes the value (start a new save, take damage, etc).
5. The debugger pauses at the instruction that wrote it. Note the PC.
6. Convert the PC to a ROM offset (see §5).

### Workflow C — Pattern search

If you know the bytes, search for them directly:
```bash
SIG="dfdfdfdfdfdfdfdf0000003000300030"   # 16+ bytes is usually unique
xxd -p rom.z64 | tr -d '\n' | grep -boE "$SIG" | head
# Result: <hex_offset>:<matched_bytes>
# File offset = hex_offset / 2 (since each byte is 2 hex chars)
```

Useful when you've found bytes via Workflow A but want the offset in a *different* ROM (e.g. building against JP, but patching US).

### Workflow D — Object file inspection

For static C variables that don't appear in the linker map (`static` C symbols often don't):
```bash
# Find which .o file has the data
grep "z_sram" build/ntsc-1.2/oot-ntsc-1.2.map | head
# Note the .data section's RAM address

# Dump the actual bytes
tools/egcs/macos/objdump -s -j .data build/ntsc-1.2/src/code/z_sram.o
# Find your target byte sequence in the output
```

---

## 5. Translating Symbols to ROM Offsets

### From a RAM address to a cart-bus address

Cart ROM is mirrored at `0x10000000` on the N64 bus. To convert:

```
cart_bus_addr = 0x10000000 + (file_offset_in_rom)
```

To get the ROM file offset of a RAM address:
1. Find the **segment** containing that RAM address (look in `oot-ntsc-X.X.map` or `dmadata`).
2. Each segment's RAM-to-ROM mapping is in the map under `LOAD` lines or in the `dmadata` table.
3. `rom_offset = segment_rom_start + (ram_addr - segment_ram_start)`

### From a `static` C variable to its ROM offset

`static` symbols are usually stripped from ELF symbol tables. Workflow:
1. Find the object file's `.data` section RAM address from the map (`grep "<file>.o" *.map`).
2. Dump the section: `objdump -s -j .data <file>.o`.
3. Find your bytes by inspection. The byte position within `.data` is your offset within the section.
4. RAM addr = section RAM base + position.
5. Convert to cart-bus address as above.

### Quickest method: skip the math, pattern-match

Once you have the bytes from `objdump`, just `grep` for them in the actual ROM file. The match offset (divided by 2 for `xxd -p` output) is your ROM file offset. This works even when the source decomp targets a different ROM version than you have, because data structs are usually region-independent.

---

## 6. ROM Patching Mechanics

### Apply patches to a ROM file (for emulator testing)

```python
# tools/patch_rom.py
import shutil, struct, sys

src, dst = sys.argv[1], sys.argv[2]
shutil.copy(src, dst)

# (cart_bus_addr, 32-bit value)  — addr must be word-aligned
patches = [
    (0x10ABC123 & ~0x3, 0x00000140),
]

with open(dst, "r+b") as f:
    for cart_addr, value in patches:
        rom_offset = cart_addr - 0x10000000
        f.seek(rom_offset)
        f.write(struct.pack(">I", value))   # big-endian (Z64)
```

Run: `python3 tools/patch_rom.py orig.z64 patched.z64`. Load `patched.z64` in Ares to verify before flashing your interceptor.

### Convert to a Gamecat interceptor rule

```c
{ 0xFFFFFFFCu, <cart_bus_addr & ~0x3>, <new_32bit_value>, true },
```

Word-aligned (mask `~0x3`). Always read the original 32-bit word at the aligned address first, so you know which 16-bit half to modify and which to preserve:

```bash
xxd -s <aligned_offset> -l 4 rom.z64
```

---

## 7. MIPS Reading Tips

### Common N64 conventions

- Big-endian. Always.
- 32-bit MIPS code mostly, with occasional 64-bit (`ld`, `sd`, `daddiu`) — that's why Ghidra needs the 64-bit language variant.
- IDO-compiled (SGI's compiler), not GCC. Expect odd patterns: heavy delay-slot use, weird register allocation, `lui`/`addiu` pair construction for full 32-bit immediates.

### Distinguishing data values from stack offsets

When searching disassembly for "0x30" (or any small constant), most hits will be **stack manipulations**, not values:

```mips
addiu sp, sp, -0x30           ; allocating 48 bytes of stack
addiu sp, sp,  0x30           ; deallocating it
sw  $reg, Stack[0x30](sp)     ; spilling to stack offset 0x30
lw  $reg, Stack[0x30](sp)     ; reloading
sw  $reg, 0x30(t3)            ; writing to *struct offset* 0x30
```

The actual value-loading patterns:
```mips
li   $t0, 0x30                ; load immediate (a real value!)
addiu $t0, $zero, 0x30        ; same thing (li expands to this)
ori  $t0, $t0, 0x30           ; sometimes used for low halves of larger values
```

Filter out stack noise to find the real constants.

### MIPS NOP

`0x00000000` is the canonical NOP (`sll $zero, $zero, 0`). Useful for patching out a branch you want to neutralize.

### Patching a load-immediate

If `addiu $t0, $zero, 0x0030` encodes as `0x24080030`, change the immediate:
- `addiu $t0, $zero, 0x0140` = `0x24080140`

Lower 16 bits of the instruction = the immediate.

---

## 8. Game Genie vs GameShark — the Conceptual Divide

### Game Genie (NES, SNES, Genesis)

Sat between cart and console. Watched the address bus. On each ROM read at a programmed address, substituted a different data value.

**Codes encode:** address + value + (optional) compare. Substitution cipher of letters to nibbles, then shuffled into fields.

**Your interceptor is electrically a Game Genie.** For platforms with existing Game Genie code libraries, you can decode codes directly into your `rules[]` table. **N64 never had a Game Genie**, so no preexisting library exists.

### GameShark (PS1, GBA, N64, etc.)

Plugged between cart and console, but works completely differently:
- Has its own onboard ROM with its own bootcode.
- Hijacks the boot vector — runs *its* code first.
- Installs an NMI handler that fires periodically.
- On each NMI, walks the user's enabled cheat list and writes new values into RAM.

**GameShark patches RAM, not ROM.** Your interceptor cannot directly use N64 GameShark codes because RAM doesn't go through the cart bus.

**N64 GameShark code formats:**
```
80XXXXXX 00YY   write byte 0xYY to RAM 0x8000_XXXX (every frame)
81XXXXXX YYYY   write halfword 0xYYYY to RAM 0x8000_XXXX (every frame, aligned)
D0XXXXXX 00YY   conditional: execute next code only if RAM[0x8000_XXXX] == 0xYY
F0XXXXXX 00YY   write once at startup
```

A GS code can be a *hint* to your ROM hacking — it tells you what address holds a value. Then use Workflow B/C from §4 to find the *instruction* that writes to that address, and patch the instruction in ROM.

---

## 9. Common Pitfalls

| Pitfall | Why | Fix |
| :------ | :-- | :-- |
| ROM CRC mismatch with decomp | You have the wrong region/version (e.g. US ROM but built `ntsc-1.2` which is JP) | Match `baseroms/<name>/` to your country code byte |
| `make setup` fails on Mac with `lld` errors | macOS lld doesn't support Mach-O | Patch `tools/fado/Makefile` to skip lld on Darwin |
| Asset extraction fails with type-union error | Python 3.9 doesn't support `str \| tuple` syntax | `brew install python@3.12`, recreate `.venv` |
| ROM build fails with "ld: unknown options: -T" | Passed `LD=ld` poisons the MIPS cross-link too | Patch the host-tool Makefile instead; don't pass `LD=ld` to the ROM build |
| `static` symbols don't appear in linker map | IDO strips static C symbols | Use `objdump -t <file>.o` and look at the `.data` section bytes directly |
| Pattern not found in compressed ROM | The data is in a Yaz0 segment | See §3 (compression). Targets in compressed segments need a flashcart, not the interceptor |
| Search for `0x30` returns hundreds of hits | Most are stack offsets, not values | See §7; filter `addiu sp` and `sw/lw ... Stack[...]` |
| Ghidra disassembly has gaps / "invalid instruction" | Picked 32-bit MIPS variant | Switch to `MIPS:BE:64:default` |

---

## 10. OoT-Specific Knowledge

### Versions

- `ntsc-1.0/1.1/1.2` in the decomp = **Japanese** NTSC (despite the name)
- `pal-1.0/1.1` = European PAL
- `gc-*` = GameCube ports (multiple variants including Master Quest debug)
- `ique-cn` = Chinese iQue version
- US N64 versions (NTSC US 1.0/1.1/1.2) often **not directly supported** by older decomp commits — translate by pattern-matching from JP versions

### Save data layout (`SavePlayerData` struct, from z_sram.c)

In `sNewSavePlayerData` (the new-game template):
- offset 0x00: `newf[6]` — magic bytes for valid save (all zero in template)
- offset 0x06: `deaths` (s16)
- offset 0x08: `playerName[8]` — eight `FILENAME_SPACE` bytes (`0xDF`)
- offset 0x10: `n64ddFlag` (s16)
- offset 0x12: `healthCapacity` (s16) — **0x30 in template (3 hearts)**
- offset 0x14: `defense` (s16)
- offset 0x16: `magicLevel` (s8)
- offset 0x17: `magic` (s8)
- ... etc

`health` (current) is **not** in the static template — it's set at runtime to equal `healthCapacity`. So patching only `healthCapacity` is enough.

`MAGIC_NORMAL_METER` = 0x30 (just to confuse pattern matching).

The second `0xE0` healthCapacity in the source is for `sDebugSavePlayerData` (14-heart debug save).

### CIC

OoT NTSC and PAL use CIC **6105** / **7105** (Zelda type). Set `N64_CIC_TYPE=6105` in firmware for either US or JP NTSC.

### Strategic notes

- `code` is compressed → **save data, hearts, items, anything in game logic is unreachable to a cart-bus interceptor**.
- `nes_message_data_static` (English dialog) is **uncompressed** → text changes work great with the interceptor.
- For "max starting hearts on a real cart": modify `z_sram.c` line 104, `make`, flash to an EverDrive. The interceptor is the wrong tool for this specific job.

---

## 11. The OoT Decomp Build (macOS)

### One-time setup

```bash
brew install python@3.12 gradle openjdk@21    # pip is not enough; need Python 3.10+

cd oot
rm -rf .venv
python3.12 -m venv .venv
```

Patch `tools/fado/Makefile` around line 27:

```make
ifneq ($(LLD),0)
  LDFLAGS   += -fuse-ld=lld
else ifneq ($(shell uname -s),Darwin)
ifneq ($(LD),ld)
  LDFLAGS   += -fuse-ld=lld
endif
endif
```

Also patch `tools/Makefile` around line 27 with the same `Darwin` check.

### Build

```bash
make setup VERSION=ntsc-1.2 LLD=0
make VERSION=ntsc-1.2 LLD=0
```

Output: `build/ntsc-1.2/oot-ntsc-1.2.elf` and `oot-ntsc-1.2.z64`, plus `oot-ntsc-1.2.map` (the symbol map).

---

## 12. Quick Command Reference

### Identify a ROM
```bash
xxd -s 0x20 -l 20 rom.z64                          # internal name
xxd -s 0x3B -l 5 rom.z64                            # game ID + country + version
md5 rom.z64
```

### Find a byte signature in a ROM
```bash
SIG="dfdfdfdfdfdfdfdf0000003000300030"
xxd -p rom.z64 | tr -d '\n' | grep -boE "$SIG"
# Convert hex offset to file offset: divide by 2
```

### Inspect a 4-byte word at a specific offset
```bash
xxd -s 0xABCDEF -l 4 rom.z64
```

### Dump a function's disassembly
```bash
mips-linux-gnu-objdump -d -j .text --disassemble=Sram_InitNewSave build/.../code.elf
```

### Dump a section's raw bytes
```bash
mips-linux-gnu-objdump -s -j .data build/.../z_sram.o
```

### List all sections of a binary
```bash
mips-linux-gnu-objdump -h build/.../oot.elf
```

### Find a symbol's address
```bash
mips-linux-gnu-nm -n build/.../oot.elf | grep Sram_InitNewSave
# Or in the map:
grep "Sram_InitNewSave" build/.../oot-ntsc-1.2.map
```

### Find readable strings in a ROM
```bash
strings -tx rom.z64 | grep -i "danger\|hello"        # offsets in hex
```

### Verify the Gamecat interceptor's rules
```bash
python3 sim/check_rules.py n64_gamecat_switched.cpp --cic 6105
```

### Run the Gamecat simulator
```bash
python3 sim/simulator.py
gtkwave sim/gamecat.vcd
```

---

## 13. The Big Picture for Your Interceptor

After all this work, here's where the Gamecat interceptor genuinely shines on N64:

✓ **Boot-time tricks** — PI bus speed override, CIC bypass NOPs, region/header changes
✓ **Patches in uncompressed segments** — text/dialog (`nes_message_data_static`), audio (`Audiobank`), skybox palettes (`vr_*_pal_static`), map textures
✓ **Things only a cart-bus device can do** — intercept the very first reads, run before the boot ROM has set up anything

✗ **Won't work for:** anything in compressed segments — game logic, save defaults, item data, actor code, compressed assets

For game-logic patches on a real cart, a flashcart with a modified ROM build is the right tool. The interceptor is a Game Genie for an era where Game Genies became mostly impractical due to ROM compression — but it still has its niche, and it's a niche no other consumer device occupies.

---

## 14. Resources

- **en64 wiki** — N64 hardware/cart docs: https://en64.shoutwiki.com/wiki/ROM
- **OoT decomp** — https://github.com/zeldaret/oot
- **Ares** — https://ares-emu.net
- **Ghidra** — https://ghidra-sre.org
- **N64 Ghidra Loader** — https://github.com/zeroKilo/N64LoaderWV (search GitHub for newer/better forks)
- **Whisky** (Wine for Mac) — `brew install --cask whisky`
- **Pico SDK docs** — https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf

---

*Compiled from a long evening's investigation. Future-you: hi, you're welcome.*
