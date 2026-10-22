# Unit 0x33 +01AD cell pair

`pr_u33_cell_pair_01ad` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+01AD..+0220` and composes its two near calls to
`+012B`, the [cell lookup](U33_CELL_LOOKUP_012B.md), which in turn
reaches unit `0x3BD` entries 3 and 2. The recall attempt, branch search,
folded-entry provenance and reused methods are recorded in the `+012B`
note.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x33 | GAME file base `0xC5D1`, stub segment `0x003B`; `+01AD` is an internal prologue |
| Function | GAME file `0xC77E..0xC7F1`; **116 bytes, 54 instructions, no conditional branch, two near calls** |
| Function SHA-256 | `6250ebfdce5344654824482798dfad959558058bf0dbb53d9de6e1852e4835db` |
| Near calls | `+01D3` and `+0209`, each `PUSH CS; CALL +012B` |
| Exit | `RETF 6` at `+021E`; exact end `+0221` exclusive |

The audit decodes each unit-0x33 routine from its own prologue and finds
exactly nine aligned `PUSH CS; CALL +01AD` sites in four routines:
`+040F` (`+042B`, `+0448`), `+0615` (`+0631`, `+064D`), `+0810` (`+0852`,
`+0864`, `+0876`) and `+1577` (`+1894`, `+1950`). The body reads the
per-facing step tables at DS `2736` and `273F` that the unit 0x3BD view
evidence records as the facing step bytes, and through `+012B` the party
y byte, map id and far buffer named in the `+012B` note. No gameplay
meaning is inferred for the OR of the two lookups; "cell pair" names the
two cells it consults.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS, `[SP+4]` facing, `[SP+6]` y and `[SP+8]` x, each read as its
low byte; `RETF 6` leaves SP ten bytes above entry. The routine pushes BP
and reserves four bytes: opposite facing `[BP-4]`, second result `[BP-3]`,
first result `[BP-2]` and the returned byte `[BP-1]`.

It zero-extends facing, adds 4, sign-extends into DX:AX with `CWD`,
divides by CX = 8 with signed `IDIV`, exchanges so AX holds the
remainder, and stores its low byte as the opposite facing. (The XOR and
ADD flags are overwritten inside the first call; `IDIV` leaves the flags
alone in both tested engines.) It pushes x, y, facing and its own BP as
live AX words and calls `+012B`, storing AL. It then forms
`sign(x) + sign(DS:[2736+facing])` and `sign(y) + sign(DS:[273F+facing])`
with facing zero-extended into DI, pushes those words, the opposite
facing and its BP, calls `+012B` again and stores AL. The two result
bytes are ORed through `[BP-1]` into AL with the OR's flags (undefined
AF as zero, which both engines produce), and the live frame is restored.

Both near calls push CS and the return offset relative to the entry
`cpu->ip` (`01AD` in the game) and enter the `+012B` model at `ip - 82`;
that model reads its own relocated far-call operands and stubs from RAM
as its note describes, so `CODE` and `TRANSFER` surface here unchanged.
DX, CX, ES and DI return as the last lookup left them. Offsets wrap at
16 bits; words and return frames are physically contiguous; physical
addresses wrap at 1 MiB. Null RAM/CPU or less than 1 MiB is rejected
before mutation. No loop exists, so every input returns.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`) checks a pair
whose first cell returns 1 and second 0 (opposite facing 6, both result
locals, registers and frame), a pair returning 3 and 3, an off-grid pair
with facing 5, a corrupted stub (`CODE`) and the host guards.

The original-byte oracle passes **1,212 returning cases** for this body
with the same fixture families as `+012B` (coordinate grid with eight
facings, edge coordinates, 240 random cases, stack ends, A20, offset
wrap, buffer and global aliases inside the frame, random background,
push-placed frame, preserved flags), each comparing the entire 1 MiB RAM
and all 14 CPU words with no flag mask; all 54 instruction starts are
reached. The DOSBox-X normal-core/386 synthetic oracle passes **150
cases** for this body in the shared run described in the `+012B` note
(result SHA-256
`33a51d0d80b6e6476ec30ae35c532718d860b8a62f4431a0b4aa6bd8d29e3d05`, COM
segment `0814`), comparing all 14 CPU words and 128 frame bytes with no
mask. The reproduce commands are in the `+012B` note; the focused test
target is `poolrad_overlay_u33_cell_pair_01ad_tests`.

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+01AD..+0220` with `+012B` and its lookups composed |
| `source_candidates` | `src/recovered/overlay_u33_cell_pair_01ad.c` |
| `signals` | Exact hash/CFG, nine aligned callers, documented step tables and cell globals, whole-RAM/CPU differential with full instruction coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Loaded-state stubs only; natural saved-game reach unmeasured; the four callers remain unresolved identities |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.

## Reproduce

Added 2026-10-22 by par-e (VP i21177). Until then this document described its checks but
gave no command to rerun them. The evidence kind is recorded in
[credit_evidence_kind.csv](credit_evidence_kind.csv) (labels: [CREDIT_EVIDENCE_KIND.md](CREDIT_EVIDENCE_KIND.md)).

Original-byte differential. The script runs the credited C as a direct target and compares
it with the original DOS bytes under Unicorn. It needs the private media under ignored
`resources/`.

```sh
python3 tools/overlay/u33_cell_lookup_012b_audit.py --unicorn
```

- `tools/overlay/u33_cell_lookup_012b_audit.py` covers target `01ad` (`pr_u33_cell_pair_01ad`). Measured on `ee07bd68`: +01ad: 1212 original-byte cases passed with the full 1 MiB RAM and all 14 CPU words equal; 54/54 instruction starts; 0/0 conditional outcomes (the body has no conditionals).

Synthetic C tests. These are hand-derived checks of the C, with no DOS bytes, so they are not
an original-byte differential:

```sh
cmake -S . -B build
cmake --build build --target poolrad_overlay_u33_cell_pair_01ad_tests
ctest --test-dir build -R '^poolrad_overlay_u33_cell_pair_01ad_tests$' --output-on-failure
```

| Credit | Exercised by |
| --- | --- |
| `OVL_0033_01AD` | the whole `poolrad_overlay_u33_cell_pair_01ad_tests` executable |

Measured on `ee07bd68`: `poolrad_overlay_u33_cell_pair_01ad_tests` passed (1/1 under ctest).
