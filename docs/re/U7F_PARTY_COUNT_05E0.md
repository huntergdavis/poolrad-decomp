# Unit 0x7F `+0x05E0`: party status count

`pr_u7f_party_count_05e0` translates the complete 90-byte, call-free
`GAME.OVR` body at unit `0x7F:+0x05E0..+0x0639`. It traverses the party
far-pointer list and returns in `AL` the number of nodes whose byte at
`+0x84` is `0` or `0xB3`, modulo 256. No full-C count promotion is claimed
before independent game-identity and parity review.

## Provenance and ownership

| Item | Measured value |
| --- | --- |
| `GAME.OVR` | 206,105 bytes, SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit code base | file `0x24C89` (150,665), as checked in [U7F_DIV5_ENTRY10.md](U7F_DIV5_ENTRY10.md) |
| Exact function | file `[0x25269,0x252C3)`, 90 bytes, SHA-256 `d4e73837529d5f4bb1ac788885f216ba39ccf5f67778a8278862f846c752a969` |
| Decode | 30 aligned instructions; all branches internal; no calls; plain `RETF` at `+0x0639` |
| Classifier | `0x7f:0x05e0`, grade B, `game_evidenced`, provisional, party, 90 decoded/90 bound |

The required `deja "Pool of Radiance GAME.OVR unit 0x7F +0x05E0 party routine callers frame parity"` query was run before byte analysis. It returned no result and was interrupted after waiting; no session material was reused. The documented unit base and game party-head identity in [U7F_DIV5_ENTRY10.md](U7F_DIV5_ENTRY10.md), [overlay_classification.csv](overlay_classification.csv), and [PR #380](https://github.com/huntergdavis/poolrad-decomp/pull/380) were reused, then checked against the pinned bytes.

The incoming near call is `+0x0652`, inside public unit `0x7F` entry 3
at `+0x063A`: `PUSH CS; CALL +0x05E0`, then `MOV [BP-9],AL`. The ledger
records one incoming near caller, and a raw call-target scan found this
one aligned site. The ledger row's `2` counts DS globals and its `4`
counts ES references inside the leaf; neither is an incoming caller count.
Three literal far calls
to the public entry-3 stub `0x8F:0x002F` occur in the pinned overlay at
file `0x536B`, `0x65E8`, and `0x7337`, inside unit `0x1E` public entry 1,
unit `0x20:+0x0ECD`, and unit `0x23` public entry 1 respectively. Those
callers are classified as game evidenced and use the active-character
state. This chain supports game ownership, while direct native reachability
of this leaf remains unmeasured.

## Frame and effects

The routine reads the party head from `DS:[0x5D96]` and `DS:[0x5D98]`.
Its current far pointer is stored at `SS:[BP-5]` and `[BP-3]`, and its
byte counter at `[BP-6]`. Each non-null iteration executes `LES DI` from
the local pointer. A zero status byte at `ES:[DI+0x84]` increments the
counter. A nonzero byte causes another `LES DI` and a second status read;
`0xB3` also increments. A final `LES DI` reads the next far link from
`ES:[DI+0x104]` and `+0x106`. These reads are kept in source order, so
aliases through the raw 1 MiB RAM remain observable. A cyclic list does
not return, as in the original.

On the null-link exit, `OR AX,[BP-3]` leaves `AX=0` before the count is
copied through scratch byte `[BP-1]` to `AL`. `AH=0`; `DX` is the last
loaded next-link segment. `ES:DI` remains the last visited node, or its
incoming value for an empty list. The return flags come from the final
`OR`: CF/OF=0, ZF/PF=1, SF=0, and AF=0, as both Unicorn and DOSBox-X leave
it after `OR`. Until 2026-10-18 the model kept the incoming AF instead; the
designed sweep below caught this (flags `0656h` against the original's
`0646h` when the entry flags had AF set), and the model now clears AF. The frame writes saved
`BP`, the two pointer words, count, and scratch byte; `RETF` consumes only
the far return address. A null head returns `AL=0` without changing
`ES:DI`. A node pointer with one zero word is still non-null because the
two words are ORed.

## Verification

`poolrad_overlay_u7f_party_count_05e0_tests` passes with
`-Wall -Wextra -Wpedantic -Werror`. It checks the public API, mixed
status count, return frame, flags, and stack writes.

`tests/recovered/check_overlay_u7f_party_count_05e0.py` hash-locks the
overlay and exact function span, checks the decode and branch targets,
then compares the original bytes in Unicorn against the C model. Fifteen
cases cover empty and single-node status paths, mixed statuses, a
257-node count wrap, node offset wrap, stack and `LES` offset boundaries,
one-word-zero far pointers, a status byte aliasing the live count local,
and an ignored captured party-RAM state. All 1 MiB RAM bytes and all
returned CPU fields agree, masking only the undefined AF bit of the final
`OR`. The captured 640 KiB input has SHA-256
`0b50157ab03676d796b60f6ccda0b73fa17556a5775f49c91154f81faf0b8705`;
the audit supplies a synthetic call frame and overlays the pinned code.
This is captured-state emulation, not a reached native frame.

## Full-state sweep and native oracle (2026-10-18)

The reviewer asked for two missing pieces of evidence: a designed
full-state original-byte sweep, and DOSBox-X entry/return parity. This
section supplies both. There is no AF masking anywhere.

- `test_overlay_u7f_party_count_05e0_sweep.c` (CTest
  `poolrad_overlay_u7f_party_count_05e0_sweep_tests`) passes 14,001
  checks under ASan/UBSan, compiled with `-Wconversion -Wsign-conversion
  -Werror`. It covers 2,000 random lists of 0–13 nodes with status bytes
  around 0 and B3h, and entry flags with AF set and clear. An independent
  count predicts AL, the count and scratch bytes, the nil pointer and the
  exact final flags, and every byte outside the stack window is checked.
- `tools/overlay/u7f_party_count_05e0_audit.py --unicorn` passes **256
  original-byte cases**, comparing the entire 1 MiB RAM and all 14 CPU
  words with no mask. Coverage is 30/30 instruction starts and 6/6
  conditional outcomes. The cases cover:
  - 240 random lists over six flag words, four with AF set;
  - a nil head with AF set, all members counted, none counted, and 12 of
    12 counted;
  - a node whose status byte is the routine's own count byte `[BP-6]`, in
    three variants;
  - stacks near both segment ends, SS across A20 and the party across A20;
  - a party at a segment end, a random background and a relocated entry.

  Counts from 0 to 12 occur. Against the pre-fix model, the same sweep
  fails at its third case on the AF bit alone.
- `tools/overlay/u7f_party_count_05e0_audit.py --native` passes **120
  DOSBox-X normal-core/386 synthetic cases** with no mask.
  - The COM copies unit `0x7F` to paragraph `8800`. For each case it
    builds a 0–12-node list at `2000:0100+200h·k` with the head at
    `1900:5D96`, then far-calls `+05E0` with IRQs masked.
  - Each case compares all 14 CPU words and a 64-byte stack window.
  - 100 of the cases enter with AF set, and all return with AF clear.
  - The result SHA-256 is
    `21c1e9d0b27b2f61951ceb310f0c44908987d813921b6585c9843472a9fb78b5`,
    with COM segment `0814`.

  This is synthetic native entry/return parity, not a reached saved-game
  frame.

## Bounded native route attempt

An ignored DOSBox-X replay under `resources/oracle-runs/u7f-05e0-native-001`
used the same pinned `START.EXE` (SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`),
`GAME.OVR` above, and slot-A save (SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`).
The heavy debugger binary had SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The replay loaded slot A at `(15,4,N)`, inspected character view, items,
trade, and encamp, then followed the nearby eastern doorway. That doorway
led to an area exit, not to a treasure or shop share menu. A debugger stop
at runtime `2467:053E` was rejected as a unit-`0x23` identity: its live
bytes start with `JBE`, whereas original unit `0x23:+0x053E` starts with a
prologue. No debugger code or script patch was made, and no native
`+0x0652` or `+0x05E0` frame was captured. Private screenshots and media
remain ignored. The saved route does not establish natural reachability;
the PR remains explicitly provisional.

Local command: `resources/venv-u9b-0255/bin/python
tests/recovered/check_overlay_u7f_party_count_05e0.py`. Original media,
captures, and the local Python environment stay under ignored `resources/`.

## Parity review

| Field | Finding |
| --- | --- |
| `target_function` | `GAME.OVR` unit `0x7F:+0x05E0` complete routine |
| `source_candidates` | `src/recovered/overlay_u7f_party_count_05e0.c`; no competing same-address implementation found |
| `signals` | exact byte span and extent; 30-instruction control flow; party head/link/status offsets; incoming entry-3 call; full-RAM original-byte parity with no mask (designed 256-case sweep plus the captured party state); DOSBox-X synthetic entry/return parity (120 cases); independent C sweep |
| `score` | code parity strong; game identity supported by caller chain but still provisional |
| `verdict` | `match` for the pinned code behavior; `likely_match` for game ownership pending a reached native frame or independent reviewer judgment |
| `review_notes` | synthetic native entry/return parity now exists, but natural saved-route reach is still unmeasured; the AF fix of 2026-10-18 removed the only mask; the verifying lane decides identity and strict credit |

The caller paths in units `0x1E`, `0x20`, and `0x23` are not translated by
this change. A native trace at `+05E0` would close the remaining identity
gap; the original-byte audit closes the local code-parity question.
