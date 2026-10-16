# U19 row-14 generic assignment operation source model

This is a default-off, local source model for the **generic unchained**
DOSBox-X backing-word assignment. It installs no emulator hook, issues no
event index or write reference, and has no native saved-run parity. A bounded
`deja "Poolrad U19 row14 DOSBox-X generic unchained VGA Generic Write Handler
false planeaddr assignment ModeOperation source contract"` search timed out
without an indexed result. The checked-in [assignment gate](U19_ROW14_ACTUAL_STORE_ASSIGNMENT_GATE.md),
[interior hook proposal](U19_ROW14_SELECTED_STORE_HOOK_API_SOURCE.md), and
[plane resolver](U19_ROW14_INTERNAL_PLANE_RESOLVER.md) were reused, then the
original source windows were independently checked. PM allowed the separate
local generic model in durable i11084 after p9 code/source clearance i11003
and pC consumer/interface clearance i11079.

## Exact source boundary

The ignored DOSBox-X revision is
`5a9c7c53481c2e933948039c33256776653f4faf`.
These inclusive `src/hardware/vga_memory.cpp` line windows preserve original
newlines:

| Lines | Bytes | SHA-256 | Role |
| --- | ---: | --- | --- |
| 579-629 | 2,211 | `01c545faf450be21ea04391d23602d5433e9112934c66a2fc83f3da38b13c7cd` | `VGA_UnchainedVGA_Handler` maps the address, runs the update trigger at 590, and calls the generic helper with `chained=false` at 591. |
| 331-384 | 2,343 | `a24d8d1551d2407eb67eb45087e3205f71c6c6eb749d241886a3adc0494d39ee` | The helper adjusts mask and final `planeaddr`, computes `ModeOperation`, merges the prior backing word, and assigns once. |
| 376-384 | 203 | `a67274a7a8f66dcecb01d6ad08ff9c5680fdfdef47d40d290eb4fb471d182230` | Final computation and backing-word assignment at 383. |

The selected guest candidate body is loaded START `[0x7417,0x7501)`, 234
bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
Candidate byte-store PCs are `RES_0709:+0412/+0438/+0445/+0451`.
The model requires an armed `CS=0F2D` candidate and `ES:DI=A000:118F`, an
observed frame at that PC or its audited successor, guest linear `0xA118F`,
byte width one, and `Handler::uvga`. These are supplied conditions, not a
measured handler or native selected-store route.

The actual order is map at 583, **original trigger at 590**, helper call at
591, live mask and final index remap at 331-374, `ModeOperation` and prior
backing-word merge at 376-381, then the original assignment at **383**.
The generic helper's `chained=true` callers at 482 and 538 are outside this
model. The final index is available only after the trigger and remap; the
adapter neither reruns the trigger nor wraps the whole helper as one store.

## Bounded operation contract

`tools/replay/u19_row14_generic_assignment_operation.hpp` accepts the
caller-supplied final word index, backing extent, and exact original line-383
assignment as a nonthrowing callback. Only at that interior site does it
copy four **host backing bytes** before and after invoking the assignment
once. No guest A000 read or extra latch load is introduced. On disabled and
rejected observations, the original assignment still runs once; the source
model never invokes the earlier update trigger. The nonthrowing callback
condition is a caller contract, not an enforced property of this model.

`Input::mapping_bound` defaults false. It must be asserted only when the
caller has linked this final post-remap index to the same selected guest
attempt. The model rejects an absent binding, but cannot authenticate an
asserted one, callback internals, handler selection, thread ordering, or
alias provenance. The final index must address four complete bytes within
the supplied backing extent. `operation_calls` saturates at 2 and includes
disabled passthroughs, so toggling observation off and on cannot hide a
second supplied assignment. `reached_store` means the callback returned;
`sample_complete` means both host-byte copies finished. Neither flag is a
native backing-assignment receipt.

Wrong frame, branch, guest target, byte width, missing mapping binding, or
backing bound returns a typed source-model stop after preserving the original
assignment. Zero and multiple calls, including disabled-to-enabled and
reentrant duplicates, fail `finish`. Equal four-byte endpoints may finish
as a valid **source-model no-op**; they are not pC's accepted native no-op.
A direct-TLB bypass, overridden handler, A200 page alias, missing live
controller/latch state, failed emission, reused or unbound issuer reference,
or unsourced guest-to-index link blocks a selected native receipt. This
model emits no receipt and cannot establish `actual_store`, first changing
writer, displayed page, RGB frame, or strict/full-C credit.

The sibling fast handler uses a different index and final assignment at
647-648. p9 owns the common issuer; pC owns the versioned receipt consumer
and eight-label audit; pR owns passive plane and RGB provenance. This file
does not edit those APIs.

The focused synthetic
`tests/replay/test_u19_row14_generic_assignment_operation.cpp` covers a
changing sample, equal-byte source no-op, wrong frame/handler/guest target/
width/mapping/bounds, callback-on-reject, missing and duplicate calls,
disabled-mode toggles, and a reentrant duplicate. Its warning-strict C++17
syntax check and one guarded `-DNDEBUG` focused executable run passed
locally. The test uses always-evaluated `CHECK` calls, so Release mode cannot
discard operation side effects. CTest and the native path remain unrun under
the PM host gate. Status: `LOCAL_GENERIC_ASSIGNMENT_SOURCE_MODEL`.
