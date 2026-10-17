# U3C6 entry 19 (+0x1298): record bytes around entry 10 (CPU model)

This note covers GAME.OVR unit `0x3C6` + `0x1298`, public entry 19, as a
whole function. The run-time stub is `0x3E0:0x7F`.

- **Status.** The CPU/RAM model is source-only. It is uncompiled, uncommitted
  draft and not counted.
- **Ownership.** Separate relative CLEAR from all 18 queried lanes (PM i14419).
  Local source GO: PM i16649. ABI packet: i14315.

| Input | SHA-256 |
| --- | --- |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit `0x3C6` code (4,864 bytes, file 198,458) | `9ce9da56407e068deff83afea3d5f3f91eeb2705630124a1f9c6770cd2eddba6` |
| Body `[1298,12FB)`, 99 bytes, file `0x319D2` | `3829ca603491d960dbbba909fb204b3618f17012f697d75b802e86e7b1fb1ae3` |
| Folded entry 15 `[0C5F,0CB9)`, 90 bytes | `c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41` |
| Folded entry 12 `[0BE7,0C0F)`, 40 bytes | `eb48355d64c585ee95a31739074ede53929cc991ba519157009c7c8e1c9b7ed5` |
| Folded entry 13 `[0C0F,0C37)`, 40 bytes | `a1000b2f6a29a0d74f37a9b631b26844044f2c1a163b2de793672cbc0664395d` |
| Hooked entry 10 `[0996,0AF8)`, 354 bytes | `573e805d9a73efb7c32d2ce4358793831008396348737a186d0933421c9cdb65` |

## Shape

- **Decoding.** 38 instructions in one straight-line block, all 99 bytes
  reached.
- **Frame.** Far Pascal with 8 argument bytes: byte a at `[BP+6]`, byte b at
  `[BP+8]`, far pointer Q at `[BP+0xA]`. `push bp; mov bp,sp` (no locals);
  `pop bp; retf 8`.
- **Callers** (fixup-validated far calls to stub slot 19):
  - unit `0x2F` + `0x0278`, `0x059F` and `0x05D0`;
  - `0x38` + `0x28D9`;
  - `0x5E` + `0x126B`, `0x24A2` and `0x2BF4`;
  - `0x9E` + `0x0F47` and `0x18CC`.

  There are no near callers and no procedure-pointer constants.
- **Callees.** All are `push cs; call near`:
  - entry 15 (`retf 4`), whose result is the slot s of Q;
  - entry 12 (`retf 4`), which returns `[0x5F27 + 4s]`;
  - entry 13 (`retf 4`), which returns `[0x5F28 + 4s]`. Entries 12 and 13
    each call entry 15.
  - entry 10 (`retf 8`), whose four arguments are read as bytes.

  Pushed argument bytes equal each RETF immediate (4, 4, 4, 8).

## Behaviour

R is the far pointer at `DS:0x6716`, re-read through LES at each of its four
uses.

1. `R[4] := a`.
2. `R[5] := [0x5F2A + 4·entry15(Q)]`.
3. Call `entry10(8, b, entry13(Q), entry12(Q))`. The words pushed are
   `0x0008`, `0x00bb`, `0x00YY` and `0x00XX`: every high byte is the 0 left by
   the folds' `xor ah,ah`.
4. `R[4] := 0`, `R[5] := 1`.
5. Return. No instruction after the entry 10 call writes flags, so the
   final flags are the ones entry 10 returns.

## CPU model

Files:
- `include/poolrad/recovered/overlay_u3c6_1298_cpu.h`;
- `src/recovered/overlay_u3c6_1298_cpu.c`, entry point
  `pr_u3c6_1298_cpu_run(ram, ram_size, cpu, retf_policy, hooks)`.

The model uses the full-state precedent conventions: 1 MiB A20-off RAM and a
14-register CPU. All instructions of the body and of entries 15, 12 and 13 run
in original order on live state, including the real pushes, frames, flags and
RETFs.

- **Hook.** `entry10_0996` receives the full CPU and RAM:
  - when: after the real pushes (four argument words, CS, and return IP
    `0x12E5`), with CS:IP at the live CS:`0996`;
  - what it must do: model the whole callee, including its own `RETF 8`.
    Entry 10 reaches U3CE entry 5 (`+04FC`) and resident `0x464:+1B52`; it is
    **opaque**, with no body, ownership or effect claim.
  - Statuses: `NEED_HOOK` (no hook), `CALLEE_FAILED` (non-zero result),
    `CALLEE_TRANSFER` (return to anything but CS:`12E5`) and `CODE_CHANGED`
    (any modelled code byte differs from the entry snapshot after the hook).
- **Returns.** After each folded RETF, CS:IP must be the pushed return.
  Otherwise the status is `RETURN_CHANGED`.
- **RETF policy.** Every RETF the model executes follows one caller-declared
  policy, with the values and meaning of the merged public `+0000` model
  (PR #629; `.h` blob `9a7287ed…`, `.c` blob `6174a4a6…`):
  - entry 15 `+0CB6`;
  - entry 12 `+0C0C`;
  - entry 13 `+0C34`;
  - the body's `+12F8`.

  The three policies:
  - `UNMEASURED` stops before the first RETF reached (entry 15's `+0CB6`),
    after its `POP BP`, with status `RETURN_PENDING`.
  - `WRAPPED_SS` and `CONTIGUOUS_PHYSICAL` are declared synthetic profiles.
    They differ only when a return IP word sits at SS:`FFFE`/`FFFF`.
- **Alias stops.** Every write, including pushes, is checked per byte against
  `[0BE7,0C37)`, `[0C5F,0CB9)` and `[1298,12FB)`. The model stops before an
  aliasing byte, with IP naming the instruction.
- **Undefined flags** follow the pinned lazy flags (`flags.cpp` sha
  `3ea36c4c…d893`): AF = 0 after XOR. After `SHL DI,1`, AF = 1 and
  OF = MSB(result) XOR MSB(operand).
  - **The hook sees them.** Entry 13's `SHL` at `+0C25` is the last flag
    write before the hook. Entry 10 therefore receives AF = 1, CF = SF = OF = 0,
    and ZF and PF from 4s.

## Controls (source only, not yet run)

`tests/recovered/test_overlay_u3c6_1298_cpu.c` builds synthetic state with no
game bytes. It is not wired into CMake and has not been compiled. A test stub
stands in for entry 10.

- **Reference runs.** 120 seeds are run under both synthetic policies.
  - At the hook: SP is entry SP − 0xE; the frame is `12E5`:CS; the argument
    words are `0008`/b/e13/e12; R[4] = a; R[5] = `[0x5F2A + 4s]`; the flags
    are those of the `+0C25` SHL.
  - After the return: CS:IP and SP = entry SP + 0xC are right, BP is popped
    intact even though the stub clobbers BP, the flags equal the hook's, and
    R[4] = 0, R[5] = 1.
- **Live R.** The hook moves `DS:0x6716`. The final stores land at the new R,
  and the old R keeps a and the table byte.
- **Hook contract.** The test covers `NEED_HOOK` (at CS:`0996` with the
  frame), `CALLEE_FAILED`, `CALLEE_TRANSFER` and `CODE_CHANGED`.
- **Code-alias boundaries.** R is placed so that R+4 or R+5 hits the first or
  last byte of a modelled range, or just outside it. Inside, the model stops
  at `+12A2` or `+12C0` with the byte unchanged; outside, it returns.
- **`UNMEASURED`.** The run stops at CS:`0CB6`, with SP on the pushed
  `12B0`, AL = s, R[4] = a and R[5] unwritten.
- **Wrap discriminators.**
  - Entry 15's return IP at SS:`FFFE`: `WRAPPED_SS` completes, and
    `CONTIGUOUS_PHYSICAL` gives `RETURN_CHANGED` to CS `0xBEEF`.
  - The body's return IP at SS:`FFFE`: `WRAPPED_SS` returns to the caller,
    and `CONTIGUOUS_PHYSICAL` returns to CS `0xBEEF`.
- **Arguments.** NULL pointers, a wrong size, an undeclared policy or a wrong
  IP give `ARGUMENT`, with the CPU and RAM unchanged.

## Unresolved

- **Entry 10.** The callee itself, with all its effects and its flag and DF
  residue, is the hook's responsibility.
- **Profile.** The LES rule is the pinned DOSBox-X normal core (`5a9c7c53`,
  `CPU_OMIT_8086` absent, `do_lds_wraparound` = true, `cpu.code.big` =
  false). B's LES operand is always `DS:0x6716`, never at `FFFD`–`FFFF`. Word
  accesses are physically consecutive, which is a profile assumption. Both
  successful RETF policies are synthetic.
- **Overlay dispatch.** The overlay stub dispatch and the caller side.
- **Evidence.** No compile, test, native or oracle evidence.
- **Credit.** Callee credit stays with its owners: the merged U3C6 slot C for
  entries 12–15.
