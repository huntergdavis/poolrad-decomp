# RES_08AE +00DC: raw CPU model of the driver dispatch (driver-internal candidate, never a credit)

This is a raw-CPU14, full-state C model of resident `08AE:[00DC,017B)`, the 159-byte far procedure with one WORD argument (`RETF 2`) that
selects which of the driver's six callees to run. It is the **composition point of the whole 08AE set**: its callees are `17AE` (four sites),
`1742` (four), `181A` (two), `006C`, `137C` and `13B1`, so it reads as the set's spine rather than as one more body; the other endpoint is `017B`
(see [RES_08AE_ENTRY_017B_CPU.md](RES_08AE_ENTRY_017B_CPU.md)). It is a member body for the [driver-internal class](DRIVER_INTERNAL_CLASS.md):
translated and evidenced, never counted in a strict or identity total. See the [source](../../src/recovered/resident_08ae_dispatch_00dc_cpu.c),
[ABI](../../include/poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h) and the
[original-byte audit and native oracle](../../tools/overlay/res08ae_dispatch_00dc_cpu_audit.py).

**Identity remains unresolved** ([RES_08AE_IDENTITY_POLICY.md](RES_08AE_IDENTITY_POLICY.md)). This change adds no strict, identity or
resident-manifest row, and the class refuses one by test.

## Scoping correction: "a bounded dispatch model exists" was not effort saved

The class breakdown lists `RES_08AE_00DC` as "full bar (a bounded dispatch model exists, `resident_8ae_dispatch.c`)". The merged
[`resident_8ae_dispatch`](RESIDENT_8AE_00DC.md) is a **host-hook model over a three-field state** (`DS:5372`, `DS:5374` and one driver byte): it
takes the argument and a callback, calls the callback at each call site and reports the site offset. It has **no CPU frame, no stack, no registers,
no flags, no return IP and CS, and no pushed argument words**. Nothing in this document relies on it, and it does not meet the credited-body
evidence bar (an unmasked CPU14 + 1 MiB differential of the original bytes). Only its call-site order was reused, as a cross-check of the decision
table below.

## Stops are the callees, not the structure

The body is **13 call sites, five join/skip jumps and eight conditional jumps**; it reads like a microcode dispatch but its structure is small. Each
call is `PUSH CS; CALL near` (the far-return convention), so each **callee entry is a named stop**: `CALL_137C`, `CALL_17AE`, `CALL_1742`,
`CALL_181A`, `CALL_006C`, `CALL_13B1`. At a stop the model has executed the `CALL` (CS and the return IP pushed, the argument word pushed below them when
the callee takes one, IP = the callee) and does **not** model the callee. Statuses are these six, `RETURN` (`RETF 2` has run) and `ARGUMENT`
(nothing changed). The thirteen sites are told apart by the return IP at `SS:[SP]`. To continue after a callee returns, set the post-return CPU state
and call again at the return point; the entries are `00DC` and the thirteen return points; any other IP is `ARGUMENT`. A resumed state is arbitrary
inside the profile.

| Site | Callee (stop) | Return IP | Pushed word | Reached when (`f = DS:5372`, `w = DS:5374` as a WORD, `A = [BP+6]`) |
| --- | --- | --- | --- | --- |
| `00F1` | `137C` | `00F4` | none | `f != 2` and `w != 0` |
| `0100` | `17AE` | `0103` | `0` | `A == 0` |
| `0107` | `1742` | `010A` | `0` | after the `17AE` above |
| `010E` | `181A` | `0111` | `0` | after the `1742` above, then join `0156` |
| `011D` | `181A` | `0120` | `1` | `A == 1`, then join `0156` |
| `012B` | `17AE` | `012E` | `0` | `A == 00FF` |
| `0132` | `1742` | `0135` | `0` | after the `17AE` above, then join `0156` |
| `0144` | `1742` | `0147` | `A-1` | `A` not `0/1/00FF` and `f == 1`, then join `0156` |
| `0153` | `17AE` | `0156` | `A-1` | `A` not `0/1/00FF` and `f == 0` |
| `0162` | `006C` | `0165` | `000A` | join `0156` and `w != 0` |
| `0169` | `17AE` | `016C` | `0` | after `006C` |
| `0170` | `1742` | `0173` | `0` | after the `17AE` above |
| `0174` | `13B1` | `0177` | none | after the `1742` above, then `POP BP; RETF 2` |

`f == 2` skips everything and returns. Every comparison is a word or byte compare on live reads (`DS:5374` is tested as a whole WORD, so `0100` is nonzero), and
the argument is read through the live `BP`. `DEC AX` keeps CF; `JBE` after `CMP word,0` is `ZF`.

## Media, extent and callers

START.EXE SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; loader-expanded image SHA-256
`a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e`. GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

The span is **08AE `[00DC,017B)`, 159 bytes / 77 instructions**, physical `[08BBC,08C5B)`, ending in `RETF 2` at `0178`; the next body, `017B`, starts
at `017B`. Body SHA-256 `aae1673000ebd85e67c66df978d2b986de8e4d52cdd7994ade46f1ac5d50bb2a`. The audit verifies the whole span, the single return, the instruction
count, the 13 call sites and targets (each preceded by `PUSH CS`), no interrupt, port, far or indirect transfer, the eight conditional and five unconditional jumps with
their targets, and **no loader relocation in the body**.

Callers, asserted by the checker. **Census method:** it is segment-wide by construction rather than by care, because the checker compares the raw candidate list
(every `E8` in the whole 08AE segment, every relocated `9A` far-call pattern in the resident image and every overlay segment fixup) with the expected decoded list, so a missed
site fails the build. It finds **no near call**, **19 direct overlay far calls** (units `001C` at `2D8C`, `2D9C`, `34D3`; `005E` at `0876`, `145D`, `16FA`, `1778`, `281F`, `2865`; `0068` at
`0742`, `0B14`, `0B59`, `1037`; `0086` at `0E00`, `0E0F`, `0E1A`, `16E6`; `03C6` at `0E2A`, `100E`) and **four resident far calls decoded to their functions** (linear decode from the nearest function
start lands on the call on an instruction boundary): `0899:0000` (image `8990`, call at `+000D`), `0899:0059` (image `89E9`, call at `+0026`), and two calls (`+0082`, `+008D`) in the resident function at
image `2B8B`. That last function **also calls `08AE:01A5`**, so if its row is ever composed both `00DC` and `01A5` must be members. `0899:0059` calls both `00DC` and `017B`. These establish use,
not game-specific source identity, and do not change this body's stops (those are its own callees).

## Measured evidence: original-byte differential

Unicorn 2.1.4 runs the original bytes at `8800:<entry>` until the `RETF 2` has run or the next instruction would be a callee entry, with both A20 aliases mapped onto the same memory, then
compares **all 14 CPU words, every byte of the 1 MiB RAM and the stop status** with the C model, with **zero masks**.

- **1788 stopped or returned cases and 5 ARGUMENT rejections**: 492 `RETURN`, 86 `CALL_137C`, 196 `CALL_17AE`, 332 `CALL_1742`, 108 `CALL_181A`, 373 `CALL_006C`, 201 `CALL_13B1`. `ARGUMENT` cases (SP `0007` and
  `FFFB`, a wrong IP, the mid-instruction IP `0178`, a short RAM view) are API rejections that leave CPU and RAM unchanged; they are not claimed original executable paths.
- **77/77 instruction starts and 16/16 conditional outcomes** (each of the eight jumps taken and not taken), accounted by successor. No exclusions.
- **Independent reference:** in 1050 cases whose stack does not alias the data segment, a decision table written separately from the C control flow (callee, return IP, pushed word per label) agrees with the original on the
  stop, IP, SP, CS, the return frame and the pushed argument, or on the `RETURN` registers and popped frame.
- Cases: all 14 entries against four values of `DS:5372` (`0 1 2 3`), three of `DS:5374` (`0`, `1`, `0100`) and six arguments (`0 1 2 00FF 0100 FFFF`); SP at `0008`, `0009`, `FFFA`, `FFF9` and `0100`; data segments `0000`, `FFFF`
  (wrapping through A20-off), `1234` and the code segment; an A20-aliased stack; a random-RAM image; a random (non-faithful) BP per entry; a frame on `DS:5372`; and **the stack inside the data segment at every even SP from `5360` to `5384`**
  for five entries (pushes, the argument and both inputs alias).
- **Mutation check:** four deliberate changes to the model (the `00FF` comparison, the `181A` return IP, the `006C` argument, a branch sense) are each caught by the differential. A fifth, `JBE` as `ZF` only, is **not**: it is an
  equivalent mutant, because `CMP word,0` always clears CF so `CF|ZF == ZF` at both `JBE` sites.

The C suite passes **100 checks under ASan/UBSan** (`-Werror`): the return path, every stop's frame and pushed word, the argument-selected paths, the whole-WORD test of `DS:5374`, `DEC` flags, the resumed entries and the join, an
aliasing frame where `[BP+6]` overlaps `DS:5372`, and the `ARGUMENT` guards with state unchanged. The focused CTest target `poolrad_resident_08ae_dispatch_00dc_cpu_tests` builds from CMake and passes 1/1.

## Measured evidence: native DOSBox-X, with declared callee stubs

DOSBox-X normal/386 runs the original instructions relocated to `8800:00DC`. The COM entry, return frame, registers and data are synthetic. **Callee stubs are declared:** the harness places a `JMP FAR` at each of
`8800:137C`, `17AE`, `1742`, `181A`, `006C` and `13B1` into a capture routine that records the CPU words, the stack window and the data segment at the callee entry; the callees are **never executed**, so this corroborates the
body up to each stop and says nothing about the callees.

- **48 synthetic runs** pass: `RETURN` 12, `CALL_137C` 1, `CALL_17AE` 7, `CALL_1742` 11, `CALL_181A` 4, `CALL_006C` 10, `CALL_13B1` 3, every entry at least three times. All 14 CPU words (flags included, no mask), a 256-byte stack window and the **whole 64 KiB data
  segment** match: **65,792 RAM bytes captured; 982,784 bytes omitted from the native comparison**. For measured COM segment `0814` the captured physical half-open ranges are `[17040,17140)` and `[50000,60000)`; the exact native
  masks are `[00000,17040)` 94,272 bytes, `[17140,50000)` 233,152 bytes and `[60000,100000)` 655,360 bytes, total 982,784. The harness does not replay DOS, BIOS, code or capture machinery as identical RAM outside its controlled windows;
  full RAM comparison comes from the original-byte differential. For a resume entry the harness sets `BP = SP`, the faithful post-return state, and the only residue modeled is the flag word it leaves below the entry SP. RESULT.BIN SHA-256
  `21b766ad2e3b32eac137e58b6eeb89660290f1eb1a4627cbc6606834a6da7bb3`.
- **Not covered natively:** stack-in-data aliasing, the A20-wrapped data segment, SP bounds and non-faithful BP; those rest on the Unicorn run. Native cases are synthetic frames and data, not gameplay reachability or device behavior.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git `5a9c7c53481c2e933948039c33256776653f4faf`). It is the packaged binary on PATH, `/usr/bin/dosbox-x`, `DOSBox-X version 2026.01.02 SDL2`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, with `core=normal`, `cputype=386`, `cycles=max`; the receipt's `dosbox_x` block records path, hash and version. The pinned build was neither rebuilt nor re-run. This body
uses only `PUSH`, `POP`, `MOV`, `CMP` (byte and word), `DEC AX`, `XOR AX,AX`, near `CALL`, conditional and unconditional jumps and `RETF 2`, no shift, rotate, `MUL`, `LES` or other instruction with a known flag-convention difference, and
the SP bounds exclude stack-offset wrap; all 14 words, flags included, matched in all 48 runs. That does not prove agreement with the pinned build on states the cases do not reach; a reviewer who needs the pinned result must re-run
the native harness with that binary.

## Driver-internal declaration (class membership proposal)

| Item | Value |
| --- | --- |
| Member id | `RES_08AE_00DC` (segment-scoped to `RES_08AE_XXXX`) |
| Evidence document | this document |
| Body | 08AE `[00DC,017B)`, **159 bytes / 77 starts**, SHA-256 `aae1673000ebd85e67c66df978d2b986de8e4d52cdd7994ade46f1ac5d50bb2a`, from the loader-expanded image |
| Closure | **NOT complete.** Near callees `17AE`, `1742`, `181A`, `006C`, `137C`, `13B1` must each be a member, strict or declared before the loader accepts this row |
| Never a credit | yes: no strict or identity row is proposed |

Closure state when this was written (`driver_internal_models.csv` has members `181A` and `0000`): `181A` is a member; `1742`, `17AE` and `006C` have merged evidence (#1250, #1253, #1251) but no member row yet; `137C` and `13B1` have no evidence PR. The loader refuses this
member until all six are rows, and says which. That is why the row is **text only** here and is added by the accepting review after the callees are:

    RES_08AE_00DC,docs/re/RES_08AE_DISPATCH_00DC_CPU.md,<review_pr>,<reviewed_commit>,aae1673000ebd85e67c66df978d2b986de8e4d52cdd7994ade46f1ac5d50bb2a,159

## Checker, gates and reproduction

`res08ae_dispatch_00dc_cpu_audit.py` declares `CHECKER_ROWS=('RES_08AE_00DC',)` and implements `--compile-only` through the same `library()` build, printing `CHECKER_BUILD_OK` with no media.

Gate results on current main (`26e77aec`): `check_checker_builds.py RES_08AE_00DC --require-current-main` PASS (1 actual checker build for 1 row); ASan/UBSan C test 100 checks, exit 0; focused CTest
`poolrad_resident_08ae_dispatch_00dc_cpu_tests` 1/1 passed; combined receipt `--unicorn --native`: 1788 stopped or returned cases + 5 ARGUMENT, 77/77 starts, 16/16 outcomes, 1050 reference-checked, 0 masked RAM bytes
(Unicorn); native 48 runs, RESULT.BIN `21b766ad...7bb3`, 982,784 bytes omitted. The pairing gate is run on the committed change.


    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/res08ae_dispatch_00dc_cpu_audit.py --unicorn --native --output resources/00dc-evidence.json
    python3 tools/overlay/check_checker_builds.py RES_08AE_00DC --require-current-main --output resources/00dc-checker-build.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_resident_08ae_dispatch_00dc_cpu.c src/recovered/resident_08ae_dispatch_00dc_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_resident_08ae_dispatch_00dc_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_resident_08ae_dispatch_00dc_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`, one checker build at a time, after a load check. Media is read from ignored `resources/DOS/POOLRAD/`; no original or extracted bytes and no capture binaries are
committed. The analysis tools need the `capstone` and `unicorn` Python modules.

## Selection, reuse and author assessment

Part of the `OVL_006F_0010` proving-row program in the class document; with `017B` it is one of the two composition endpoints that `RES_0899_0059` calls. The audit and native structure follow the merged `RES_08AE_017B` evidence; the
decision-table reference is new. The recall index was unavailable in this session, so no prior-session result was reused.

| Assessment field | Author finding |
| --- | --- |
| target_function | RES_08AE `[00DC,017B)` |
| source_candidates | The merged bounded host-hook dispatch model (call-site order only), the merged raw `017B` model and audit (pattern) |
| signals | Span/hash/relocation identity, segment-wide caller census (near, overlay, resident), full-RAM original differential with six named stops and outcome accounting, independent decision-table reference, mutation check, native replay with declared stubs |
| verdict | likely_match within the documented bounded CPU profile; no identity claim |
| review_notes | Driver-internal candidate, never credited; closure incomplete until 17AE/1742/181A/006C/137C/13B1 are members; callees are stops and stubbed natively; one equivalent mutant recorded; packaged DOSBox-X substitution declared; no accounting edits |
