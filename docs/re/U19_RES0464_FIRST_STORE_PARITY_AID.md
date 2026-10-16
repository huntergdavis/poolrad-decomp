# U19 row-14 first-store EGA and RES0464 parity aid

This source-only note separates the CPU byte offered by the U19 drawer from
the four EGA plane bytes that a reached store could change. It adds no
native stop, plane capture, host pixel, or full-function parity claim.
The bounded `deja` attempt for this work could not create its index lock;
PM declined the retry under the host-load gate. No indexed finding was
reused. The source evidence below was checked independently.

## Pinned U19 route and first possible witness store

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its **expanded MZ image**, not a raw file slice, has the 234-byte EGA
drawer span `[0x7417,0x7501)` with SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
The four store instructions were rechecked through the repository MZ
unpacker: `+0412` and `+0438` are `26 88 15` (`MOV ES:[DI],DL`, SHA-256
`4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e`);
`+0445` and `+0451` are `26 C6 05 00` (`MOV ES:[DI],0`, SHA-256
`5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29`).
Their loaded drawer offsets are `0F2D:0412/:0438/:0445/:0451`.

The [candidate-order audit](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md)
and [CPU-source model](U19_ROW14_CPU_STORE_SOURCES.md) establish that
color 10 takes zero, glyph, zero, glyph branches for plane-loop indexes
0, 1, 2, 3. The retained four-byte map-mask table is `08 04 02 01`
(SHA-256 `1ba1475eadf8ae7a3b170515ef3c887513a6feed761b5f6f889882b809cd553f`);
indexing it by `3 - plane` offers Sequencer masks `01,02,04,08` in loop
order. For the witness at byte `A000:118F`, bit `20`, the plane-0 zero
store at `+0445` is the first **possible store to this witness within
that character**; `+0451` targets the second page. Plane 1's glyph
stores follow at `+0412/:0438`. Earlier characters can already have
written row 14, and an offered zero can leave an already-zero plane
unchanged. No first **value-changing** writer is established.

## What the first CPU store can change

The pinned private DOSBox-X `vga_memory.cpp` SHA-256 is
`f506017948503fa22ce40715205293c987473a938247cbd09ba409ffbb652c14`.
For its generic unchained write handler, after paging/bank and
odd/even/extended-memory address resolution, the four-plane backing word
is updated as:

```text
effective_mask = Sequencer full_map_mask, reduced by odd/even routing if active
data = ModeOperation(CPU_byte; GC mode, rotate, set/reset, raster op, bit mask, latch)
new_planes = (old_planes & ~effective_mask) | (data & effective_mask)
```

This is the source formula at `vga_memory.cpp:331..385`, conditional on
that handler being active. The fast unchained handler at lines 640..665
has a distinct expanded-CPU-byte write formula. The [internal-plane
resolver](U19_ROW14_INTERNAL_PLANE_RESOLVER.md) already guards handler
identity, host bank, remapped index, and backing bounds; `A000:118F`
alone does not identify a backing word.

For a plane-0 `+0445` zero offer, an effective Sequencer mask of `01`
limits a normal unchained write to plane 0. It does **not** by itself
force that plane byte or bit to zero:

| GC write path | Meaning of the CPU zero | Latch requirement |
| --- | --- | --- |
| Mode 0, rotate 0, set/reset disabled, raster op replace, bit mask `FF` | Selected plane receives zero | Latch does not supply masked data on this exact setting |
| Mode 0 with a partial bit mask or another raster operation | Some output bits can come from or combine with the latch | Capture GC fields and all four latch bytes |
| Mode 1 | CPU zero is ignored; selected plane receives its latch byte | The latch is the data source |
| Modes 2/3 | CPU bits or set/reset and GC masks determine data | Latch can affect the raster operation |

The guest `MOV` changes no general register, segment register, or status
flag other than advancing IP, but its memory operation can update EGA
backing planes. A guest `A000` diagnostic read loads the latch; changing
GC read-map and restoring only the visible GC/Sequencer registers does not
restore that hidden state. The [capture-integrity audit](U19_ROW14_PLANE_CAPTURE_INTEGRITY.md)
also requires proof of read mode 0, disabled odd/even remapping, active
handler, and successful four-plane sampling before map-selected dumps can
be interpreted as plane bytes. Record live GC write mode, rotate/raster
operation, enable/set-reset, bit mask, read mode/map, miscellaneous register,
Sequencer map mask/memory mode, all four latch bytes, prior backing word,
and resolved backing index at the selected immediate instruction pair.
The existing `+02DD` host RGB witness does not supply these values.

## Check against the earlier RES0464 +29E6 source gate

The [bounded +29E6 gate](RES0464_29E6_EXTENT_STORE.md) pins 13 **loaded
RAM** bytes `8B 46 EA 48 8B 7E E2 D1 E7 89 85 70 4D`, SHA-256
`9ca81ab58e1a2b24f894125a76afde21c2e0ad12c9bc527463e3250a77c876b5`.
For its separately supplied frame it writes only the word `138F` at
`DS:4DE6` (`DS=14A0`, conventional physical `0x197E6`) and stops at
`0C88:29F3`. Its row-slot input is `003B`; that local index cannot be
equated to the U19 text row 14. It performs no `A000:118F` access,
GC/Sequencer port write, latch read, or U19 drawer call in this bounded
slice. The loaded span's original-file mapping is not asserted here.

| Parity-review field | Finding |
| --- | --- |
| `target_function` | U19 first possible `A000:118F` drawer store |
| `source_candidates` | U19 `0F2D:+0445/+0451` zero stores; RES0464 `0C88:+29E6..+29F3` extent store |
| `signals` | U19 writes through `ES:[DI]` into EGA; RES0464 writes `DS:4DE6` in conventional RAM and has no direct video side effect |
| `score` | Direct-store match: 0/5 independent signals; address, instruction, data source, controller effect, and caller boundary differ |
| `verdict` | `not_match` for a direct first-store explanation; `unclear` for any later indirect influence |
| `review_notes` | No supplied same-run call chain or dependency from `DS:4DE6` to the U19 page words, font, or selected EGA store; the +29E6 frame is synthetic |

The checker pass leaves three open questions: whether the reached U19
instruction is the first *actual* value-changing store, which handler
and hidden latch were live, and whether earlier RES0464 metadata could
influence a later route outside the +29E6 bounded slice. A shared
"video" theme or numeric row label does not resolve any of them.

## Separate source leads

1. Trace the resident mode-1 transfer from A200 source reads through
   four latches to A000 writes using its own [row-56 C boundary](SLOT_A_U40C_ROW56_TRANSFER_MODEL.md)
   and original bytes. That path must not inherit the U19 drawer's GC
   assumptions without a live controller snapshot.
2. Audit an independent U19 `+02DD` printer return CPU/stack and page
   helper source boundary. A post-return host frame cannot stand in for
   a paired instruction-level plane capture.

Original media, RAM, planes, latches, and pixels stay under ignored
`resources/`. No native, first-writer, host-page, full-C, or identity
credit follows from this note.
