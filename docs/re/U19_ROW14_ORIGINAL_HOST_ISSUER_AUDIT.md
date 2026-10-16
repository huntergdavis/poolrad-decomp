# U19 row-14 original host issuer and frame identity audit

This is a bounded source audit of the selected host write through the VGA
latch and renderer. It finds no issuer in these reached source boundaries that
stamps the selected write and backend outcome from one named run-wide host
clock. It does not prove that no timer exists elsewhere in DOSBox-X, install
a hook, observe a saved run, or authenticate a display frame. Two bounded
`deja` searches for the U19 host issuer and backend witness returned no
indexed finding. This audit rechecks the merged [issuer proposal]
(U19_ROW14_HOST_ISSUER_PROPAGATION_PROPOSAL.md), [handler phase proposal]
(U19_ROW14_HANDLER_PHASE_CLOCK_PROPOSAL.md), and [backend boundary]
(U19_ROW14_BACKEND_COMPLETION_SOURCE_BOUNDARY.md).

## Exact source lock

The public handler source is from main
`ef587349de5405d959b184661534f397dc42ce99`; its complete SHA-256 is
`b81343bcf632298da70517c1e24a6feeaed375b2888a68d476ca9f82adc4c055`.
The ignored original DOSBox-X source is clean revision
`5a9c7c53481c2e933948039c33256776653f4faf`. Its complete
`vga_memory.cpp` SHA-256 is
`f973a04e9f5ae82864bd7699200652d123e999954b2968786d0c5b1229195374`;
the independently reviewed assignment windows below are the portable
cross-copy pins. Every line window is inclusive and retains original
newlines.

| Source window | Bytes | SHA-256 | Bounded fact |
| --- | ---: | --- | --- |
| `u19_row14_in_handler_capture.hpp:28–60` | 884 | `398eaa597d3e860dfd0b41a2534c7fa37102f4645ff728799fe980eb2d7cf916` | The sampled guest frame and capture state have no host clock, origin, domain, or rendered-frame identity. |
| `u19_row14_in_handler_capture.hpp:100–152` | 2,156 | `6ee1eca8d64dd1b918327ac19b80db255da038de5f0283fbc59f2f5b3ee751b8` | `observe_write` samples before and after one opaque `apply_write`; it does not expose the assignment instant. Failure paths still execute the original write once. |
| `u19_row14_in_handler_capture.hpp:173–202` | 1,056 | `9248133267d2a1bf352dd423557f95cd8668bfe2ee70aaae152e1848efe18968` | `SelectedEventInput` and `SelectedEvent` carry declared run/arm/stream/plane labels and guest PC, but no host event or rendered-frame ID. |
| `u19_row14_in_handler_capture.hpp:254–278` | 1,394 | `4ea2680807997f1c5b5ffd4c6bcbfa623ed9b55b4e289d5374e8314e492ee691` | The eight selected labels are bound only after `finish` accepts the successor. Emit time cannot be substituted for store time. |
| `vga_memory.cpp:344–384` | 1,531 | `0510d93e6333a1cc461a6d9d3a5cf65cf3290289ffb82697dcede74771c0c0ba` | The generic planar path assigns its resolved backing word at line 383 without an event receipt. |
| `vga_memory.cpp:636–649` | 656 | `f0b0dcad5958fb49f1082c6e10edae72e581070a813f488b4cb293ac47a115c5` | The fast planar path assigns at lines 647–648 without an event receipt. Runtime handler selection is not established here. |
| `vga_memory.cpp:683–699` | 796 | `5877a7b3f017422e69223d2abac2263a26115de972ee04a8d12ad53000fc6d8d` | A `PIC_FullIndex` sample here computes CGA snow position, not a selected-write/backend issuer. |
| `vga_draw.cpp:4060–4077` | 906 | `d6f7ca1dbe991cc4284d97ad493acff4f63d1dd6e7fe2073fc527c0c67268aeb` | Another `PIC_FullIndex` sample computes on-demand scanline progress. |
| `vga_draw.cpp:4089–4148` | 1,948 | `86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce` | The latch can complete the old frame before assigning the new `real_start`; neither phase emits a common-clock ID. |
| `vga_draw.cpp:6190–6220` | 825 | `7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b` | End update can be followed by an override or failed new renderer start. |
| `render.cpp:472–525` | 2,154 | `c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3` | Renderer abort, skip, and output gating do not issue a shared write/backend frame record. |
| `sdlmain.cpp:3187–3270` | 2,256 | `c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e` | Backend dispatch has early returns; the OpenGL counter-only arm increments `actual_frame_count` without presentation. |

## Identity and order established by this path

The p5 wrapper preserves guest `CS:IP/ES:DI` for its selected operation. It
can bound an opaque write between pre- and post-samples, then bind the eight
declared selected-event labels after the accepted successor. The original
generic and fast assignment sites do not stamp an `actual_store` event. Thus
neither a candidate ordinal, a plane-pair sequence, nor the later receipt
emission is a sourced host store index.

The latch's preassignment completion belongs to the prior `real_start`.
`RENDER_EndUpdate` and `GFX_EndUpdate` may stop before a selected new-frame
presentation. PIC scanline timing, renderer skip logging, `frames++`, and the
OpenGL `actual_frame_count` are separate timing or display counters; the
audited path does not bind any one of them to the handler receipt, latch,
backend outcome, and RGB observation as a common `clock_origin` and
`clock_domain`. Equality of supplied `run_token` strings or page words does
not fill that source gap. The [typed visual witness]
(U19_VISUAL_WITNESS_QA_CONTRACT.md) checks supplied event ordering, while the
[v2 RGB packet](U19_ROW14_COMPLETED_BACKEND_FRAME_PACKET_V2.md) deliberately
has no sourced frame/clock fields.

The first new evidence required is a separately reviewed live issuer and
frame reference: one run/origin/domain, ordered indices at the selected
write interval or proven interior store, latch and actual new-frame start,
selected backend outcome, and RGB observation. A wrapper interval cannot
claim an interior assignment index without p5's actual-store hook or a
reviewed interval-only consumer rule. The frame reference must retain the
`real_start` used by the frame and reject a previous-frame completion.
These are future requirements, not a current API or verified same-run join.

p5 cleared this source-only scope in durable inbox `i10317`; pC and pR
cleared their adjacent consumer and display scopes in `i10316` and
`i10319`, relayed by PM in `i10334`. Their handler, consumer, parser, and
wire files are untouched. Original source and any future traces, game media,
or RGB buffers remain ignored under `resources/`. This note establishes no
live issuer, first writer, selected page, native frame, RGB parity, or C
parity.
