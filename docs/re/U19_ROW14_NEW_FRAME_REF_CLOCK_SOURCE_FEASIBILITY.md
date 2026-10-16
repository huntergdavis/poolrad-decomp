# U19 row-14 new-frame reference and common-clock source feasibility

This source-only note maps the proposed **new** frame reference from guest
page request through VGA latch, renderer start, backend completion and RGB
observation. It adds no issuer, frame token, hook, packet field, parser,
test, or native capture. The merged [page/crop/palette contract](U19_ROW14_PAGE_CROP_PALETTE_CHANNEL_CONTRACT_V2.md)
and [canonical RGB24 producer boundary](U19_ROW14_CANONICAL_POST_BACKEND_RGB24_PRODUCER_SOURCE.md)
describe the separate pixel and backend gates. This note isolates the frame
lifetime and clock links needed to use them together. The host-heavy hold
precludes repeat `deja`; it reuses those checked-in notes' bounded recall.
The [original issuer audit](U19_ROW14_ORIGINAL_HOST_ISSUER_AUDIT.md) and
[backend witness audit](U19_ROW14_ORIGINAL_BACKEND_WITNESS_AUDIT.md) supply
the bounded common-clock and completion blockers reused here. p9 owns those
audits; p5 owns the selected handler and any interior store hook; pC owns
acceptance.

The ignored DOSBox-X revision is
`5a9c7c53481c2e933948039c33256776653f4faf`. The complete inclusive
windows below retain original newlines. Their byte lengths and SHA-256 values
were checked locally. They are **candidate source sites**, not events
measured in the saved U19 run.

| Source window | Bytes | SHA-256 | Feasibility boundary |
| --- | ---: | --- | --- |
| `int10_char.cpp:1184–1213` | 1,097 | `17d105217bf5a6d88f4416a3d38b8f22ac61c8d0bcf564fb4874040c0ad1906e` | A reached INT 10h page-helper request is a candidate input, not the frame's accepted page. |
| `vga_crtc.cpp:247–259` | 687 | `0f6f4cd6fb0781a3f7fd86776596dfcd8b809e0c03b22e8b7a918f3250b33274` | CRTC writes can change pending display start; source and reached value must be retained. |
| `vga_draw.cpp:4089–4148` | 1,948 | `86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce` | The latch can complete the old frame before assigning new `real_start`; display overrides can then change the effective start. |
| `vga_draw.cpp:6190–6220` | 825 | `7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b` | Timer end of a prior update may occur before a new renderer start. |
| `vga_draw.cpp:6259–6290` | 1,151 | `888d758087aa016fa34a920fd9913b6393723e920c22478c128ad5f3e3e9b9a9` | EGA frame-address setup consumes `real_start`, rather than pending `display_start`. |
| `vga_draw.cpp:3956–4038` | 3,429 | `c3ce297f472f8fd67f0b4b41ce8e9f0927cdb2c5fa8052ee87502487c287c963` | Line skip, blanking and address progression can prevent a requested page from producing the proposed row. |
| `vga_draw.cpp:7738–7778` | 1,570 | `c135178ee4d3d5d69b455e8e0f69dc4587e3ce824d1c5b461b1c05e656cb6bc2` | The reached EGA expander/decoder remains a frame input. |
| `vga_draw.cpp:1092–1122` | 1,616 | `706c9e03335312bea83e6983fcf896cf4bc5ef73b73cf31b824e3e48d2e53585` | Palette translation chooses VGA DAC, EGA attribute palette, or raw snapshot by card; effective identity must be captured at decode. |
| `render.cpp:397–445` | 1,959 | `23875191a5488b3bbddae494a95aa0a668ea3025c9cb5c1f29e11bba9fabda5b` | A reached renderer start is the first candidate that can accept the newly latched frame. |
| `render.cpp:472–525` | 2,154 | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | Renderer end and pre-backend cache capture are separate from verified host completion. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | Backend dispatch can return or take a counter-only path; an entry is not a completed new frame. |
| `vga_memory.cpp:374–384` | 207 | `834dc84bc70ae24f8596da3733949ac2fa35745f1a759974e2e23a2cb324656d` | Generic EGA plane assignment is only a candidate p5-owned interior store site. |
| `vga_memory.cpp:637–650` | 585 | `16da1db60e1b9fb953af86f89d1cbcae369ebf72c49eefbfaa532325a6bc4794` | Fast mapped assignment is a distinct p5-owned candidate; handler choice and backing index are unresolved. |

The current public synthetic capture has a separate, bounded limit:

| Public window | Bytes | SHA-256 | Current exposure |
| --- | ---: | --- | --- |
| `tools/replay/u19_row14_in_handler_capture.hpp:28–52` | 675 | `3d61b16d7541b731d7632b6153995ece0603be7190aa7bcdc2ae939ee5d6dfab` | Guest state and sample shapes do not carry a host issuer clock. |
| `tools/replay/u19_row14_in_handler_capture.hpp:99–153` | 2,239 | `baff184f1fdf72cbe82536279a9e304e15c84e8af51d48ffb45cbc1f7783eda9` | Before sample, one opaque `apply_write`, and after sample expose an interval, not the interior assignment time. |
| `tools/replay/u19_row14_in_handler_capture.hpp:155–170` | 642 | `d4d2f5eefd7bf6c54bbf1f15feebe5c877a1076e9461c41aa8116857ae7b3288` | Finish does not create a live frame or common-clock receipt. |
| `tools/replay/cold_start_codeword_row14_phase_clock_qa.py:12–57` | 2,065 | `e21021fa9fdeed4d7b873d27ca0a97fb4f87a7f15221a9721d3d1e7f70a0133e` | QA `host_ns` values are supplied inputs; they do not establish live hook timing. |

## Proposed frame lifetime, conditional on a common issuer

1. A reached guest page helper and CRTC write may program pending
   `display_start`. Neither guest DS page words nor that pending value
   accepts a new frame. Retain the mode, CRTC start, mask and display
   overrides as source inputs.
2. At `VGA_DisplayStartLatch`, any on-demand completion before the new
   assignment belongs to the **old** frame reference. A future issuer may
   allocate a provisional new `frame_ref` at latch entry, but it must never
   relabel that old completion. Record the final effective `real_start`
   after mask/override handling, not merely the first assignment.
3. Accept that provisional reference as a new frame only after a sourced
   renderer start and the address/decoder path that uses its final
   `real_start`. Source the actual page interpretation, machine/mode,
   decoder, line/split/skip state and effective palette identity for the
   same frame. A request for a page or a matching page word is insufficient.
4. Keep one issuer origin/domain and strictly increasing event indices from
   selected handler operation through latch assignment, renderer start/end,
   p9's independently verified backend result and pR's canonical RGB24
   observation. Distinct `write_ref`, `frame_ref`, `backend_event_id` and RGB
   observation identity must be correlated; their equal text or a CPU
   ordinal cannot supply that correlation. p9's original issuer source
   review and live hook are still missing.

Without a p5 interior assignment hook, the conservative selected-write
relation is `apply_call_exit < latch_enter` in the **same** issuer domain;
the present before/apply/after sample cannot place an actual store inside
the opaque call. If latch entry occurs during that interval, store order is
unresolved. A future p5 hook must pin the truly reached assignment and
backing effect before its index can narrow the relation. A valid zero plane
delta would remain distinct from an absent store. This note does not
install or validate that hook.

The [canonical RGB24 producer boundary](U19_ROW14_CANONICAL_POST_BACKEND_RGB24_PRODUCER_SOURCE.md)
still lacks an independently authenticated successful-output criterion and
post-completion RGB24 conversion. Even a correctly ordered new
`frame_ref` cannot turn a backend call, surface API return, GL void swap,
supplied digest, or expected green crop into a completed native frame.

Reject old-frame completion at the latch, pending-only page state, missing
new renderer start, stale or overridden `real_start`, unobserved palette or
decoder, skipped row, mixed origin/domain, duplicated/reordered indices,
opaque-call store overlap, and backend/RGB identities from another frame.
Status: **BLOCKED_LIVE_NEW_FRAME_REF_AND_COMMON_ISSUER_SOURCE**. No live
selected page/palette, natural timing, first writer, backend completion,
RGB parity, frame parity, or C parity follows. Private source, media,
palette, planes and images remain under ignored `resources/`.
