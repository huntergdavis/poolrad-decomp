# U9E public entry 16: byte result helper at +10E4

## Publication refresh (2026-10-17)

This additive publication reuses archived branch codex/fullc-u9e-10e4-entry16
at bcdb0c34629e11dd6fc8aa48fc04b4d374be4289. The C implementation is unchanged.
It is rebased onto current main; the CMake integration preserves every
existing registration and adds only this source and focused test.
The original branch was already pushed but had no PR. No superseded
implementation was found in current main.

The historical evidence below is retained for provenance. Fresh original-byte,
native, strict C11/ASan/UBSan and CMake/CTest results are recorded in the PR
and ignored resources/publication/ logs. Earlier whole-suite counts below
describe the archived run, not a new whole-suite claim.

The pre-publication deja query timed out without a reusable result. Reuse
is attributed to the Git head above. Independent review and credit remain
with the reviewer/VP; this publication does not change acceptance,
classification, or accounting. Native cases are synthetic selected-memory
captures, distinct from the original-byte whole-RAM comparisons. Native
coverage applies only to the listed frames and is not a general proof of
every segmented-memory boundary. CPU storage must be separate from RAM;
the code is immutable, execution synchronous, and hardware faults/devices
are outside the flat A20-disabled RAM profile.

`pr_u9e_entry16_10e4` translates all of DOS v1.3 `GAME.OVR` unit `0x9E`
public entry 16. It takes two word arguments on a far-call stack frame, uses
the low byte of the second to compute a default result, and returns
`low(first)+1` when the low byte of the second is `0x12`. The default is
`low(second)+100`. The 16-bit `AX` can carry into `AH`; `RETF 4` removes both
arguments. The final CPU flags are the original `CMP` or `INC AX` flags.

## Source and scope

`deja "Poolrad U9E entry 16 +10E4 leaf full C parity 41 bytes"` timed out
locally. The address and caller inventory come from the checked-in
[overlay function ledger](overlay_function_ledger.csv); the 1 MiB CPU/RAM
parity layout follows the nearby [U9E entry 9 audit](U9E_ENTRY9_ROLL_LATCH_0E2E.md).

| Evidence | Value |
| --- | --- |
| Original `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Entry 16 body | file `[0x2BFCE,0x2BFF7)`, U9E `+10E4..+110C`; 41 bytes, 17 instructions; SHA-256 `12ce30806a561605b7f7be238444a28a0d22f7cc53bda4872d6e0800330f41db` |
| Identity | public entry 16; ledger grade A, one far and three near call references; no calls inside; `RETF 4` at `+110A` |

The three near-call opcodes resolving to `+10E4` are at U9E `+1199`,
`+1220`, and `+1271`, all within the following `+115C` routine.

The translation executes the complete body from `CS:10E4` through the far
return, including the saved `BP`, two-byte local, wrap-capable stack reads,
full `AX`, final arithmetic flags and caller `CS:IP`. It has no callback or
host approximation. Invalid API inputs return before memory mutation; that
host guard is outside DOS parity.

`tests/recovered/check_overlay_u9e_entry16_10e4.py` verifies the exact
original slice and compares the C model to 16-bit Unicorn at the far-return
target. It compares all 1 MiB RAM bytes and all 14 CPU words across 72
synthetic entry frames: five first argument bytes, seven second argument
bytes, both paths, normal and wrapped stack layouts, two flag patterns, and
directed SP `FFFD` frames for the two branch outcomes.
It requires Python `capstone` and `unicorn`; they may be installed under
ignored `resources/` and supplied through `PYTHONPATH`.
`tests/recovered/test_overlay_u9e_entry16_10e4.c` pins both result paths,
wrapped stack behavior and API rejection under strict C11 ASan/UBSan.

The same Unicorn setup misses the intended far-return target at SP `FFFE`
and `FFFF` with first byte `FF`, second byte `11` or `12`, SS `E000`, and
FLAGS `0002`. SP `FFFD` passes for both outcomes. This is an oracle limit for
those stack placements; it is not evidence that the C return is wrong.

`tools/overlay/u9e_entry16_10e4_native.py` runs the exact original 41-byte
body in synthetic DOSBox-X COM frames at SP `FFFE` and `FFFF`, with second
byte `11` and `12`. The normal-core 386 executable is 12,192,432 bytes,
SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`.
All four native returns match the C translation for all 14 CPU words and
25 relevant stack bytes, including the byte physically beyond SS:`FFFF`.
The harness checks the COM's loaded body against the original hash; all COM,
body and native result files remain under ignored `resources/`.

These are synthetic frames. Natural native reach remains provisional: no
saved-game caller frame has been captured for entry 16. Function accounting
remains with PM review.

## Adversarial parity review

| Field | Assessment |
| --- | --- |
| `target_function` | Original U9E `+10E4..+110C`, public entry 16 |
| `source_candidates` | `pr_u9e_entry16_10e4` |
| `signals` | Exact byte extent and hash; branch and arithmetic shape; stack and `RETF 4` effects; 72 full-state Unicorn comparisons; four native wrap comparisons |
| `score` | Five corroborating signals; no contradiction in the tested frames |
| `verdict` | `match` for the complete executable body under the tested synthetic frames |
| `review_notes` | Unicorn cannot decide SP `FFFE`/`FFFF` here; DOSBox-X covers those placements. Native saved-game reach remains unmeasured. The native oracle compares all CPU words and affected stack bytes, while the Unicorn oracle compares full RAM. |

## Explicit CF := 0 at +10FE (t44 follow-up, 2026-10-23)

The model now writes the carry left by `XOR AH,AH` at `+10FE` explicitly. Before this change, the folded value happened
to be right, as explained below.

- **Why it matters.** On the `[BP+8] = 12h` arm, `XOR AH,AH` leaves CF = 0. The following `INC AX` keeps CF, so that
  CF reaches `RETF 4` (`flag_fold_liveness.py`; [FLAG_FOLDED_SLICE6.md](FLAG_FOLDED_SLICE6.md)).
- **Why the old value was already correct.** The arm runs only when `CMP BYTE [BP+8],12h` at `+10F5` compared equal,
  and an equal compare leaves CF = 0. The model carried that CF through `inc16_ax`, so the returned FLAGS were already
  right.
- **The change.** `cpu->flags &= ~CF` is now written before `inc16_ax`, with that reasoning in the comment, as the PM
  asked (i21812). The output is unchanged.
- **Census.** The census row for the function's inline flag write is classified by hand against pinned 5a9c7c53:
  `XOR`'s CF is false, and the lazy and stored values agree.

Re-run on this change through `poolrad-heavy.sh`:

```sh
python3 tests/recovered/check_overlay_u9e_entry16_10e4.py   # 72 original-byte cases PASS, full RAM and 14 registers
python3 tools/overlay/u9e_entry16_10e4_native.py           # DOSBox-X: PASS
```

There is no C-test claim. The model compiles under `-Wconversion -Wsign-conversion -Wshadow -Werror`, and the build
has asserts on (no `CMAKE_BUILD_TYPE`, no `-DNDEBUG`).
