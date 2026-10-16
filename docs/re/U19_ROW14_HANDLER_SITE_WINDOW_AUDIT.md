# U19 row-14 handler site windows

This source-only check binds the four possible resident drawer store PCs to
their immediate instruction successors. It also lists the capture failures
that must reject a selected event while still executing the original handler
write. No new native run was made.

A bounded `deja "Poolrad U19 row14 four handler site successor instruction
window rejection"` found no indexed session. The merged
[page-witness source join](U19_ROW14_PAGE_WITNESS_SOURCE_JOIN.md),
[first-plane source bound](U19_ROW14_FIRST_PLANE_CHANGE_SOURCE_BOUND.md), and
[in-handler capture](../../tools/replay/u19_row14_in_handler_capture.hpp)
provided the image mapping, candidate set, and observation contract. This
check independently hashed the ignored original and decoded each window in
16-bit mode.

## Original instruction boundaries

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The resident image mapping is `0x7090 + RES_0709 offset`. Each five-instruction
window loads a page word from DS, saves it briefly on the stack, loads DI from
`[BP-4]`, restores ES, and performs one byte store. The store has no branch
between its address and listed successor.

| Source and page word | Original image interval | Length / SHA-256 | Store PC → successor | Operand |
| --- | --- | --- | --- | --- |
| Glyph, DS:4A16 | `0x749A..0x74A4` | 11 / `668ad60906de51402939c895a2687eddece913e70b023ea7c7be51d646e9d180` | `0F2D:0412 → 0F2D:0415` | `ES:[DI] ← DL` |
| Glyph, DS:4A18 | `0x74C0..0x74CA` | 11 / `15ee7df4e841c4218395d9c9c64d6e847bd0c7c9e1625122733e29fe04d0401b` | `0F2D:0438 → 0F2D:043B` | `ES:[DI] ← DL` |
| Zero, DS:4A16 | `0x74CD..0x74D8` | 12 / `2bc2aee2f658b6638259de2bc2be7a5de36efd43f248f77fcc251818fe12389b` | `0F2D:0445 → 0F2D:0449` | `ES:[DI] ← 0` |
| Zero, DS:4A18 | `0x74D9..0x74E4` | 12 / `da009d58775ce7c015d1be321a188200dfaf054ab0096febfb08fede5c5b80e7` | `0F2D:0451 → 0F2D:0455` | `ES:[DI] ← 0` |

The intervals above are inclusive. The corresponding half-open routine-offset
windows are `+040A..+0415`, `+0430..+043B`, `+043D..+0449`, and
`+0449..+0455`. The first and second page stores read DS:4A16 and DS:4A18
at their own setup, so a retained endpoint page word does not prove ES at a
reached store. `A000:118F` is only the selected row-14 byte; the source
route has 2,304 conditional store visits, with its first possible selected
visit at ordinal 897. The site/successor pair alone establishes neither
that visit nor a changed EGA plane.

## Capture rejection plan

The merged [synthetic handler test](../../tests/replay/test_u19_row14_in_handler_capture.cpp)
already exercises all four site/successor pairs and one-write completion.
Its negative cases cover wrong guest linear target, non-byte width,
unsupported handler, out-of-range backing index, mutation during passive
sampling, wrong observed CS/IP/ES/DI, missing or duplicate write, wrong
successor ES/DI, invalid supplied identity, and a second emission. For
rejected observations, the original write callback still runs once. A
selected event is default-off and can be emitted only after the matching
successor and a completed capture; emission performs no new state read.

Before using a live packet, the narrow follow-on checks are:

1. Reject wrong arm CS, site IP, ES, or DI, and wrong successor CS or IP, in
   addition to the already tested successor ES/DI mismatch.
2. Reject a null backing pointer, changed host handler or resolved backing
   index across the selected operation, and any event without exactly one
   callback write; preserve the handler's original write in every case.
3. Bind the producer's run, arm, stream, event, raw digest, ordinal, and exact
   PC pair to the capture without sampling again. Keep producer
   authentication separate from the handler's structural check.
4. Require a complete same-run candidate stream and a passive four-plane
   before/after packet with the actual EGA handler state before inferring a
   first value-changing writer.

The existing source audit can be reproduced with ignored local media:

```sh
python3 tools/replay/cold_start_codeword_row14_page_witness_source_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

This note makes no reached-store, changed-plane, page-return, RGB, C-parity,
or full-frame claim. Original bytes, RAM, planes, and pixels remain ignored
under `resources/`.
