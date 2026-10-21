# RES_08AE +01A5: raw CPU model of the state setter (driver-internal candidate, never a credit)

This is a raw-CPU14, full-state C model of resident `08AE:[01A5,01D2)`, the 45-byte far procedure with one WORD argument (`RETF 2`) that, unless
`DS:5372 == 2`, calls `137C` (argument 0) or `13B1` (argument non-zero, `DS:5374` zero) and then stores its argument at `DS:5374`. It is a leaf-level
neighbour of the two spine bodies (`00DC`, `017B`) and a composition endpoint in its own right: the resident function at image `2B8B` calls both `00DC`
and `01A5`. It is a member body for the [driver-internal class](DRIVER_INTERNAL_CLASS.md): translated and evidenced, never counted in a strict or
identity total. See the [source](../../src/recovered/resident_08ae_state_01a5_cpu.c),
[ABI](../../include/poolrad/recovered/resident_08ae_state_01a5_cpu.h) and the
[original-byte audit and native oracle](../../tools/overlay/res08ae_state_01a5_cpu_audit.py).

**Identity remains unresolved** ([RES_08AE_IDENTITY_POLICY.md](RES_08AE_IDENTITY_POLICY.md)). This change adds no strict, identity or resident-manifest row, and
the class refuses one by test.

## Scoping: what existed

The merged [`RESIDENT_8AE_STATE_01A5.md`](RESIDENT_8AE_STATE_01A5.md) proves this body's instruction paths and its nine overlay callers and **submits no C translation**
("full-C blocker"). There was no model to reuse. This body is a new raw-CPU14 model whose callees are named stops, which removes that blocker for the body itself: the
DOS vector and port contract belongs to `137C` and `13B1`, not to this procedure. That earlier note's caller list was overlay-only; the census below adds the resident callers.

## Stops are the callees

Each call is `PUSH CS; CALL near` (the far-return convention), so each **callee entry is a named stop**: `CALL_13B1` (return IP `01C2`) and `CALL_137C` (return IP `01C8`),
with no pushed argument. At a stop the model has executed the `CALL` (CS and the return IP pushed, IP = the callee) and does **not** model the callee. Statuses are these two,
`RETURN` (`RETF 2` has run) and `ARGUMENT` (nothing changed). To continue after a callee returns, set the post-return CPU state and call again at the return point; the entries
are `01A5`, `01C2` and `01C8`; any other IP is `ARGUMENT`. A resumed state is arbitrary inside the profile.

| Entry | Path (`f = DS:5372`, `w = DS:5374` as a WORD, `A = [BP+6]`) | Result |
| --- | --- | --- |
| `01A5` | `f == 2` | `RETURN`, nothing stored |
| `01A5` | `f != 2`, `A == 0` | stop `CALL_137C`, return IP `01C8` |
| `01A5` | `f != 2`, `A != 0`, `w == 0` | stop `CALL_13B1`, return IP `01C2` |
| `01A5` | `f != 2`, `A != 0`, `w != 0` | no call: `AX := A; DS:5374 := A`; `RETURN` |
| `01C2`, `01C8` | resumed | `AX := A; DS:5374 := A`; `RETURN` |

The comparison of `A` is `JBE` after `CMP word,0`, i.e. `ZF` (CF is always clear after `CMP x,0`), and `DS:5374` is tested as a whole WORD. The argument is read through the live `BP`, the
store through the live `DS`.

## Media, extent and callers

START.EXE SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; loader-expanded image SHA-256 `a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e`. GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

The span is **08AE `[01A5,01D2)`, 45 bytes / 18 instructions**, physical `[08C85,08CB2)`, ending in `RETF 2` at `01CF`; the next procedure starts at `01D2`. Body SHA-256
`7307263a3cf75d350b4ac4c02226dc74790e437194fb31922620ef24074e0c2f`. The audit verifies the whole span, the single return, the instruction count, the two call sites and targets (`01BF`→`13B1`,
`01C5`→`137C`, each preceded by `PUSH CS`), no interrupt, port, far or indirect transfer, the three conditional and two unconditional jumps with their targets, and **no loader relocation in the body**.

Callers, asserted by the checker. **Census method:** it is segment-wide by construction rather than by care, because the checker compares the raw candidate list (every `E8` in the whole 08AE segment, every
relocated `9A` far-call pattern in the resident image and every overlay segment fixup) with the expected decoded list, so a missed site fails the build. It finds **no near call**, **nine direct overlay far calls**
(units `0036` at `0464`; `005E` at `2816`, `286E`; `0086` at `0DF0`, `0E3E`, `16DD`, `1736`; `03C6` at `0E1A`, `109C`) and **two resident far calls decoded to their function** (linear decode from the nearest function start
lands on the call on an instruction boundary): both in the resident function at image `2B8B` (`+0026` and `+017C`). **Reciprocity:** that function also calls `00DC` (`+0082`, `+008D`), so composing it requires `00DC` and `01A5`
both to be members. These establish use, not game-specific source identity, and do not change this body's stops.

## Measured evidence: original-byte differential

Unicorn 2.1.4 runs the original bytes at `8800:<entry>` until the `RETF 2` has run or the next instruction would be a callee entry, with both A20 aliases mapped onto the same memory, then compares **all 14 CPU words, every
byte of the 1 MiB RAM and the stop status** with the C model, with **zero masks**.

- **603 stopped or returned cases and 5 ARGUMENT rejections**: 529 `RETURN`, 55 `CALL_13B1`, 19 `CALL_137C`. `ARGUMENT` cases (SP `0005` and `FFFB`, a wrong IP, the mid-instruction IP `01CE`, a short RAM view) are API
  rejections that leave CPU and RAM unchanged; they are not claimed original executable paths.
- **18/18 instruction starts and 6/6 conditional outcomes**, accounted by successor. No exclusions.
- **Independent reference:** in 225 cases whose stack does not alias the data segment, a decision table written separately from the C control flow agrees with the original on the stop, IP, SP, CS, the return frame, the `AX` and the
  `DS:5374` store (or the unchanged input).
- Cases: the three entries against four values of `DS:5372` (`0 1 2 3`), three of `DS:5374` (`0`, `1`, `0100`) and six arguments (`0 1 2 00FF 0100 FFFF`); SP at `0006`, `0007`, `FFFA`, `FFF9` and `0100`; data segments `0000`, `FFFF`
  (wrapping through A20-off), `1234` and the code segment; an A20-aliased stack; a random-RAM image; a random (non-faithful) BP; a frame on `DS:5372`; and **the stack inside the data segment at every even SP from `5360` to `5384`** (the
  argument word then overlaps `DS:5374` itself).
- **Mutation check:** four deliberate changes to the model (the `13B1` return IP, the `DS:5374` zero test sense, the store address, the byte-for-word test of `DS:5374`) are each caught by the differential.

The C suite passes **45 checks under ASan/UBSan** (`-Werror`): the `f == 2` return, each stop's frame, the no-call store path, the whole-WORD test, the resumed entries, an aliasing frame where `[BP+6]` is `DS:5374`, and the `ARGUMENT` guards
with state unchanged. The focused CTest target `poolrad_resident_08ae_state_01a5_cpu_tests` builds from CMake and passes 1/1.

## Measured evidence: native DOSBox-X, with declared callee stubs

DOSBox-X normal/386 runs the original instructions relocated to `8800:01A5`. The COM entry, return frame, registers and data are synthetic. **Callee stubs are declared:** the harness places a `JMP FAR` at each of `8800:13B1` and
`8800:137C` into a capture routine that records the CPU words, the stack window and the data segment at the callee entry; the callees are **never executed**, so this corroborates the body up to each stop and says nothing about the callees.

- **48 synthetic runs** pass: `RETURN` 38, `CALL_13B1` 6, `CALL_137C` 4, every entry 16 times. All 14 CPU words (flags included, no mask), a 256-byte stack window and the **whole 64 KiB data segment** match: **65,792 RAM bytes captured;
  982,784 bytes omitted from the native comparison**. For measured COM segment `0814` the captured physical half-open ranges are `[17040,17140)` and `[50000,60000)`; the exact native masks are `[00000,17040)` 94,272 bytes, `[17140,50000)` 233,152
  bytes and `[60000,100000)` 655,360 bytes, total 982,784. The harness does not replay DOS, BIOS, code or capture machinery as identical RAM outside its controlled windows; full RAM comparison comes from the original-byte differential. For a resume
  entry the harness sets `BP = SP`, the faithful post-return state, and the only residue modeled is the flag word it leaves below the entry SP. RESULT.BIN SHA-256 `1d501ffd0aea191caa1eade597d46ecb4f8e8f1c2cf4b17a4a7533ea985c07a5`.
- **Not covered natively:** stack-in-data aliasing, the A20-wrapped data segment, SP bounds and non-faithful BP; those rest on the Unicorn run. Native cases are synthetic frames and data, not gameplay reachability or device behavior.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git `5a9c7c53481c2e933948039c33256776653f4faf`). It is the packaged binary on PATH, `/usr/bin/dosbox-x`, `DOSBox-X version 2026.01.02 SDL2`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, with `core=normal`, `cputype=386`, `cycles=max`; the receipt's `dosbox_x` block records path, hash and version. The pinned build was neither rebuilt nor re-run. This body uses only
`PUSH`, `POP`, `MOV`, `CMP` (byte and word), near `CALL`, conditional and unconditional jumps and `RETF 2`, no shift, rotate, `MUL`, `LES` or other instruction with a known flag-convention difference, and the SP bounds exclude stack-offset wrap; all 14
words, flags included, matched in all 48 runs. That does not prove agreement with the pinned build on states the cases do not reach; a reviewer who needs the pinned result must re-run the native harness with that binary.

## Driver-internal declaration (class membership proposal)

| Item | Value |
| --- | --- |
| Member id | `RES_08AE_01A5` (segment-scoped to `RES_08AE_XXXX`) |
| Evidence document | this document |
| Body | 08AE `[01A5,01D2)`, **45 bytes / 18 starts**, SHA-256 `7307263a3cf75d350b4ac4c02226dc74790e437194fb31922620ef24074e0c2f`, from the loader-expanded image |
| Closure | **NOT complete.** Near callees `13B1` and `137C` must each be a member, strict or declared before the loader accepts this row |
| Never a credit | yes: no strict or identity row is proposed |

Closure state when this was written: `137C` and `13B1` have no merged evidence (par-c holds them). The loader refuses this member until both are rows, and says which. That is why the row is **text only** here and is added by
the accepting review after the callees are:

    RES_08AE_01A5,docs/re/RES_08AE_STATE_01A5_CPU.md,<review_pr>,<reviewed_commit>,7307263a3cf75d350b4ac4c02226dc74790e437194fb31922620ef24074e0c2f,45

## Checker, gates and reproduction

`res08ae_state_01a5_cpu_audit.py` declares `CHECKER_ROWS=('RES_08AE_01A5',)` and implements `--compile-only` through the same `library()` build, printing `CHECKER_BUILD_OK` with no media.
GATE_PENDING

    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/res08ae_state_01a5_cpu_audit.py --unicorn --native --output resources/01a5-evidence.json
    python3 tools/overlay/check_checker_builds.py RES_08AE_01A5 --require-current-main --output resources/01a5-checker-build.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_resident_08ae_state_01a5_cpu.c src/recovered/resident_08ae_state_01a5_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_resident_08ae_state_01a5_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_resident_08ae_state_01a5_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`, one checker build at a time, after a load check. Media is read from ignored `resources/DOS/POOLRAD/`; no original or extracted bytes and no capture binaries are committed. The
analysis tools need the `capstone` and `unicorn` Python modules.

## Selection, reuse and author assessment

Part of the `OVL_006F_0010` proving-row program in the class document; it follows the merged `RES_08AE_00DC` and `RES_08AE_017B` evidence in structure. The recall index was unavailable in this session, so no prior-session result was reused.

| Assessment field | Author finding |
| --- | --- |
| target_function | RES_08AE `[01A5,01D2)` |
| source_candidates | The merged control-flow note (no model); the merged raw `00DC` and `017B` models and audits (pattern) |
| signals | Span/hash/relocation identity, segment-wide caller census (near, overlay, resident), full-RAM original differential with two named stops and outcome accounting, independent decision-table reference, mutation check, native replay with declared stubs |
| verdict | likely_match within the documented bounded CPU profile; no identity claim |
| review_notes | Driver-internal candidate, never credited; closure incomplete until 137C/13B1 are members; callees are stops and stubbed natively; packaged DOSBox-X substitution declared; no accounting edits |
