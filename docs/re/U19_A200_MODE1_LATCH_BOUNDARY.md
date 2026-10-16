# U19 witness versus the saved A200 mode-1 transfer

This source-only gate asks whether the already measured saved-slot-A
`RES_0464:1B52` transfer can account for the U19 row-14 witness byte at
`A000:118F`. It cannot **in that saved flush**: the reached page writes end
before the witness. The saved flush and cold-start U19 print are different
runs, so this address exclusion is not a claim about every `+1B52` call or
about the live U19 controller state. No new native or emulator run was made.

The required bounded `deja` attempt for the U19/RES0464 work could not
obtain the local index lock. PM declined a retry under the host-load gate;
no indexed finding was reused. The merged [all-dirty transfer](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md),
[row-56 C gate](SLOT_A_U40C_ROW56_TRANSFER_MODEL.md), and
the separate U19 first-store parity note (PR #774) supply the
separate run boundaries. The original file hashes and range calculation
below were checked again.

## Original byte and ownership boundary

Ignored DOS v1.3 `START.EXE` is 65,231 bytes with SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The contiguous original **file** span `[0x61CB,0x64D5)` is the 778-byte
`RES_0464:1B52..1E5B` flush, SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
Its runtime Move callee `0AF8:1685..16A8` is original file
`[0xC2FB,0xC31E)`, 35 bytes, SHA-256
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.
These two code spans, the saved-slot-A offscreen object, and its row metadata
are separate from U19's `38D3:02D8` printer and `0F2D:0412/:0438/:0445/:0451`
drawer stores. The [merged all-dirty C model](../../src/recovered/resident_464_dirty_transfer.c)
owns the reached 88-row saved flush; this note adds no duplicate transfer C.

For the measured video-state-1 flush, the original `+1C37/+1C7E/+1CC6/+1D0F`
stores stage offscreen CPU bytes into `A200` under Sequencer masks 1, 2, 4,
and 8. At `+1D31..+1D49`, the original selects mask `0F` and Graphics
Controller write mode 1. Its `+1DA0` call enters runtime Move; each `A200`
read at `+16A1` fills four EGA latches, and each `A000` write consumes them.
The CPU-visible read-map byte is not the four-plane write source. The
original `+1DB0..+1DB5` restores GC write mode 0 after the transfer.
That restoration does not prove a later U19 latch value or any page state.

## Saved-run address exclusion

The [saved all-dirty record](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md) pins
88 dirty rows `r=16..103`, each with page offset
`0x03C3 + 40*(r-16)` and length 11. Therefore its page spans are
`[0x03C3,0x03CE)` through `[0x115B,0x1166)`, with gaps between rows.
The U19 witness `0x118F` is `0x29` bytes after the final exclusive end;
none of the 88 saved `A000` row spans can touch that byte. The original
transfer and Move SHA checks plus the 88-span arithmetic passed on the
ignored START.EXE; the pre-existing native/C comparison remains
27,928/27,928 captured plane bytes for the saved run. That comparison
is retained evidence, not a new test or a U19 cross-run comparison.

| Parity-review field | Finding |
| --- | --- |
| `target_function` | U19 candidate first store to `A000:118F` |
| `source_candidates` | Saved-slot-A `RES_0464:1B52` mode-1 `A200→A000` transfer |
| `signals` | Same EGA page segment but disjoint reached offsets, different caller/run, different GC write mode and CPU data source |
| `score` | Direct saved-flush witness match: 0/4 required address, route, controller, and temporal signals |
| `verdict` | `not_match` for this saved flush as the direct U19 witness writer; `unclear` for other unmeasured flushes or later indirect state |
| `review_notes` | U19 lacks a paired pre-store plane/controller/latch capture; the saved slot-A mode-1 result cannot fill it in |

For a later **same-run** `+1B52` candidate, first bind the caller and
video-state-1 branch, read its own dirty flags/extents/page base, and prove
its computed page span includes `0x118F`. Then pair its `A200` source plane
and four-latch read with the immediate `A000` successor while preserving
the controller state. U19's direct drawer CPU source byte, and the
separate `+02DD` caller return, remain independent boundaries.

All game media, RAM, EGA planes, latches, and RGB stay under ignored
`resources/`. This note adds no first value-changing writer, U19 mode-1
reachability, full-C, or game-identity credit.
