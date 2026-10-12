# Unit 0x9B:+0x0255 record table leaf

`pr_u9b_record_table_0255` translates all terminating paths of the
call-free DOS v1.3 unit `0x9B:+0x0255..+0x0328` body. The routine
sets five record bytes to a default value, then conditionally lowers
each from a DS lookup table. Field meanings remain unknown. The
saved first insertion [reaches this leaf](U9B_RUNTIME_DISPATCH_0000.md)
from the `+0x0000` body at `+0x0191`.

Before implementation, `deja "Poolrad u9B +0255 record leaf RETF4 0x6D 0x96"`
found no exact match; `deja "u9b 0255"` recovered the prior native
dispatch discussion. This work reused [PR #274's native +0191 and
return captures](U9B_RUNTIME_DISPATCH_0000.md), [PR #271's entry
table](U6F_U9B_TRANSFER_1574.md), and the existing 1 MiB
[u6F SS RAM model](U6F_PARTY_INSERT_1446_SS.md). The original
span, extent, and saved state were independently checked again.

| Source | Verified bound |
| --- | --- |
| START.EXE | SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes, SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `+0x0255..+0x0328`, GAME file `0x2ACDC..0x2ADAF` | **212 bytes, 86 instructions**, nine branches, no calls; SHA-256 `f19017361e6ac6197e38eeee7b70f438d29962a34675ff43903ac712444dd326` |
| Exit | `RETF 4` at `+0x0326`, ending exactly at the next table entry `+0x0329` |

The earlier 86 figure describes **instructions**, not bytes. The
source entry table lists `+0x0255` and `+0x0329` as adjacent entries.
No helper call or unresolved callback lies inside this leaf.

## DOS call frame and ordered effects

The API starts **after** the near `CALL +0x0255`, with the call's
return IP, caller CS word, and far record argument already in SS
RAM. On the saved path, the caller's `PUSH CS` at `+0x0190` leaves
`SP=3786`; the near call writes return IP `0194` at `SS:3784` and
enters this leaf with `SP=3784`. `SS:3786` holds caller CS `2CCD`,
and `SS:3788/378A` hold argument offset/segment `0008/449A`.
The C model does not invent those caller writes; its input must
include them. Its first instruction pushes the caller BP, making
`BP=3782` for this saved leaf frame, with local bytes at `BP-1/-2`.

For a disjoint record, outer index `0..4` and inner index `0..7`
follow these source-ordered operations:

1. `+0x0264..+0x0272` reloads the far argument and stores `0x14`
   at record `+0x6D + outer` before testing any inner field.
2. `+0x027B..+0x028C` reloads the argument at each inner index and
   enters the table path only if signed record byte `+0x96 + inner`
   is greater than zero.
3. `+0x028F..+0x02D0` reads DS byte at
   `+0x426E + inner*0x2D + class*5 + outer`, using 16-bit arithmetic.
   It compares the current record field and table byte **unsigned**.
4. Only when the current field is greater, `+0x02D2..+0x030D`
   recomputes the index, reloads the far argument, and stores the
   newly read table byte. The C model repeats these reads rather
   than caching a prior value.

All SS locals, SS argument words, DS table bytes, and ES record
bytes share one A20-off 1 MiB RAM array. Each `LES` reads four
contiguous physical bytes from its SS effective address; offset
`FFFF` can cross the segment's 64 KiB offset boundary. `RETF 4` reads
its return words in the same manner. The model rereads locals and
the far argument at each original site, so an alias can redirect
later record accesses or change the returned CS:IP. Finite alias
paths terminate as in DOS; a cycling alias loops as in DOS. The
only host guard rejects null or short RAM and null CPU state before
execution.

## Saved return and parity gates

The hash-locked saved pre-call RAM snapshot stops at `2CCD:0191`.
The verifier supplies precisely the reached near-call return word
at `SS:3784`; no other RAM byte is synthesized. Running the C leaf
produces a **zero-byte difference across all 640 KiB** from the
native `+0x01AC` snapshot. The saved caller path between the leaf's
return at `+0x0194` and `+0x01AC` does not write RAM: record `+0x9C`
is zero, so it bypasses `+0x0329`.

| Register group | Leaf entry | After `RETF 4`, at `2CCD:0194` |
| --- | --- | --- |
| `CS:IP`, `SS:SP`, `BP` | `2CCD:0255`, `1D16:3784`, `3794` | `2CCD:0194`, `1D16:378C`, `3794` |
| `AX BX CX DX` | `0007 001F 0000 0000` | `0007 0004 0005 0011` |
| `SI DI ES` | `44BA 0008 449A` | `0001 000F 449A` |
| `DS FLAGS` | `14A0 7246` | `14A0 7246` |

The leaf's return flags come from its final comparison of local
outer index with four: ZF and PF set; CF, AF, SF, and OF clear;
the remaining flag bits are preserved. The `RETF 4` CS:IP and
stack cleanup are read from live SS RAM, not hardcoded.

The focused strict CTest covers empty classes, signed and unsigned
gates, table lowering, shared DS/record table bytes, an argument
word changed by a record store, a changed return frame, and far
argument/return reads crossing offset `FFFF`. An independent local
Unicorn 2.1.4 run executes the ignored original bytes for **105
deterministic synthetic cases** and compares all 1 MiB RAM bytes
and every returned register with portable C. All pass. The same
public audit also checks the saved native RAM boundary.

```sh
cmake -S . -B resources/build-u9b-0255 -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u9b-0255 --target poolrad_overlay_u9b_record_table_0255_tests
ctest --test-dir resources/build-u9b-0255 -R poolrad_overlay_u9b_record_table_0255_tests --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u9b_0255_parity_audit.py \
  --native-dir resources/oracle-runs/u9b-entry-zero-runtime-002 --unicorn
```

The C source, synthetic CTest, and audit contain no original code
or game media. All original images and RAM captures remain under
ignored `resources/`. This leaf is a full-C candidate; project-wide
strict and game-identity counts remain unchanged pending independent
PM review.
