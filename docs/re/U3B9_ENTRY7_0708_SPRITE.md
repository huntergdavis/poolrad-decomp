# U3B9 entry 7 (+0x0708): range-checked record-slot sprite draw

This note covers GAME.OVR unit `0x3B9` + `0x0708`, public entry 7, as a whole
function. The run-time stub is `0x3D0:0x43`.

- **Status.** The model is source-only and path-complete, with explicit
  callbacks. It is uncompiled under the host hold and not counted.
- **Ownership.** Cleared before drafting (PM i12649): relative CLEAR from p5,
  pF, p9, pV, pP, pN, pG and p6.

| Input | SHA-256 |
| --- | --- |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit `0x3B9` code (1,989 bytes, file `0x2E0CE`) | `fd883756075e2fd9e6269cc6c262ec8b7870b13d2ee5d115cb307176bf387892` |
| Body `[0708,07C0)`, 184 bytes, file `0x2E7D6` | `e86596a8c7b47ea2dd6ff9a00a8419242be12e2f1ae306605745938e10eb7a93` |
| Private CS literal pool `[06E9,0708)`, one 30-character string | `21de157c48f6fe8866352b63b056d43f1ddf1e49269b6ff33433863f0444adaa` |

The literal pool is loaded only by this function, at `+0737`. The checker
asserts that `mov di,0x06E9` occurs exactly once in the unit.

## Shape

- **Decoding.** Recursive descent reaches all 74 instruction starts and all
  184 bytes, in eight basic blocks with no loop.
- **Frame.** `sub sp,0x46` sets up the locals: a far-pointer temporary at
  `BP-0x46`/`BP-0x44`, then the 0x42-byte value copy of the record at
  `BP-0x42`.
- **Arguments.** The far record pointer is `[BP+6]` and the index byte is
  `[BP+0xA]`; `retf 6`; no result.

| Offset | Behavior |
| --- | --- |
| `+070E` | `0xAF8:0x0A3F` copies 0x42 bytes from `[BP+6]` to `BP-0x42` |
| `+0721`, `+0727` | Two separate reads of `[BP+0xA]`: below 1 → `+072D`; at most 3 → `+074E` |
| `+072D` | Resident `0x464:0x00FF`; Write(Text at `DS:0x8608`, CS `0x06E9`, width 0); Writeln; resident `0x899:0x0000`; falls through to `+074E` if that returns |
| `+074E` | `di := index << 3`; the words `[BP+di-0x44]` and `[BP+di-0x42]` are both zero → `retf` |
| `+0761` | Temporary := that far pointer p |
| `+0775..+078B` | `x = p^[4] + 2`, `y = p^[6] + 2` (16-bit) |
| `+078D..+07AC` | Resident `0x464:0x0D71`(x, y, k = AL 0, flags = AL 1, S = slot re-read, D = `DS:[0x4A22]:[0x4A20]`). AH in both the k and flags words is y's high byte |
| `+07B5` | Resident `0x464:0x1B52`, then `retf 6` |

## Callers and data

- **Callers.** Unit `0x25` + `0x054E`, the merged caller model (p5), calls
  this entry at `+05D5` and `+05E4`. Both push the index word, then the far
  pointer `DS:0x6ABE`, and call `0x3D0:0x43`. There is no procedure-pointer
  constant to the entry.
- **Draw-buffer pointer.** In the 815 decoded game-tier bodies scanned for
  this screen, `DS:0x4A20/0x4A22` is a far draw-buffer pointer:
  - its address is taken by unit `0x36` + `0x002D`;
  - about twenty functions push it;
  - `0x464:0x1B52` loads it.

  This entry only reads it. The statement is bounded to that scan; no global
  write-exclusion audit was done.

## Model

- **Files.** `include/poolrad/recovered/overlay_u3b9_entry7_0708.h` and
  `src/recovered/overlay_u3b9_entry7_0708.c`, entry point
  `pr_u3b9_0708_show(frame, state, message, hooks, out)`.
- **Live frame.** The frame (arguments plus the 0x46-byte locals image) is a
  mutable structure passed to every callback. Each original re-read of
  `[BP+0xA]`, of the slot words and of the temporary is a re-read of that
  structure: `+0721`, `+0727`, `+074E`, `+0761`, `+0775`, `+0781` and
  `+0793`. A callback that aliases the represented frame therefore changes
  the model the way it would change the original. This is bounded to the
  represented arguments, locals and DS state; physical DS/SS aliases, CPU
  flags, the full AX and DF are not represented. The tests exercise this for
  the copy and for `0x899:0x0000`. This is the live-frame point raised in the
  independent entry-6 review (pN, i12651).
- **Out-of-range index.** Index 0..8 is modelled from the locals image. Index
  0 reads the temporary's segment word, which the host supplies as the
  uninitialized-stack content. Index i > 8 selects the words at
  `BP+(8i-0x44)` and `BP+(8i-0x42)`:
  - index 9 reads the return CS (`BP+4`) and the record-offset argument
    (`BP+6`);
  - index 10 and above read stack beyond the explicit arguments;
  - no index selects the saved BP (`BP+0`) or the return IP (`BP+2`).

  The model carries no upper-stack image. It stops with `UNMODELED_FRAME`
  whenever a re-read index exceeds 8: either after `0x899:0x0000` returns,
  or after a callback (including a far-word read before `+0793`) changed the
  live index. This stop is a host boundary, not an original branch
  (independent review pN i12741, finding F1).
- **Untested overlap.** Index 0 with a non-nil slot overlaps the temporary
  written at `+076F`/`+0772` with the slot words re-read at `+079E`. The code
  keeps that overlap, but no test covers it. The read-hook mutation and the
  non-returning `0x899:0x0000` path are also not exercised.
- **Callbacks.** These are explicit callbacks: the copy, `0x464:0x00FF`,
  Write, Writeln, `0x899:0x0000`, the two far word reads, `0x464:0x0D71` and
  `0x464:0x1B52`. The `0x0D71` words are passed in push order
  (x first). This follows the unmerged source-only contracts of p6
  (i12642) and pV (i12632), not verified parity.
- **Literal and stops.** The 30-character literal is supplied by the caller,
  length-checked, and pinned only by hash. A failed or missing reached
  callback stops at its exact source offset.
- **Tests.** `tests/recovered/test_overlay_u3b9_entry7_0708.c` uses a
  synthetic literal and covers:
  - a draw from slot 2, with all eight `0x0D71` words, D, the temporary
    write, and AH carried into k/flags;
  - a nil slot, and the index word's high byte ignored;
  - the error path then slot 5;
  - index 0 through the locals image;
  - index 9 giving `UNMODELED_FRAME`;
  - aliasing callbacks;
  - x wrap;
  - copy, read and draw failures;
  - missing hooks, reached versus unreached;
  - argument checks.
- **Byte checker.** `tests/recovered/check_overlay_u3b9_entry7_0708.py`
  passed 32/32 checks against the owner-held GAME.OVR, and without media it
  prints the contract and skips. It pins:
  - the unit, body and pool hashes, the literal length and its single load;
  - the seven far-call operands;
  - 18 instruction-byte groups;
  - both U25 caller corridors.

## Parity limits and domain

- **Uncompiled.** The model is uncompiled and unrun: no compiler, native or
  emulator run. `tests/recovered/build_u3b9_0708.sh` is the strict build for
  when the hold lifts.
- **Bounded domain.** The match claim is limited to this domain:
  - immutable, correct literal bytes;
  - callbacks that honour the stated word ABIs;
  - host-supplied initial locals for index 0;
  - DS state read at `+07A4`/`+07A8`.
- **Not modelled.** CPU flags, the full AX (beyond the AH that reaches the
  pushed k/flags words), DF on return, the stack image above the locals,
  and SYSTEM/resident side effects all stay external.
- **Error-path continuation.** Whether `0x899:0x0000` returns is not
  asserted; the model covers both outcomes.
- **Unmeasured reach.** Natural saved-game reach is unmeasured.

## Proposed registration (not applied)

Following the merged U6F `+01B8` pattern:

1. Add `src/recovered/overlay_u3b9_entry7_0708.c` to the `poolrad_recovered`
   source list.
2. Add the test target:

```cmake
    add_executable(poolrad_overlay_u3b9_entry7_0708_tests
                   tests/recovered/test_overlay_u3b9_entry7_0708.c)
    target_link_libraries(poolrad_overlay_u3b9_entry7_0708_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_overlay_u3b9_entry7_0708_tests
             COMMAND poolrad_overlay_u3b9_entry7_0708_tests)
```

The README and `FUNCTION_LEDGER.md` rows are deferred to restack with the
other U3B9 rows.

## CPU model (strict lane, uncompiled)

Files: `include/poolrad/recovered/overlay_u3b9_entry7_0708_cpu.h` and
`src/recovered/overlay_u3b9_entry7_0708_cpu.c`, entry point
`pr_u3b9_0708_cpu_run(ram, ram_size, cpu, hooks)`. It follows the
full-state precedents: 1 MiB A20-off RAM and a 14-register CPU. The whole
function runs on live state.

- **Live RAM.** Every push, re-read of `[BP+0xA]`, slot word, temporary,
  far word and `DS:[0x4A20]` goes through RAM. Indexes above 8 read whatever
  the stack holds, so the abstract model's `UNMODELED_FRAME` stop does not
  arise here.
- **Folded copy.** SYSTEM `+0A3F` (the value-parameter copy) is folded
  exactly from its decoded body at the live call segment. It writes no
  arithmetic flags and sets DF = 0.
- **External hooks.** Six callees are hooks, each receiving the full CPU and
  RAM after the real pushes and return push: `0x464:+00FF` (it reaches the
  DOS-unit Intr through `+00CF`), SYSTEM `+0919` Write, SYSTEM `+084A`
  Writeln, `0x899:+0000` (it reaches SYSTEM `+00E9`), `0x464:+0D71` and
  `0x464:+1B52`. Execution continues only at the pushed return CS:IP. A
  non-zero hook result covers both a non-returning callee and a host
  failure.
- **Undefined flags.** OF after `SHL DI,3` is written as MSB(result) XOR CF.
  AF after SHL/XOR/OR is written as 0. The third SHL's flags reach the
  `0x0D71` hook.
- **Safety.** These are the same as the entry-6 CPU model: up-front write
  alias screen (`SS:[SP-0x5C,SP)`), folded-byte snapshot, and a
  post-callee re-screen.
- **Unresolved:**
  - all effects of the six hooked callees;
  - the undefined-flag conventions;
  - the overlay stub dispatch;
  - no compile, native or oracle evidence.
