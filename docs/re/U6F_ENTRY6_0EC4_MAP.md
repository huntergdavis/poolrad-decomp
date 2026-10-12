# Unit 0x6f entry 6 +0x0EC4 (`0x7C:0x3E`): static first-path map of the monster-record initializer

Read-only static analysis (no C, no strict credit, no route advancement). This is the callee of the unit `0x25` entry 5 wrapper (`0x2B:0x39`, `+0x0411`) that LOAD MONSTER reaches
([U1C_LOAD_MONSTER_0450_MAP.md](U1C_LOAD_MONSTER_0450_MAP.md)). Unit `0x6f` file base `0x1DFE6`; the routine is `+0x0EC4..+0x1261`, file `0x1EEAA..0x1F247`: **926 bytes, 341 instructions**,
ending in `retf 6`. SHA-256 `1160665fef6397a88b922ce115836e5cf4b55f09b5c4db04e765e8a02f08f747` (GAME.OVR, 206,105 bytes, `872964a3…7928614`). A 0x11A-byte frame. The stub arithmetic that
maps `0x7C:0x3E` to this entry is calibrated on `0x7C:0x57` = entry 11 (stub offset `0x20 + 5*entry`). `deja` found no earlier note (no match). Only offsets, lengths, hashes and instruction
descriptions are published here; **no machine-code bytes and no asset or script bytes**. The four code-segment string constants named below are described by length and hash only.

## Arguments and what the callee reads of the id

`retf 6` pops 6 bytes: `[bp+6]` far pointer to the 285-byte (`0x11D`) object the wrapper allocated, `[bp+0xA]` the id word. Across the **whole** routine `[bp+0xA]` is referenced at exactly three sites
(`+0x0F19`, `+0x10B4`, `+0x119C`), each `mov al, byte ptr [bp+0xA]; push ax`: **only AL is read; the high byte of the pushed word (which the wrapper loads from the `GetMem` offset's high byte) is never used by
this routine**. The `push ax` hands the callee's `0x802:0x716` a word whose high byte is whatever `AH` holds at that point (a leftover from the preceding runtime calls, not the id's); whether `0x802:0x716`
uses that high byte is not decoded.

## Structure: one object, three block loads (same id, three files)

Each block is found by building a file name in a 256-byte-class Pascal string local from `cs` constants and the byte `[0x5376]`, then calling `0x802:0x716(AL = id, far &size, far &ptr)`:

| Step | Range | What it does |
| --- | --- | --- |
| name 1 | `+0x0ECB..+0x0F19` | `Str([0x5376] as a longint, 1-width...)` via `0xAF8:0x1366`; `StrLoad cs:0x0E98` (len 3, hash `403e928a…`), `StrCat` the number, `StrCat cs:0x0E9C` (len 3, hash `a68c46ae…`); a second local := `cs:0x0EA0` (len 4, hash `a374cb55…`) |
| block 1 | `+0x0F27..+0x107B` | `0x802:0x716` -> `size` (`[bp-2]`), far `ptr` (`[bp-6]`); **if `size = 0` -> error path** (`+0x0F32..+0x0F4C`: copy a 22-byte constant at `cs:0x0EA5`, hash `7487c754…`, call resident `0x709:0x0BF5`, then `0x899:0`; neither is decoded here, so whether the path returns is unknown); else `Move(0x11D)` the block into the object (`0xAF8:0x1685`), then clear pointer fields: `+0xC8`, `+0xCA`, `+0x7F`, `+0x81`, `+0x104..+0x10B`; `+0x2E` selects `+0xC0`: values 1, 3, 5 give 1, values 2, 4, 6, 7 give 2, others leave it; `+0xC1..+0xC3` and `+0xC5..+0xC6` get `((r + 8) << 4) + r` truncated to a byte for **five `0xB0:0x48(1, 6)` rolls** (three, then two) (r = AL), `+0xC4 := 0xC4`; `FreeMem(loaded block)` (`0xAF8:0x0254`); **`[0x557C] := 1`** |
| name 2 / block 2 | `+0x1080..+0x1163` | same name with `cs:0x0EBC` (len 3, hash `dc2b108f…`), `0x802:0x716`; if `size <> 0`: split the block into **9-byte units** with a chain on `+0x7F/+0x81` (`GetMem(9)` + `Move(9)` per unit, link words at unit `+5/+7`), `FreeMem` the block |
| name 3 / block 3 | `+0x1168..+0x125A` | same with `cs:0x0EC0` (len 3, hash `7d4aaad1…`); if `size <> 0`: **0x3F-byte units** chained on `+0xC8/+0xCA` (link words at unit `+0x2A/+0x2C`), `FreeMem` the block; at `+0x1252` (reached on both block-3 paths) **`[0x557C] := 0`**, then `lcall 0x899:0x0104` (not decoded), `mov sp,bp; pop bp; retf 6` |

Blocks 2 and 3 have no error path: `size = 0` just skips (`+0x10CB..+0x10CD`, `+0x11B3..+0x11B5`).

## Branch predicates (first path)

`[bp-2] = 0` after block 1 (error path); `+0x2E` of the loaded record in {1,3,5} / {2,4,6,7} (sets `+0xC0` to 1 / 2); `[bp-0x13]` loop counters 1..3 and 5..6 (fixed trip counts: three then two roll
stores, five rolls in all); `offset >= size` loop exits for the 9-byte and 0x3F-byte splits. No predicate depends on the id except through what `0x802:0x716` returns.

## Effects

* **DS:** reads `[0x5376]` (the "ECL file byte"); writes `[0x557C] := 1` (after block 1 succeeds) and `[0x557C] := 0` (at `+0x1252`, the end): a flag set only while blocks 2 and 3 are loaded. These are the only two DS stores in the routine (the `[0x557C]` byte may also be written by the error path's callees, undecoded).
* **Object (`[bp+6]`):** filled from block 1 (`0x11D` bytes), then `+0xC0..+0xC6` (class byte and the random bytes), pointer fields cleared and, for blocks 2 and 3, `+0x7F` and `+0xC8` chains set. The wrapper
  ([U25_ENTRY5_0411.md](U25_ENTRY5_0411.md)) returns exactly these two chain heads.
* **Heap:** `GetMem(9)` per 9-byte unit and `GetMem(0x3F)` per 0x3F-byte unit; `FreeMem` of each loaded block with its returned size.
* **Random:** five calls to `0xB0:0x48` (unit `0x9e` entry 8: count byte, range byte pushed `1, 6`), results used as above.

## Call targets

| Target | Identity | Status |
| --- | --- | --- |
| `0xAF8:0x1366` | Pascal numeric-to-string (longint) | not decoded here |
| `0xAF8:0x0B0B`, `0xB98`, `0xB25` | `StrLoad`, `StrCat`, `StrStore` | decoded earlier ([U1C_HORIZONTAL_MENU_10D1.md](U1C_HORIZONTAL_MENU_10D1.md)) |
| `0x802:0x716` | **resident routine at START.EXE expanded image `0x8736`**: composes a file name (`StrStore`, `StrLoad`, `StrCat`, a near helper at `0x5CD`), and uses `0xAF8:0x14FB` / `0xAF8:0x04A9` (block read and file calls) four times, with an error path calling `0x899:0` | **first unresolved file/block-loading boundary** (an earlier undecoded runtime call, `0xAF8:0x1366`, only builds the file name): file open/read by name and block id, returning `size` and `ptr` |
| `0xAF8:0x1685` | Move-shaped runtime copy (source far, destination far, count) | not decoded here |
| `0xAF8:0x023F` / `0x0254` | `GetMem` / `FreeMem` wrappers | decoded ([U1C_CLEARMONSTERS_125D.md](U1C_CLEARMONSTERS_125D.md), [U1C_LOAD_MONSTER_0450_MAP.md](U1C_LOAD_MONSTER_0450_MAP.md)) |
| `0xB0:0x48` | unit `0x9e` entry 8 roll | model exists ([U68_MOVEMENT_FLAG_04DF.md](U68_MOVEMENT_FLAG_04DF.md)) |
| `0x709:0x0BF5`, `0x899:0` | error-path display and exit | not decoded |

## Unknowns

1. The file names, the block format and what `0x802:0x716` returns for a given id (the content of the three files is not read or published); the first `size = 0` outcome is the error path for the record block.
2. Whether `0x802:0x716` uses the high byte of the id word it is passed (here a residue, not the id's); the wrapper's offset-high-byte propagation therefore does not reach a read of AH in this routine.
3. The exact semantics of `0xAF8:0x1685` and `0xAF8:0x1366` (assumed Move and number-to-string from their argument shapes).
4. DOS-observed effect order for the `0xB0:0x48` rolls, the error path and `[0x557C]`.

No DOS reachability, native search reachability, one-Up parity or count is claimed.
