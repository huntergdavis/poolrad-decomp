# ECL interpreter and script contracts (DOS v1.3)

**Status:** 2026-10-10, lane A. Every claim is tagged **verified** (re-derived from the
supplied build with the tools in `tools/overlay/`) or **open**. Offsets are file offsets into
`GAME.OVR` (sha256 `872964a3…928614`) unless marked as unit-relative. No script bytes or text
are reproduced here. Reference material: Gold Box Explorer (bsimser, rev `eac30aba`, MIT),
whose ECL interpreter table and operand loader were checked against this build; the
DOS Decode Kit (commit `d9f7fb45`), whose claims were audited in [../FABLE_AUDIT.md](../FABLE_AUDIT.md).

## 1. Script container — verified

- `ECL1.DAX` … `ECL8.DAX` hold 29 blocks in total; block ids equal the GEO map ids of the area
  each script serves (the Mac snapshot's name table applies: 18 = Podol Plaza, 20 = Slums, ...).
- A block is `u16 prefix` (the same value in all 29 blocks) followed by five entry instructions
  at unpacked offsets 2, 6, 10, 14, 18. Each is opcode `0x01` (GOTO) with one word operand of
  tag `0x01`. The five entries are, in order: run, search-location, pre-camp check, camp
  interrupted, initial entry (names from the reference interpreter).
- **Virtual addresses:** a script word address `a` maps to unpacked offset `a - 0x9900 + 2`.
  Checked on all 145 entry jumps: 145 in range and on a valid opcode. (The kit's `a - 0x98F1`
  gives 102 of 145.)
- Blocks reference three wallsets with opcode `0x37` (29 occurrences; map 18 loads sets 1, 3,
  5, which are the block ids present in `WALLDEF1.DAX`).

## 2. Interpreter location — verified

| item | value |
| --- | --- |
| overlay unit | stub paragraph `0x1c`, file offset 3398, 13,581 code bytes, 402 fixups, 2 public entries |
| run function | 0x03EB7 (unit +0x3171): stores its parameter to the PC global, clears the stop byte, loops at 0x03EC5 |
| fetch | PC `[0x49ED]`, buffer far pointer `[0x49DE]`, fetch bias 0x6700, opcode latch `[0x6F7F]`, previous opcode `[0x6F7E]` |
| stop bytes | `[0x442E]`, `[0x49FF]` (loop exits when either is non-zero) |
| dispatcher | 0x03C73 (unit +0x2F2D): compare chain on `[0x6F7F]` for 0x00 to 0x3D, 0x1F has no case; 61 opcodes, 49 distinct handlers |
| call stack | linked list head `[0x6F70]` (far pointer), 6-byte nodes freed by the EXIT handler |
| compare flags | `[0x6F78]` … `[0x6F7D]` (set by COMPARE / COMPARE AND, tested by the IF family) |

Call-graph facts (from `callgraph.py`, fixup-confirmed far-call sites, i.e. sites whose segment word carries a linker relocation): unit `0x1c` has 99 such sites targeting unit `0x25`,
whose entries 20 to 23 are the operand-fetch helpers (the kit's "resident 0x2B helpers");
the 0x20 handler calls unit `0x7f` entry 1, the overlay/script loader.

## 3. Opcode table — handler addresses verified, names from the reference

Operands are the fixed count the reference interpreter reads (`+n` = a trailing list whose
length is the last fixed operand). Handlers shared by several opcodes branch on the latched
opcode internally.

| opcode | reference name | operands | GAME.OVR offset | unit 0x1c offset |
| --- | --- | --- | --- | --- |
| 0x00 | EXIT | 0 | 0x00D47 | 0x0001 |
| 0x01 | GOTO | 1 | 0x00E3A | 0x00F4 |
| 0x02 | GOSUB | 1 | 0x00E57 | 0x0111 |
| 0x03 | COMPARE | 2 | 0x00E6C | 0x0126 |
| 0x04 | ADD | 3 | 0x00EE9 | 0x01A3 |
| 0x05 | SUBTRACT | 3 | 0x00EE9 | 0x01A3 |
| 0x06 | DIVIDE | 3 | 0x00EE9 | 0x01A3 |
| 0x07 | MULTIPLY | 3 | 0x00EE9 | 0x01A3 |
| 0x08 | RANDOM | 2 | 0x00F7A | 0x0234 |
| 0x09 | SAVE | 2 | 0x00FD8 | 0x0292 |
| 0x0A | LOAD CHARACTER | 1 | 0x01026 | 0x02E0 |
| 0x0B | LOAD MONSTER | 3 | 0x01196 | 0x0450 |
| 0x0C | SETUP MONSTER | 3 | 0x010FA | 0x03B4 |
| 0x0D | APPROACH | 0 | 0x0155D | 0x0817 |
| 0x0E | PICTURE | 1 | 0x0159C | 0x0856 |
| 0x0F | INPUT NUMBER | 2 | 0x01697 | 0x0951 |
| 0x10 | INPUT STRING | 2 | 0x016DA | 0x0994 |
| 0x11 | PRINT | 1 | 0x0174F | 0x0A09 |
| 0x12 | PRINTCLEAR | 1 | 0x0174F | 0x0A09 |
| 0x13 | RETURN | 0 | 0x017E5 | 0x0A9F |
| 0x14 | COMPARE AND | 4 | 0x01833 | 0x0AED |
| 0x15 | VERTICAL MENU | 3+n | 0x01C4E | 0x0F08 |
| 0x16 | IF = | 0 | 0x01898 | 0x0B52 |
| 0x17 | IF <> | 0 | 0x01898 | 0x0B52 |
| 0x18 | IF < | 0 | 0x01898 | 0x0B52 |
| 0x19 | IF > | 0 | 0x01898 | 0x0B52 |
| 0x1A | IF <= | 0 | 0x01898 | 0x0B52 |
| 0x1B | IF >= | 0 | 0x01898 | 0x0B52 |
| 0x1C | CLEARMONSTERS | 0 | 0x01FA3 | 0x125D |
| 0x1D | PARTYSTRENGTH | 1 | 0x0200C | 0x12C6 |
| 0x1E | CHECKPARTY | 6 | 0x0217B | 0x1435 |
| 0x1F | (no dispatch case) | 2 | - | - |
| 0x20 | NEWECL | 1 | 0x01917 | 0x0BD1 |
| 0x21 | LOAD FILES | 3 | 0x019BA | 0x0C74 |
| 0x22 | PARTY SURPRISE | 2 | 0x0236F | 0x1629 |
| 0x23 | SURPRISE | 4 | 0x0240F | 0x16C9 |
| 0x24 | COMBAT | 0 | 0x024E1 | 0x179B |
| 0x25 | ON GOTO | 2+n | 0x0263E | 0x18F8 |
| 0x26 | ON GOSUB | 2+n | 0x0263E | 0x18F8 |
| 0x27 | TREASURE | 8 | 0x026F6 | 0x19B0 |
| 0x28 | ROB | 3 | 0x02AF7 | 0x1DB1 |
| 0x29 | ENCOUNTER MENU | 14 | 0x02C3E | 0x1EF8 |
| 0x2A | GETTABLE | 3 | 0x01BA4 | 0x0E5E |
| 0x2B | HORIZONTAL MENU | 2+n | 0x01E17 | 0x10D1 |
| 0x2C | PARLAY | 6 | 0x03311 | 0x25CB |
| 0x2D | CALL | 1 | 0x03A15 | 0x2CCF |
| 0x2E | DAMAGE | 5 | 0x034BB | 0x2775 |
| 0x2F | AND | 3 | 0x01B35 | 0x0DEF |
| 0x30 | OR | 3 | 0x01B35 | 0x0DEF |
| 0x31 | SPRITE OFF | 0 | 0x037E8 | 0x2AA2 |
| 0x32 | FIND ITEM | 1 | 0x033B0 | 0x266A |
| 0x33 | PRINT RETURN | 0 | 0x03831 | 0x2AEB |
| 0x34 | ECL CLOCK | 2 | 0x03807 | 0x2AC1 |
| 0x35 | SAVE TABLE | 3 | 0x01C02 | 0x0EBC |
| 0x36 | ADD NPC | 2 | 0x038C4 | 0x2B7E |
| 0x37 | LOAD PIECES | 3 | 0x019BA | 0x0C74 |
| 0x38 | PROGRAM | 1 | 0x03B56 | 0x2E10 |
| 0x39 | WHO | 1 | 0x03879 | 0x2B33 |
| 0x3A | DELAY | 0 | 0x03465 | 0x271F |
| 0x3B | SPELL | 3 | 0x03949 | 0x2C03 |
| 0x3C | PROTECTION | 1 | 0x03BC3 | 0x2E7D |
| 0x3D | CLEAR BOX | 0 | 0x0385A | 0x2B14 |

Note that 0x21 and 0x37 share one handler, as do 0x04 to 0x07, 0x11/0x12, 0x16 to 0x1B,
0x25/0x26, and 0x2F/0x30.

## 4. Operand grammar — verified on this build

Each operand is a tag byte followed by a value:

| tag | length | meaning (reference interpreter) |
| --- | --- | --- |
| `0x00` | 2 | immediate byte |
| `0x01` | 3 | word: script memory address |
| `0x02` | 3 | word: immediate |
| `0x03` | 3 | word: indirect address |
| `0x80` | 2 + L | inline packed string: length byte L, then L bytes of 6-bit text (4 chars per 3 bytes, MSB first; code c ≤ 0x1F maps to ASCII c + 0x40, 0x20 to 0x3F pass through) |
| `0x81` | 3 | string by memory address |

Zero-operand opcodes are one byte. Menus and ON GOTO / ON GOSUB take their fixed operands and
then a list of `n` further operands where `n` is the value of the last fixed operand (strings
for menus, word targets for the jump tables).

Evidence (`tools/overlay/ecl_walk.py`):

- A control-flow walk from the five entries over all 29 blocks decodes 3,726 instructions
  (53 distinct opcodes) with zero instruction-boundary overlaps.
- A linear sweep (`ecl_walk.py --sweep`) from offset 22 decodes 107,060 of 175,627 body
  bytes (61.0%) before the first error; in 26 of 29 blocks the first error directly follows an
  unconditional transfer (EXIT, GOTO, RETURN, GOSUB, NEWECL), which is where inline data begins.
  Five blocks decode essentially to their end (ECL1#18, ECL2#20, ECL4#2, ECL6#1, ECL6#19 at 99
  to 100%). The per-block table is the tool's output.
- The three remaining breaks follow opcodes 0x03, 0x04, and 0x34; their operand counts are
  **open**.
- 47 jump targets fall outside their block (virtual addresses below 0x9900 or beyond the
  block): **open**, probably references into engine-resident script memory.

## 5. Handler semantics — verified from the decompiled or disassembled handlers

- **0x00 EXIT:** sets `[0x442E]` = 1, increments the PC, frees every 6-byte node of the call
  stack list, resets the active-window state bytes `[0x5E24]/[0x5E25]`.
- **0x01 GOTO:** writes the resolved word operand to `[0x49ED]`.
- **0x20 NEWECL:** builds the `ECL<n>.DAX` file name from its byte operand, far-calls unit
  `0x7f` entry 1 (the loader) and unit `0x25` entries 20 to 23 (operand helpers), then sets
  `[0x442E]` = 1 and `[0x442F]` = 1 and clears `[0x4431]`.
- **0x21 LOAD FILES:** the fifth entry of map 18 jumps directly to a `0x21` whose first byte
  operand is 18, the map's own id; the next instruction is `0x37` with the three wallset ids.
  The kit's reading of 0x21 as "set area encounter" is contradicted.
- **0x37 LOAD PIECES:** three byte operands = wallset ids; verified against the WALLDEF block
  ids of map 18.
- **IF family 0x16–0x1B:** one handler tests one of the six compare-flag bytes and, if the
  test fails, skips the next instruction through unit `0x25` entry 41 (segment `0x2B`
  offset `0x8E`; six fixup-confirmed call sites, all in unit `0x1c`).

## 6. Open items for the next step

1. Names for 0x0A to 0x0E, 0x0F to 0x12, 0x22 to 0x24, 0x27: the kit and the reference
   disagree; resolve by reading each handler (all addresses above) and by oracle traces.
2. Operand counts for 0x03, 0x04, 0x34; the menu list termination; the out-of-block targets.
3. The exact loader (unit `0x7f` entry 1) contract: file name, block selection by map id,
   where the block is placed relative to the 0x6700 fetch bias and the 0x9900 address base.
4. 170 fixup-confirmed far-call sites (4.2% of 4,009) whose relocated `seg:off` lands inside a stub
   range but not on a stub, and 28 candidate 9A sites without a relocation (probably not
   calls): the linear resolution model does not cover them. All call-site counts in this
   document come from byte scans for E8/9A without instruction-boundary validation; only
   fixup-confirmed far sites are proven operands.

## 7. Byte layout of one ECL record (contract for the lane B container reader)

All values little-endian. "Unpacked" means after DAX RLE expansion of the block; the block id
in the DAX directory is the GEO map id the script serves.

| unpacked offset | size | field | verified value on this build |
| --- | --- | --- | --- |
| 0 | u16 | prefix word | the same constant in all 29 blocks (0x1388); meaning unknown, the reference interpreter discards it |
| 2 | 4 | entry 0 "run": `01 01 lo hi` = GOTO, word tag, target | opcode 0x01 and tag 0x01 in 29/29 blocks |
| 6 | 4 | entry 1 "search location" | same shape |
| 10 | 4 | entry 2 "pre-camp check" | same shape |
| 14 | 4 | entry 3 "camp interrupted" | same shape |
| 18 | 4 | entry 4 "initial entry" | same shape |
| 22 | to end | instruction stream with inline data | 61% decodes linearly before the first inline-data region |

Address rule: a word target `t` denotes unpacked offset `t - 0x9900 + 2`; equivalently, the
byte at unpacked offset 2 has virtual address 0x9900. A reader that strips the prefix word can
use `t - 0x9900` directly. 145/145 entry targets satisfy `0 <= offset < block length` and
land on a dispatchable opcode.

Instruction: `opcode u8` followed by operands as in section 4. Inline string operand:
`80 LL` then `LL` bytes; decode 4 six-bit codes per 3 bytes, MSB first, stop at code 0,
code c <= 0x1F -> ASCII c + 0x40, else ASCII c. A reader must not assume strings are
zero-terminated; the length byte is authoritative (the trailing zero code is padding).

Sizes on this build: 29 blocks, unpacked 2,171 to 7,679 bytes each (176,265 total), 98
inline strings in the largest city block. Counts come from `tools/overlay/ecl_walk.py` and
the audit; no block exceeds 64 KB, so 16-bit offsets suffice.
