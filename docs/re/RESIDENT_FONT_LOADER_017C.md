# Resident 0709:017C complete font-loader body

This source translates the complete RES_0709_017C body through its far return,
including both string-copy calls, DAX request, optional Move and FreeMem,
live local reloads, and caller-frame restoration. External callee semantics
are explicit full-CPU/RAM callback boundaries. The existing font reader and
drawer in font_8x8d_201.c are separate APIs, not this loader implementation.

| Source | Measurement |
| --- | --- |
| START.EXE | 65231 bytes; SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| Expanded image | [720C,726F); CS offsets [017C,01DF); 99 bytes, 44 instruction starts |
| Body SHA-256 | 2da845b46455670c4382adeb8c5038613e458534d915d312d6103f6b01c01300 |
| Return | Sole 01DE RETF; no argument cleanup |
| Calls | 018C and 019B to AF8:B0B; 01AD to 0802:0716; 01C8 to AF8:1685; 01D6 to AF8:0254 |
| Relocations | All five call-segment operand words occur in the hash-locked loader relocation stream |
| Packed source | First byte 7245; this specific 99-byte window is contiguous, not a segment-wide mapping |

The source-only audit calls only the existing hash-locked unpacker and
relocation-stream reader. It decodes this assigned body alone and emits
counts, hashes and addresses. No find_units, global call census or emulator
is needed.

## CPU and external-call contract

The incoming IP is 017C. The caller supplies the loader segment bias; each
original external call segment is relocated by that bias modulo 65536.
Each callback observes the actual callee-entry CPU, including far CALL's
pushed CS and return IP, and complete mutable RAM. It executes its callee
and return/cleanup; success must return to the recorded CS/IP. A failed
callback or unexpected return stops with preceding effects retained.

B0B leaves each destination pointer on the caller stack for the later DAX
request. Treating it as a conventional cleanup of all eight argument bytes
would lose those two pointers. The AL-only block-id assignment preserves
AH. The DAX call receives those two strings, the id word, size pointer and
block pointer. A zero size skips both Move and FreeMem. A nonzero size uses
LES for source/destination and then rereads all three local words after
Move returns before passing the release arguments.

The raw-RAM model uses 20-bit physical wrap. Word bytes and LES's four-byte
memory operand are physically contiguous. Separately executed stack
operations wrap their effective 16-bit offsets independently. CPU storage
must be distinct from RAM, code immutable and execution synchronous.
Callbacks own their entire external effects; this source does not implement
a DAX decoder, Pascal string routine, heap allocator or memory mover.

## Current synthetic evidence

Strict C11 and ASan/UBSan pass 131 all-14-CPU and complete-1-MiB-RAM cases:
seven SP offsets (including FFFE/FFFF and LES four-byte boundary crossings),
three SS values (including FFFF), three relocation biases, and both zero/
nonzero size branches, plus five literal size-boundary cases including 8000/FFFF. Ten additional cases stop at each of five callback
failures or unexpected returns. Five entry guards reject invalid arguments.
Seven literal SUB flag vectors anchor the initial stack-allocation flags.
The tests inspect all callee-entry arguments and return words, synthetic
string-pointer retention, AH preservation, relocated targets, live post-Move
local changes, final caller CPU and the complete frame write footprint.
All string and block test data is synthetic.

The source-only review verdict is likely: the complete body and call frames
are translated, while fresh original-byte execution and callee parity remain open.

These checks compare C to synthetic ABI/final-state expectations. Original
bytes were decoded and hash-checked, not executed. No new native/full-game
parity, strict manifest/count, natural reach or denominator claim follows.
The resident ledger still classifies source ownership as unresolved.
Callee implementations and original-byte execution remain separate gates.

## Reused findings and reproduction

The bounded font-target deja query timed out (exit 124); recall is incomplete.
Reused concrete sources are [FONT_8X8D_201.md](FONT_8X8D_201.md) for the
documented whole loader route and distinct font-reader scope,
[RESIDENT_LOADER_MAP.md](RESIDENT_LOADER_MAP.md) for packed expansion/provenance,
and [ECL_BLOCK_LOAD.md](ECL_BLOCK_LOAD.md) for B0B's retained string pointers
and the DAX callee's RETF 18 argument contract.

From the repository root, retain all binaries and original media under
ignored resources:

```sh
mkdir -p resources/font017c-focused
nice -n 19 timeout 20s cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude \
  src/recovered/resident_font_loader_017c.c \
  tests/recovered/test_resident_font_loader_017c.c \
  -o resources/font017c-focused/test
nice -n 19 timeout 20s resources/font017c-focused/test
python3 tools/overlay/resident_font_loader_017c_audit.py \
  --start-exe /path/to/ignored/resources/DOS/POOLRAD/START.EXE
```

Capstone is required only for the static audit. Heavy host-native work remains
subject to the PM scheduling hold. Merge and publication require exact-head
PM review.
