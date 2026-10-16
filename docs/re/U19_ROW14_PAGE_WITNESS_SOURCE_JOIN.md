# U19 row-14 page return and witness source join

This source-only gate pins the original instruction boundaries between the
resident page helpers' returns and the drawer's possible stores at the fixed
row-14 witness. It does not observe a page-helper call inside U19, a changed
EGA plane byte, a page flip, or host presentation timing. Heavy native remained
held.

The bounded `deja "Poolrad U19 row14 page selector return RES_0709 +008C
+0099 A000 118F bit 20 first store source"` found no indexed session. The
merged [page-return handoff](COLD_START_CODEWORD_ROW14_PAGE_RETURN_RGB_HANDOFF.md),
[store candidates](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md), and
[four-plane watch](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md) supplied the
helper mapping, static store order, and retained witness. This gate rechecks
those inputs and narrows eight instruction windows against the ignored
original START image.

## Original-byte boundaries

`START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
For these resident routines, original image address is `0x7090 + offset`.
The active helper's full 47-byte SHA-256 is
`26854c57a4d438a0e37a6c00ab8a7a6b9bff9567da09f681ee4247aeccd42b77`;
the draw helper's 23-byte SHA-256 is
`6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d`.
The existing page-selector audit separately checks the active helper's one
loaded relocation and the draw helper's original/loaded equality.

| Original offset interval | Bytes | SHA-256 | Source boundary |
| --- | ---: | --- | --- |
| `+0087..+008B` | 5 | `6c44fa171cea0b9d6aa7502458b0d6c591ffa871bf7c75dc27e31b545b4415cf` | Active helper runtime far call; `+008C` is its return point. |
| `+008C..+009B` | 16 | `6a4544c7307fe99e0c26729e2b79c3db2ae6501923901ed98c82a82cf0b3bf1e` | Reload low `[BP+6]`, shift left nine, add A000, store DS:4A16 at `+0099`. |
| `+009C..+009F` | 4 | `99328d20d43f7037d7e84a250f7a942f00f9a85f9c05f034df7214b3fd0264e3` | Pop BP; `RETF 2` at `+009D`. |
| `+00B0..+00B6` | 7 | `5e2fcb9a2f578e16512c2304ba793b55b28e46b6d33385df439b38adbc22b28f` | Draw helper DS:4A18 store, pop BP, `RETF 2` at `+00B4`. |
| `+040A..+0414` | 11 | `668ad60906de51402939c895a2687eddece913e70b023ea7c7be51d646e9d180` | Glyph path reads DS:4A16, loads DI from `[BP-4]`, stores DL at ES:DI `+0412`. |
| `+0430..+043A` | 11 | `15ee7df4e841c4218395d9c9c64d6e847bd0c7c9e1625122733e29fe04d0401b` | Glyph draw-page read and ES:DI store at `+0438`. |
| `+043D..+0448` | 12 | `2bc2aee2f658b6638259de2bc2be7a5de36efd43f248f77fcc251818fe12389b` | Zero path reads DS:4A16, loads the same DI, stores zero at `+0445`. |
| `+0449..+0454` | 12 | `da009d58775ce7c015d1be321a188200dfaf054ab0096febfb08fede5c5b80e7` | Zero draw-page read and ES:DI store at `+0451`. |

The active helper requests BIOS page selection before the `+008C` return;
the draw helper has no BIOS call. Each uses only the low byte of `[BP+6]`
to calculate `A000 + (page << 9)` modulo 16 bits. Both return after their
DS segment store, removing their WORD argument with `RETF 2`. If either
helper is observed, its far return IP/CS and intervening runtime call must
be captured to tie that invocation to U19; equal endpoint words alone do not
prove that it ran.

The merged source/host witness is `(x=122,y=112)`, hence
`112*40 + floor(122/8) = 0x118F` and bit `0x80 >> (122 mod 8) = 0x20`.
The static color-10 plane order is zero, glyph, zero, glyph. Conditional on
the drawer reaching this cell with `[BP-4]=0x118F`, `+0445` is the first
**possible** store to the witness in plane 0, followed by the draw-page
`+0451` and later glyph `+0412/+0438` in plane 1. The page words are read
at the store setup, so a preceding helper result would be consumed there.
The four retained U19 stops show DS:4A16/4A18=`A000/A000`; they have no
synchronous plane bytes and do not show a helper return frame. Earlier
characters can write the row, and a zero store may leave the plane unchanged.

The next native discriminator is one same-run, chronological helper-return
frame and immediate four-plane before/after sample at a selected drawer PC,
as specified in the merged [event plan](U19_ROW14_SAME_RUN_EVENT_PLAN.md).
Until then, first value-changing writer, page selection during U19, and RGB
presentation remain unresolved.

Reproduce the read-only audit with ignored originals and retained captures:

```sh
python3 tools/replay/cold_start_codeword_row14_page_witness_source_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

Only addresses, hashes, and counts are public. Original game bytes, RAM,
EGA planes, and pixels remain under ignored `resources/`. This source gate
earns no native, C, strict, first-writer, page/RGB, or full-frame credit.
