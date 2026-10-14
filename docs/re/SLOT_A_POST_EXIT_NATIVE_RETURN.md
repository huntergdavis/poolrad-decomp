# Saved slot-A Up: direct post-EXIT caller and entry-1 return

One Up from the unmodified saved slot-A playable view reached unit
`0x1C:345B` after the first ECL `EXIT`, then `+348D`, `+3492`, `+3497`,
`+34CF`, `+34D8`, and `+34E7` in one native run. The loaded caller segment
was `3582`, DS `14A0`, window 0 `3996:0000`, and SS `1D16`. The debugger
stopped before each indicated instruction or call. The game media, full RAM
dumps, and screenshots stay in ignored `resources/`; this note contains only
addresses, hashes, counts, and numeric state.

The earlier [caller-prefix note](SLOT_A_CALLER_TO_ENTRY1.md) had only a
projection from an upstream AND stop. This run directly verifies that prefix
and separates entry 1, view, resident, and caller effects. Reused the
[unit-1C loop map](U1C_LOOP_3387.md),
[unit-68 coordinate-writer map](U68_COORDINATE_WRITER_073B.md), and
[first EXIT boundary](SLOT_A_FIRST_EXIT_9974.md). A `deja` recall query for
this direct return gate was attempted before tracing but did not return; those
repo notes were rechecked against the live stops.

## Original bytes and stop identities

The ignored original `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x1C` begins at file offset `3398`. Original
`+345B..+3491` is 55 bytes, SHA-256
`3970e585476250421f70a482f2315571c16ca249bf4395293efbdc14e5475df8`.
All seven live stops had the same loaded 55-byte caller digest,
`785a11da0a7c4bf8328f5865751fe96597ab3a1dc13ff84100b1337d366daf1a`.
The loaded first 50 bytes equal the original caller prefix. The far-call
segment word differs in loaded RAM, as recorded in the earlier note.

Each RAM stop is a private 655,360-byte dump:

| Stop at `3582:` | RAM SHA-256 | Changed RAM bytes from previous stop |
| --- | --- | ---: |
| `345B` | `fa6a4a111160c6116b0303428b6d20552ffa44b7f7d5b67863ceadfe01859299` | input |
| `348D` | `0faaf7d0c7af057339dcafb6af540d29d2ae98749d62c235d499bd1a596a8c13` | 1 |
| `3492` | `eb692580c5952ab6398cbdbe8044a5a71d562df9155047c42e989f771f273a5f` | 6,198 |
| `3497` | `e6445efc550a21a435a96823791dd1cd74b25fb5103560815e6fb56026592bc8` | 832 |
| `34CF` | `e6445efc550a21a435a96823791dd1cd74b25fb5103560815e6fb56026592bc8` | 0 |
| `34D8` | `1e17583b6d78d99e629a59436d201626c01b93aee9f0c45e0f519b14aee82d50` | 95 |
| `34E7` | `04ade382f947462c95505389a6b729ee9d0bf3a4836b807c29316264c28375c2` | 3 |

## Write attribution

| Boundary | Direct observation |
| --- | --- |
| `345B → 348D` | The **only** full-RAM change is physical `0x39B40`, window 0 `+1E0` low byte `1 → 15`. Its high byte and `+1E2` y word remain zero and 4. The reached `442F` and `49FF` gates were zero. |
| `348D → 3492` | Unit `0x68` entry 1 returned. Party `(x,y,facing)` changed `(15,4,6) → (14,4,6)`. Window 0 `+18E` changed `0 → 1`; `+190/+192` stayed `(5,10)`. Comparison words stayed `(15,4)`. The 6,198 full-RAM changes include transient code and display regions, so this table does not attribute every byte to a gameplay field. |
| `3492 → 3497` | The view call returned; selected party, clock, comparison, and gate fields stayed unchanged. Its full-RAM delta is 832 bytes. |
| `3497 → 34CF` | Caller coordinate comparison changed no RAM. It reached the changed-position resident branch because window 0 comparison x was 15 while current party x was 14. |
| `34CF → 34D8` | Resident `0x8AE:00DC` returned; 95 RAM bytes changed, mostly in its transient loaded region. Selected party, clock, comparison, and gate fields stayed unchanged. |
| `34D8 → 34E7` | Exactly three bytes changed: DS `84E1` `0 → 1`, and SS `3EF4..3EF5` became the pushed `0x9975` script argument. DS `84E0` stayed zero. At `+34E7`, DS `49E5` and ECL PC `49ED` were both `0x9975`; the script call had not run. |

At every stop, DS `442E`, `442F`, and `49FF` were zero. The saved window-0
clock words at `+18E/+190/+192` went from `(0,5,10)` to `(1,5,10)` on the
entry-1 call and stayed there through `+34E7`.

## Verification and limit

Run `bash tests/replay/check_slot_a_post_exit_native.sh` with the ignored
`GAME.OVR` and the seven RAM dump paths in the table's order. The audit locks
all input hashes, checks the original and loaded caller bytes, exact full-RAM
delta counts, selected state at each stop, and the `0x9975` pre-call stack
argument. The strict C11 integration test applies the merged
`pr_slot_a_caller_to_entry1` model to a copy of direct `+345B` RAM and compares
**all 655,360 bytes** against direct native `+348D` RAM; it passes. The
existing quick synthetic caller test remains registered in CTest.

This establishes direct C/native parity only for `+345B..+348C`. The
entry-1, view, resident, and script-runner effects are measured at their
separate return boundaries, but are not composed into a full C one-Up replay.
The next boundary is the call at `+34E7` and its return, including the
post-script `[442F]` and `[49FF]` gates. No new full-C routine or complete
one-Up claim follows from this trace.
