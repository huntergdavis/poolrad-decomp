# Resident 0x8AE:+1742 table writer

This is an address-only audit and bounded C memory-effects model of one
call-free resident procedure. It does not establish that the saved slot-A
one-Up execution selects this leaf or that the procedure has game-specific
identity.

## Prior work and source span

Before decoding, `deja "Poolrad resident 8AE 1742 table writer 55F 22D
alias order"` and `deja "8AE:1742"` found no matching indexed session.
The merged [adjacent-leaf audit](RESIDENT_8AE_LEAVES.md) supplied the
candidate bound, and its +17AE model supplied a portable 64 KiB segment
interface. The +1742 span and effects below were rechecked independently
against the ignored START.EXE, not inferred from the adjacent procedure.

Input SHA-256: DOS v1.3 `START.EXE
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The hash-locked `tools/overlay/resident_loader_map.py` expansion maps
resident `0x8AE:+1742..+17AD` to loaded `0x0A222..0x0A28D` and START
file `0x09F1F..0x09F8A`. All 108 bytes have contiguous copied provenance
without loader fill. The span SHA-256 is
`6128c5126be22f692937ccbb5e5eb2b07c85081a04f086e9bb84579486258484`.
It decodes as 43 x86-16 instructions, zero calls, three jumps, and ends in
`RETF 2` at `+17AB`. The next prologue starts at `+17AE`.

The caller passes one full WORD at `[BP+6]`. The leaf saves/restores AX,
BX, DI, SI, DS, and BP; it changes DS to `0x8AE` for all data accesses.
It makes no external call or direct port I/O.

## Byte-verified memory effects

| Procedure site | Effect in DS=0x8AE |
| --- | --- |
| `+1752` | Store full argument WORD at `+0x01FF`. |
| `+1756..+1766` | Form `SI = 8*argument` with 16-bit wrap; set byte `+0x01D8=1`, byte `+0x01D9=4`, and first record offset `DI=0x022D`. |
| `+1769` | At each of four rows, read a WORD at `0x055F+SI` with 16-bit effective-offset wrap. |
| `+1770..+1790` | On a nonzero WORD, clear 24 WORDs from record `DI+0x2E` down to `DI`; store the source WORD at `DI+2`, the low argument byte at `0x0209+row`, then WORD 1 at `DI`. On zero, leave that record and selector byte untouched. |
| `+1794..+17A0` | Advance DI by `0x30`, SI by 2, decrement byte `+0x01D9` and repeat until four rows finish; set byte `+0x01D8=0`. The row counter ends at zero. |

In row `i`, the source WORD address is
`(0x055F + 8*argument + 2*i) mod 65536` and the output record begins
at `0x022D + 0x30*i`. Reads occur immediately before each row's writes.
This matters if an unusual argument makes a later source address alias
an earlier record. The C model preserves that order and rejects null or
shorter-than-64-KiB segment views before any write.

The merged [dispatch audit](RESIDENT_8AE_00DC.md) proves conditional
+1742 call sites. The [native coordinate watch](SLOT_A_ONE_UP_NATIVE_COORDINATE_WATCH.md)
proves that the enclosing dispatch ran before the observed coordinate
store, but did not measure mode `[0x5372]`, active WORD `[0x5374]`, or
the dispatch argument. Thus this leaf's saved-route execution remains
unproved. A sound or library role is plausible from neighboring timer
and speaker handling; it remains an inference. No game identity or
strict full-C game-count credit is claimed.

## Synthetic verification

`test_resident_8ae_table_1742.c` covers zero rows and unchanged records,
separate hit/miss records, full-WORD argument versus low-byte selector,
effective offset wrap through `0xFFFF`, and a row-3 source that observes
an earlier row-0 output write. It also checks null/short-view rejection.
