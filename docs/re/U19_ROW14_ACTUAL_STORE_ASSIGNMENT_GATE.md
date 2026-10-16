# U19 selected actual-store assignment source gate

This is a source-only gate for a future selected host backing-word write
observation. It adds no emulator hook, packet field, native trace, or first
value-changing writer. Two bounded deja searches for the actual-store and
late-receipt routes timed out without an indexed result. Reused checked-in
research is the [first plane-change bound](U19_ROW14_FIRST_PLANE_CHANGE_SOURCE_BOUND.md),
[in-handler receipt bound](U19_ROW14_IN_HANDLER_RECEIPT_BOUND.md), and
[handler phase-clock proposal](U19_ROW14_HANDLER_PHASE_CLOCK_PROPOSAL.md).

## Exact source and caller boundary

The public source baseline is main e513cad972ba58a1edbe59b91f3525a5d5e74d7f.
The ignored DOSBox-X source is committed revision
5a9c7c53481c2e933948039c33256776653f4faf. Every line window below is
inclusive, retains original newlines, and is hashed from the committed
revision, rather than a locally instrumented working file.

| Source window | Bytes | SHA-256 | Supported role |
| --- | ---: | --- | --- |
| include/paging.h:463-467 | 240 | 4363344295a437947e2b00d654c0cccfc71584051e46b533eef0a69fa307b49a | A guest byte write uses a direct TLB host pointer when present; otherwise it calls the selected PageHandler writeb. |
| src/hardware/vga_memory.cpp:579-629 | 2,211 | 01c545faf450be21ea04391d23602d5433e9112934c66a2fc83f3da38b13c7cd | Generic unchained writeb to do_write to writeHandler8 to generic planar operation. |
| src/hardware/vga_memory.cpp:636-668 | 1,405 | 029a87e8d754cfcd53e564ce48603cbc915da1597459c8fbc25d6a09b5e5f69a | Fast unchained writeb to do_write to writeHandler8 and masked replacement. |
| src/hardware/vga_memory.cpp:2694-2758 | 2,770 | 891693a6a306953b8176f1a742f9b3e2760fc8d694b0135bf02faa90abb9453d | EGA/VGA mode, chained state, complexity, and optimization choose candidate handlers. |
| src/hardware/vga_memory.cpp:2766-2829 | 2,267 | 15e0304c61444c5305903655008f8e15493fd11cc1b839026abf19358a9bd15a | GC memory map chooses the installed page range; later override can replace the A000 handler. |
| src/hardware/vga_memory.cpp:331-384 | 2,343 | a24d8d1551d2407eb67eb45087e3205f71c6c6eb749d241886a3adc0494d39ee | Generic assignment to the resolved backing word is at line 383. |
| src/hardware/vga_memory.cpp:636-649 | 656 | f0b0dcad5958fb49f1082c6e10edae72e581070a813f488b4cb293ac47a115c5 | Fast assignment is at lines 647-648. |

The clean upstream vga_memory.cpp file hashes to
f973a04e9f5ae82864bd7699200652d123e999954b2968786d0c5b1229195374.
A separately instrumented ignored working copy has a different full-file
hash, so window evidence here is bound to the committed revision.

The [original CPU store ledger](U19_ROW14_FIRST_PLANE_CHANGE_SOURCE_BOUND.md)
pins the expanded START drawer at loaded image [0x7417,0x7501), 234 bytes,
SHA-256 3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4.
Its four possible byte stores are RES_0709:+0412/+0438/+0445/+0451.
For the conditional fixed row, 36 cells, eight source rows, four planes,
and two pages produce 2,304 candidate visits at 288 distinct byte offsets.
A000:118F is first possible at ordinal 897 or 898 on the zero sites;
899/900 are glyph-site possibilities. The saved run's page segments and
reached handler are not proved by that arithmetic.

## Assignment versus wrapper interval

The merged handler observer at tools/replay/u19_row14_in_handler_capture.hpp
lines 102-153 is 2,031 bytes, SHA-256
cc868874be41ac2a9979afdb88eea9ee89beb7fc3893fd36ccff2dd4535b4b37.
It samples before, calls the original opaque apply_write once at line 138,
then samples after on a still-valid path. A timestamp at wrapper entry or
exit bounds the actual assignment but does not identify its instant.

The generic path computes a live mask and ModeOperation result, combines
them with the prior backing word, then assigns at line 383. The fast path
uses its separate masked replacement at lines 647-648. A future
actual_store phase therefore needs an issuer at the reached interior
assignment, with a provisional local write reference carried back through
the observer. The page handler, bank, index, GC/Sequencer state, latch,
write mode, and four complete prior/result plane bytes must be measured
without perturbing guest latch or renderer state. Neither EGA mode nor a
source-only map selects generic versus fast for this saved run.

## First evidence and negative gates

The next source-only gate is a p5-only typed contract for the direct-TLB
and generic/fast handler alternatives, assignment-local provisional
reference, and valid no-op versus failed observation. It cannot assert
which alternative ran. A later PM-controlled same-run native stop must
bind selected guest PC/ES:DI and ordinal to the reached page handler,
resolved backing index, live controller state, and nonperturbing four-plane
before/after bytes at the interior assignment.

A claim of first value-changing store must name its scope and interval:
first in the full image, at one backing word, or at the selected target.
It must exclude every earlier changing write in that scope, including
noncandidate writes and changes later restored. A full guest candidate
stream alone has no all-store poststates or authenticated native
coverage; one selected receipt cannot exclude an earlier change. A
successful write with four equal before/after bytes is a valid no-op, not
a missing write. Guest A000/A000 page aliases remain distinct ordinal
events; A200:118F is not the selected A000:118F linear address.

p5 owns this source and any future selected assignment hook. p9 owns the
separate common host issuer and backend source notes; pC owns a versioned
consumer; pR owns the passive plane packet and frame/RGB join. No shared
source, schema, emulator, or original media was edited for this gate.
Status: SOURCE_ONLY_ACTUAL_STORE_BOUND; FIRST_CHANGE_UNPROVEN.
