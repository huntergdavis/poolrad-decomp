# LES/LDS segment-word wrap: can a game state reach it? (PM i22172; a reachability note, no fixes)

**The rule.** Pinned DOSBox-X (prefix_none.h C4h/C5h) reads `LES`/`LDS`'s segment word at `(offset+2) mod 64K` when
the offset is above FFFCh. Unicorn and many raw models read it physically contiguously. **This was measured natively on
RES_00BA_2E03 (#1494):** a link word at offset FFFF gave DI 0050 in DOSBox-X and 0A50 contiguously.

A model differs from the original **only** when a far-pointer word sits at offset **FFFDh–FFFFh** of its segment. This
note asks whether any reachable game state puts one there.

The two census lists (122 helpers, and the inline scan) went to the PM in i22170 as `les_helpers.txt` and
`les_census.txt`. They are leads, not verdicts.

## (a) Heap pointers are normalized

**Source:** [SYSTEM_GETMEM_023F.md](SYSTEM_GETMEM_023F.md) and
[SYSTEM_GETMEM_23F_NATIVE_ABI.md](SYSTEM_GETMEM_23F_NATIVE_ABI.md).

Turbo Pascal's GetMem works in 8-byte granules. Each free block and each result is **normalized** by the SYSTEM
normalizer `+0491..04A1` (17 bytes, SHA-256 `440dc8e6…`). The offset part is therefore below 10h, and the granule
leaves it at 0 or 8.

**The consequence:** a far-pointer field at `+k` of a heap object sits at offset ≤ `0Fh + k`. Reaching FFFDh needs
`k ≥ FFEEh`.

**Not measured:** a live sample of GetMem's returned offsets from a hybrid observer log. No current log records GetMem
returns. It can be added to the next observer run, since GetMem is already a registry entry.

## (b) Global far pointers in DS

Every `LES`/`LDS` the models translate that loads straight from a DS global was counted: **7 distinct offsets, the
highest `6F70h`** (`overlay_u25_alloc_1951_cpu.c`; the others are `6A5Ch`, `6716h`, …).

This is a census of the models, **not of the whole DS map**: a DS global far pointer that no model translates is not
counted.

## (c) The stack

[U25_ALLOC_1951_CPU.md](U25_ALLOC_1951_CPU.md) records the game's stack as **SS:SP = 14F2:4000**. It is a 16 KiB Turbo
Pascal stack that grows down, restored from the EXEPACK header, and a static sweep found no other `MOV SS`. Every
frame-based `LES DI,[BP+n]` therefore reads at offsets ≤ 4000h + a few bytes, far below FFFDh.

## (d) Field offsets k in the credited data-pointer leads

The largest constant added to DI/BX/SI in each lead's source is a heuristic (`(di|bx|si) + 0xK` forms):

| Lead | Largest k | Reading |
| --- | --- | --- |
| u6c 3291 / 3fb3 / 408c | BFh / 96h / 96h | record fields |
| u6c 3f31 | **408Eh** | needs a read: probably a DS table index, not a field |
| u6a 019c / 0000 / 025c | CAh / CAh / B1h | record fields |
| u6a 14a9 / 1601 | 106h | record fields |
| u25 01b6 / 1ea6 / 0e0c | **6E00h / 6700h / 6700h** | needs a read: probably DS table bases indexed by a register |
| u25 0000 / string cores | none | |
| u5e 17f6 / 2574 | 108h | record fields |
| u5e 1bf8 | **66DBh** | needs a read: probably a DS slot-table index |
| u3c2 0422 | **2746h** | needs a read: probably a DS table |
| u3c2 01a0, u3c6 0000 | none | |
| u3c6 10a7 | **66DCh** | needs a read: probably a DS slot-table index |
| u77 17ed | 39h | record fields |

## Conclusion, provisional

- **Heap pointers (normalized, plus a record-field k ≤ 108h)** stay below 0118h.
- **DS globals** stay below 6F70h + 4.
- **The stack** stays below about 4010h.

**None of these routes can put a far-pointer word at FFFDh–FFFFh, so the wrap is unreachable through them.**

**Open:**
1. The six large-k leads need a read, to confirm that k is a DS table base and not a pointer field.
2. Pointers not made by GetMem, such as arithmetic on a far pointer into a large static table or video memory, are not
   covered.
3. There is no live GetMem sample.

Item (3) is one observer row. Item (1) is a short read of six sources. A ruling on fixes should wait for both.
