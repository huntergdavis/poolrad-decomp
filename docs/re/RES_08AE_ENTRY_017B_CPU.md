# RES_08AE +017B: raw CPU model of the four-callee driver entry (driver-internal candidate, never a credit)

This is a raw-CPU14, full-state C model of resident `08AE:[017B,01A5)`, the 42-byte function that, unless `DS:5372 == 2`, calls `137C`,
`1742`, `17AE` and `13B1` in that order. It is one of the two **composition endpoints** of the 08AE driver set (the other is
`00DC` (its own evidence document, a separate PR)): the callees are the other members, so this body reads as the set's spine rather than as one
more leaf. It is a member body for the [driver-internal class](DRIVER_INTERNAL_CLASS.md): translated and evidenced, never counted in a
strict or identity total. See the [source](../../src/recovered/resident_08ae_entry_017b_cpu.c),
[ABI](../../include/poolrad/recovered/resident_08ae_entry_017b_cpu.h) and the
[original-byte audit and native oracle](../../tools/overlay/res08ae_entry_017b_cpu_audit.py).

**Identity remains unresolved** ([RES_08AE_IDENTITY_POLICY.md](RES_08AE_IDENTITY_POLICY.md)). This change adds no strict, identity or
resident-manifest row, and the class refuses one by test.

## Stops are the callees, not the structure

The body has **four call sites and no other control flow of interest**: one conditional jump (`JNE 018D`) and one unconditional jump
(`JMP 01A3`). Each call is `PUSH CS; CALL near` (the far-return convention), so the model treats each **callee entry as a named stop**:
`PR_R08AE_017B_CALL_137C`, `_1742`, `_17AE`, `_13B1`. At a stop the model has executed the `CALL` (CS and the return IP are pushed, IP is the
callee) and does **not** model the callee. Together with `RETURN` (the `RETF` has run) and `ARGUMENT` (nothing changed) these are all the
statuses. To continue after a callee returns the caller sets the post-return CPU state and calls the model again at the return point; the
entries are `017B` (the function) and `0191`, `0198`, `019F`, `01A3` (the four return points); any other IP is `ARGUMENT`. A resumed state
is arbitrary inside the profile, because the callee may have changed any register, flag or memory.

| Entry | Next stop | Stack at the stop (low to high) | Flags at the stop |
| --- | --- | --- | --- |
| `017B`, `DS:5372 != 2` | `CALL_137C` | return IP `0191`, CS, saved BP | byte `CMP DS:5372,2` |
| `017B`, `DS:5372 == 2` | `RETURN` (no call) | frame popped, BP restored | ZF and PF only |
| `0191` | `CALL_1742` | return IP `0198`, CS, word 0 | `XOR AX,AX`: ZF, PF |
| `0198` | `CALL_17AE` | return IP `019F`, CS, word 0 | `XOR AX,AX`: ZF, PF |
| `019F` | `CALL_13B1` | return IP `01A3`, CS | unchanged |
| `01A3` | `RETURN` | BP, IP, CS popped | unchanged |

The semantic core is `AL := DS:5372; DS:5373 := AL` before the comparison, and that both reads and the byte store use the live
caller DS (no segment load in this body).

## Media, extent and callers

START.EXE SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; loader-expanded image SHA-256
`a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e`. GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

The span is **08AE `[017B,01A5)`, 42 bytes / 21 instructions**, physical `[08C5B,08C85)`, ending in `RETF` (no immediate) at `01A4`; the next
body starts at `01A5`. Body SHA-256 `2cf68891044056f55a1559fb007ce30089c3bf070f44e915cb5e571cd7fa613f`. The audit verifies the whole span,
the single return, the instruction count, the four call sites and targets (`018E`→`137C`, `0195`→`1742`, `019C`→`17AE`, `01A0`→`13B1`,
each preceded by `PUSH CS`), no interrupt, port, far or indirect transfer, the one conditional jump, and **no loader relocation in the body**.

Callers, asserted by the checker: a scan of **every `E8` in the whole 08AE segment** whose target is `017B` finds **none**; a hash-locked scan
of every overlay segment fixup finds **zero direct far calls**; the resident image contains **two** `9A 7B 01 AE 08` far-call patterns
whose segment word is a loader relocation, and both are **decoded to their containing functions** (linear decode from the nearest far-call-target start lands on the
call on an instruction boundary): the call at image `45B2` is in the resident function at image `4354` (`+025E`), and the call at image `8A31` is in the function at
`0899:0059` (image `89E9`, `+0048`), the funded wall's `RES_0899_0059`. **No census gap remains.** The callers do not change this body's stops (the stops are its own four callees); they name who
composes it. **Census method.** The caller census is segment-wide by construction rather than by care: the checker compares the raw candidate list (every `E8` in the whole 08AE segment, and every relocated `9A` far-call pattern in the resident image) with the expected decoded list, so a missed site fails the build. It establishes use, not game-specific source identity.

## Measured evidence: original-byte differential

Unicorn 2.1.4 runs the original bytes at `8800:<entry>` until the `RETF` has run or the next instruction would be a callee entry, with both
A20 aliases mapped onto the same memory, then compares **all 14 CPU words, every byte of the 1 MiB RAM and the stop status** with the C model,
with **zero masks**.

- **720 stopped or returned cases and 5 ARGUMENT rejections**: 195 `RETURN`, 93 `CALL_137C`, 144 each of `CALL_1742`, `CALL_17AE` and
  `CALL_13B1`. `ARGUMENT` cases (SP `0005` and `FFFB`, a wrong IP, the mid-instruction IP `01A4`, a short RAM view) are API rejections that leave CPU
  and RAM unchanged; they are not claimed original executable paths.
- **21/21 instruction starts and 2/2 conditional outcomes** (`JNE` taken and not taken), accounted by successor. No exclusions.
- **Independent reference:** in 320 cases whose stack does not alias the data segment, a Python reference of the documented effect (status, AX, BP,
  SP, IP, CS, the six flags with the CMP flags formed by a different formulation, and the byte at `DS:5373`) agrees with the original.
- Cases: all five entries against eight values of `DS:5372` (`00 01 02 03 7F 80 82 FF`) and seven flag images, SP at `0006`, `0007`, `FFFA`, `FFF9` and
  `0100`, **the stack inside the data segment at every SP from `5360` to `5385`** (pushes and the `DS:5373` store overwrite each other, the flag byte and the
  return frame), data segments `0000`, `FFFF` (wrapping through A20-off), `1234` and the code segment, an A20-aliased stack, a return frame on `DS:5372`, and a
  random-RAM image per entry.
- **Mutation check:** four deliberate changes to the model (BP after `PUSH BP`, the `XOR` flag image, the `DS:5373` store address, the `13B1` return IP) are each caught
  by the differential.

The C suite passes **53 checks under ASan/UBSan** (`-Werror`): the return path, each stop's frame and flags, the CMP flag images for `00`, `80`, `82`, the resumed
entries, the `DS:5373` store overwriting the pushed BP (`SS == DS`, `SP = 5375`), and the `ARGUMENT` guards with state unchanged. The focused CTest target
`poolrad_resident_08ae_entry_017b_cpu_tests` builds from CMake and passes 1/1.

## Measured evidence: native DOSBox-X, with declared callee stubs

DOSBox-X normal/386 runs the original instructions relocated to `8800:017B`. The COM entry, return frame, registers and data are synthetic.
**Callee stubs are declared:** the harness places a `JMP FAR` at each of `8800:137C`, `1742`, `17AE` and `13B1` into a capture routine that records the CPU
words, the stack window and the data segment at the callee entry; the callees themselves are **never executed**, so this corroborates the body's
behavior up to each stop and says nothing about the callees.

- **48 synthetic runs** pass: `RETURN` 13, `CALL_137C` 8, `CALL_1742` 9, `CALL_17AE` 9, `CALL_13B1` 9. All 14 CPU words (flags included, no mask), a
  256-byte stack window and the **whole 64 KiB data segment** match, i.e. **65,792 RAM bytes captured; 982,784 bytes omitted from the native comparison**.
  For measured COM segment `0814` the captured physical half-open ranges are `[17040,17140)` and `[50000,60000)`; the exact native masks are `[00000,17040)`
  94,272 bytes, `[17140,50000)` 233,152 bytes and `[60000,100000)` 655,360 bytes, total 982,784. The harness does not replay DOS, BIOS, code or capture
  machinery as identical RAM outside its controlled windows. Full RAM comparison comes from the original-byte differential. The only residue modeled is the
  pushed flag word the harness leaves below the entry SP. RESULT.BIN SHA-256
  `dbc46cf5059790f3e0e19525ca1732876d4b89b64871685ed0c7e394d8a451c6`.
- **Not covered natively:** stack-in-data aliasing, the A20-wrapped data segment and the SP bounds; those rest on the Unicorn run. Native cases are synthetic frames
  and data, not gameplay reachability or device behavior.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git `5a9c7c53481c2e933948039c33256776653f4faf`). It is the packaged binary on PATH,
`/usr/bin/dosbox-x`, `DOSBox-X version 2026.01.02 SDL2`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, with `core=normal`,
`cputype=386`, `cycles=max`; the receipt's `dosbox_x` block records path, hash and version. The pinned build was neither rebuilt nor re-run. This body uses only
`PUSH`, `POP`, `MOV`, `CMP` (byte), `XOR AX,AX`, near `CALL`, `JNE`, `JMP` and `RETF`, no shift, rotate, `MUL`, `LES` or other instruction with a known
flag-convention difference, and the SP bounds exclude stack-offset wrap; all 14 words, flags included, matched in all 48 runs. That does not prove agreement
with the pinned build on states the cases do not reach; a reviewer who needs the pinned result must re-run the native harness with that binary.

## Driver-internal declaration (class membership proposal)

| Item | Value |
| --- | --- |
| Member id | `RES_08AE_017B` (segment-scoped to `RES_08AE_XXXX`) |
| Evidence document | this document |
| Body | 08AE `[017B,01A5)`, **42 bytes / 21 starts**, SHA-256 `2cf68891044056f55a1559fb007ce30089c3bf070f44e915cb5e571cd7fa613f`, from the loader-expanded image |
| Closure | **NOT complete.** Near callees `137C`, `1742`, `17AE`, `13B1` must each be a member, strict or declared before the loader accepts this row |
| Never a credit | yes: no strict or identity row is proposed |

Closure state when this was written (`driver_internal_models.csv` has members `181A` and `0000`): `1742` and `17AE` have merged evidence (#1250, #1253) but no
member row yet; `137C` and `13B1` have no evidence PR. The loader refuses this member until all four are rows, and says which. That is why the row is **text only**
here and is added by the accepting review after the callees are:

    RES_08AE_017B,docs/re/RES_08AE_ENTRY_017B_CPU.md,<review_pr>,<reviewed_commit>,2cf68891044056f55a1559fb007ce30089c3bf070f44e915cb5e571cd7fa613f,42

## Checker, gates and reproduction

`res08ae_entry_017b_cpu_audit.py` declares `CHECKER_ROWS=('RES_08AE_017B',)` and implements `--compile-only` through the same `library()` build, printing
`CHECKER_BUILD_OK` with no media.

Gate results on current main (`16af6e2c`): `check_checker_builds.py RES_08AE_017B --require-current-main` PASS (1 actual checker build for 1 row); ASan/UBSan C
test 53 checks, exit 0; focused CTest `poolrad_resident_08ae_entry_017b_cpu_tests` 1/1 passed; combined receipt `--unicorn --native`: 720 stopped or returned
cases + 5 ARGUMENT, 21/21 starts, 2/2 outcomes, 320 reference-checked, 0 masked RAM bytes (Unicorn); native 48 runs, RESULT.BIN `dbc46cf5...51c6`, 982,784 bytes
omitted. The pairing gate is run on the committed change.


    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/res08ae_entry_017b_cpu_audit.py --unicorn --native --output resources/017b-evidence.json
    python3 tools/overlay/check_checker_builds.py RES_08AE_017B --require-current-main --output resources/017b-checker-build.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_resident_08ae_entry_017b_cpu.c src/recovered/resident_08ae_entry_017b_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_resident_08ae_entry_017b_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_resident_08ae_entry_017b_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`, one checker build at a time, after a load check. Media is read from ignored
`resources/DOS/POOLRAD/`; no original or extracted bytes and no capture binaries are committed. The analysis tools need the `capstone` and `unicorn`
Python modules.

## Selection, reuse and author assessment

Part of the `OVL_006F_0010` proving-row program in the class document; it is the second of the two composition endpoints (`017B`, `00DC` (its own evidence document, a separate PR)).
The audit and native structure follow the merged `RES_08AE_1742` and `RES_08AE_17AE` evidence; the stop-per-callee shape is new. The recall index was unavailable in this
session, so no prior-session result was reused.

| Assessment field | Author finding |
| --- | --- |
| target_function | RES_08AE `[017B,01A5)` |
| source_candidates | None for the behavior; the merged raw `1742`/`17AE` models and audits (pattern only) |
| signals | Span/hash/relocation identity, segment-wide caller census, full-RAM original differential with stops and outcome accounting, independent reference, mutation check, native replay with declared stubs |
| verdict | likely_match within the documented bounded CPU profile; no identity claim |
| review_notes | Driver-internal candidate, never credited; closure incomplete until 137C/1742/17AE/13B1 are members; callees are stops and stubbed natively; packaged DOSBox-X substitution declared; no accounting edits |
