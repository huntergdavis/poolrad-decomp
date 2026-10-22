# Unit 0x33 +0377 stamp square

`pr_u33_stamp_square_0377` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+0377..+040E` and composes its eight near-call sites
to the merged [`+0221` clipped writer](U33_CLIPPED_WRITER_0221.md). Four of
them run on each path. It writes a 2 by 2 square of glyphs at positions
relative to its caller's frame. Its sibling, conventions and validation
setup are those of [`+02BA`](U33_STAMP_BLOCK_02BA.md).

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE / GAME.OVR | As pinned in [U33_STAMP_BLOCK_02BA.md](U33_STAMP_BLOCK_02BA.md) |
| Function | GAME file `0xC948..0xC9DF`; **152 bytes, 82 instructions, one conditional branch, eight near-call sites** |
| Function SHA-256 | `aa813423109dca49427278a990c14f33e65a2a320fa1089a6e29680c2ca866a3` |
| Near calls | `+0394`, `+03A4`, `+03B4`, `+03C4`, `+03D6`, `+03E6`, `+03F6` and `+0406` `CALL +0221`, each preceded by `PUSH CS` |
| Exit | `RETF 2` at `+040C`; exact end `+040F` exclusive |

Every branch target lies inside the body, and `+040F` is the next
prologue. The only aligned `PUSH CS; CALL +0377` is `+0883` inside
`+0810`.

## Complete ABI and ordered effects

The ABI is `+02BA`'s: `[SP+4]` is the caller's frame offset L, and the
body returns with `RETF 2`. It pushes BP and reserves two bytes that it
never reads. `MOV DI,[BP+6]` and `CMP BYTE SS:[DI-7],1` choose the
pattern:

- **Selector 1:** glyph 5 at (row 0, columns 3 and 4) and glyph 0Ah at
  (row 1, columns 3 and 4).
- **Otherwise:** glyph 16h at the same four positions.

Each stamp goes through +0221 exactly as in `+02BA`, with the live AH in the
pushed words. The final flags are the last +0221 call's `ADD` or clip
`CMP`. Each near call enters the +0221 model at ip − 156h and must return
to its frame, else `TRANSFER`.

## Validation and parity review

The focused C11 test checks both patterns cell by cell, the four-cell
extent and the host guards.

The original-byte oracle passes **158 returning cases** with the same
families as `+02BA`, including a grid laid over the caller's frame so the
first stamp rewrites `SS:[L-2]`. Each case compares the entire RAM and all
14 CPU words with no mask. All 82 instruction starts and both conditional
outcomes are reached.

The DOSBox-X synthetic oracle passes **40 cases** for this body in the shared
run. The result SHA-256 and the commands are in
[U33_STAMP_BLOCK_02BA.md](U33_STAMP_BLOCK_02BA.md).

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+0377..+040E` with `+0221` composed at eight sites |
| `source_candidates` | `src/recovered/overlay_u33_stamp_square_0377.c` |
| `signals` | Exact hash/CFG, one aligned caller, whole-RAM/CPU differential with full coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `+0221` strict credit pending; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.

## Reproduce

Added 2026-10-22 by par-e (VP i21177). Until then this document described its checks but
gave no command to rerun them. The evidence kind is recorded in
[credit_evidence_kind.csv](credit_evidence_kind.csv) (labels: [CREDIT_EVIDENCE_KIND.md](CREDIT_EVIDENCE_KIND.md)).

Original-byte differential. The script runs the credited C as a direct target and compares
it with the original DOS bytes under Unicorn. It needs the private media under ignored
`resources/`.

```sh
python3 tools/overlay/u33_stamp_02ba_audit.py --unicorn
```

- `tools/overlay/u33_stamp_02ba_audit.py` covers target `0377` (`pr_u33_stamp_square_0377`). Measured on `ee07bd68`: +0377: 158 original-byte cases passed with full RAM and CPU equal; 82/82 instruction starts; 2/2 conditional outcomes.

Synthetic C tests. These are hand-derived checks of the C, with no DOS bytes, so they are not
an original-byte differential:

```sh
cmake -S . -B build
cmake --build build --target poolrad_overlay_u33_stamp_square_0377_tests
ctest --test-dir build -R '^poolrad_overlay_u33_stamp_square_0377_tests$' --output-on-failure
```

| Credit | Exercised by |
| --- | --- |
| `OVL_0033_0377` | the whole `poolrad_overlay_u33_stamp_square_0377_tests` executable |

Measured on `ee07bd68`: `poolrad_overlay_u33_stamp_square_0377_tests` passed (1/1 under ctest).
