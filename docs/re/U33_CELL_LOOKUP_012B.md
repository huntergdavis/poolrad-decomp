# Unit 0x33 +012B cell lookup

`pr_u33_cell_lookup_012b` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+012B..+01AC` and composes its two far calls, unit
`0x3BD` public entries 3 (`+0350`, the reviewed
[two-bit field lookup](U3BD_FIELD_ENTRY3.md)) and 2 (`+047B`, the
reviewed [nibble lookup](U3BD_NIBBLE_ENTRY2.md)), reached through the
overlay stubs `03D5:002F` and `03D5:002A`. Its only callers are the two
sites in `+01AD`, translated in [U33_CELL_PAIR_01AD.md](U33_CELL_PAIR_01AD.md).

The required recall was attempted once this session with
`timeout 90 deja "poolrad next unowned original function cluster strict
full-C translation queue"`; it found no indexed session. Current `main`
and every remote branch were searched for `+012B`/`+01AD` translations of
unit `0x33`: none exist. The merged unit `0x25` `+0482` model reaches the
same entry 2 through a host hook, so no full-state model of entry 2 or 3
existed. Both are therefore folded here from their original bytes into
`src/recovered/overlay_u3bd_lookup_fullstate.c`, composing the merged
[`+031F` range leaf](U3BD_IN_GRID_031F.md) full-state model; the folded
bodies are run as direct differential targets below. They are already
strict originals and gain nothing here. Reused methods: the
[U9B composition pattern](U9B_ENTRY_ZERO_COMPOSITION.md), the
[U7F](U7F_STATE_FLAGS_0F7E.md) differential and native harness, and the
[U6A pair](U6A_RECORD_WEIGHT_0000.md) relocated-stub oracle.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x33 | GAME file base `0xC5D1`, stub segment `0x003B`; `+012B` is an internal prologue |
| Function | GAME file `0xC6FC..0xC77D`; **130 bytes, 49 instructions, nine conditional branches, two far calls** |
| Function SHA-256 | `8a4956ef566e0c033171d771c19d0dc48aaf4b7d8e04e99be34c6fe3e3f276ae` |
| Far calls | `+0155` `LCALL 03D5:002F` (entry 3, `+0350`, 299 bytes, SHA-256 `3b0a525351d0a12b3afeed4c9d9d39e28fdf3564166cdab0d16219fad1b60498`) and `+0170` `LCALL 03D5:002A` (entry 2, `+047B`, 255 bytes, SHA-256 `92d75878d963a7f5766b1ea49e2f6c1fb51287775d06a5b6ca485fee8bc43658`); both segment words fixup-backed |
| Leaf | `+031F..+034F`, 49 bytes, SHA-256 `ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b`, merged model reused |
| Exit | `RETF 8` at `+01AA`; exact end `+01AD` exclusive |

Every branch target lies inside the 130-byte body. The audit decodes
each unit-0x33 routine from its own prologue and finds exactly two aligned
`PUSH CS; CALL +012B` sites, `+01AD` at `+01D3` and `+0209`. The
out-of-grid path compares y with DS `6AAE`, the party y byte named in
[GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) and used by the entry 2/3 callers,
and the lookups read the map id `84DC` and the far buffer `6A5C` that the
same notes document. `+01AD` passes x, y and a facing and consults the
per-facing step tables at DS `2736`/`273F` recorded in the unit 0x3BD
view evidence. No gameplay meaning is inferred for the returned 0/1/3;
"cell lookup" names the operation.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS, `[SP+4]` an unused word (the caller pushes its BP), `[SP+6]`
facing, `[SP+8]` y and `[SP+A]` x, each read as its low byte; `RETF 8`
leaves SP twelve bytes above entry. The routine pushes BP and reserves
two bytes; its result lives at `[BP-1]`.

Four signed compares reread x and y: any byte outside 0..15 takes the
out-of-grid path. In the grid it pushes x, y and facing as the live AX
words and calls entry 3; a zero result stores 1. Otherwise it pushes the
same three bytes again and calls entry 2; zero stores 0 and anything
else stores 3. Out of the grid it stores 0 only when y equals DS `6AAE`
and facing is 2 or 6, else 1. The last compare or `OR AL,AL` on each path
supplies the returned arithmetic flags; undefined AF after `OR` is
represented as zero, which both tested engines also produce.

Each far call is modelled as the loaded-state transfer: the model reads
the call site in RAM at `cs:(ip+2A)` and `cs:(ip+48)` (opcode `9A`, the
stub offset and the relocated segment word), reads the five-byte far jump
at that paragraph's offset `2F` or `2A`, pushes CS and the return offset,
enters the folded entry model at the jump's CS:IP, and checks the RETF
frame. A site or stub that is not the loaded-state original reports
`CODE` before any effect; a frame that no longer returns reports
`TRANSFER`. `cpu->ip` is the body's load offset (`012B` in the game).
Inside the folded entries every operand is reread from RAM, including the
unwritten `[BP-1]` local that an unrecognised selector returns; the
`SHR AX,CL` flags follow the last bit shifted out, with undefined AF as
zero. Offsets wrap at 16 bits; words, LES operands and return frames are
physically contiguous; physical addresses wrap at 1 MiB. Null RAM/CPU or
less than 1 MiB is rejected before mutation. No loop exists, so every
input returns.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`) checks the
three in-grid outcomes (entry 3 zero, entry 2 zero, nonzero), the
party-row and other out-of-grid outcomes with their exact compare flags,
registers and frame, a stub that is not a far jump (`CODE`) and the host
guards.

The original-byte oracle passes **1,212 returning cases** for this body,
each comparing the entire 1 MiB RAM and all 14 CPU words with no flag
mask: every pair of ten signed coordinates with eight facings, edge
coordinates with twelve facings over maps 0, 5 and 10, 240 random
coordinates, facings, maps, party bytes, buffers, tables, flags and ES
values, fifteen stack offsets at both segment ends, A20 wrap of the
stack, DS and buffer, buffer offset wrap, the buffer under the frame, the
pointer, party byte, map byte and tables inside the frame with DS=SS, a
random background, a push-placed frame at the segment top and preserved
incoming flags. All 49 instruction starts and all 18 conditional outcomes
are reached. The folded entries run as direct targets with the same
fixture families plus every selector and map combination: **3,268 cases**
each, all 120/120 and 103/103 instruction starts and all 24/24 and 22/22
conditional outcomes, no mask.

The DOSBox-X normal-core/386 synthetic oracle passes **150 cases** for
this body (and 150 each for `+01AD` and the two folded entries in the same
run), comparing all 14 CPU words and 128 frame bytes. The COM copies the
original unit 0x33 span, the original unit 0x3BD span and two loaded-state
far jumps in one stub paragraph; the two segment words are relocated by
the paragraph arithmetic the loader applies to those fixups. No mask is
applied to `+012B` or `+01AD`; 66 direct entry 2/3 cases differ only in
the undefined AF bit after `SHR`, which DOSBox-X sets and the model, like
Unicorn, leaves clear. Maskable interrupts are disabled from frame priming
through capture. The result SHA-256 is
`33a51d0d80b6e6476ec30ae35c532718d860b8a62f4431a0b4aa6bd8d29e3d05` with
COM segment `0814`; another DOS allocation changes the hash while
retaining parity. Original bytes, generated assembly/COM, results and logs
stay under ignored `resources/`; public fixtures are synthetic.

```sh
cmake -S . -B resources/build-u33-012b -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u33-012b --target \
  poolrad_overlay_u33_cell_lookup_012b_tests poolrad_overlay_u33_cell_pair_01ad_tests
ctest --test-dir resources/build-u33-012b \
  -R 'poolrad_overlay_u33_cell_(lookup_012b|pair_01ad)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1200s \
  python3 tools/overlay/u33_cell_lookup_012b_audit.py --unicorn --native \
  --output resources/u33-012b-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+012B..+01AC` with entries 3 and 2 of unit `0x3BD` composed |
| `source_candidates` | `src/recovered/overlay_u33_cell_lookup_012b.c`, `src/recovered/overlay_u3bd_lookup_fullstate.c` |
| `signals` | Exact hash/CFG, two aligned callers in the translated `+01AD`, the documented party/map/buffer globals, whole-RAM/CPU differential with full instruction and branch coverage of the body and both folded entries, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Loaded-state stubs only; natural saved-game reach unmeasured; AF after `SHR` undefined and masked only on the direct folded-entry native cases |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.
