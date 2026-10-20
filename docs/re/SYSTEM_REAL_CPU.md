# SYSTEM 6-byte Real entries 0AF8:1186, 119A, 119E: raw CPU models

Raw-CPU, full-state C models of three Turbo Pascal SYSTEM entry points in the loaded SYSTEM
segment, with no callbacks and no skipped effects: **Real multiply** (`0AF8:1186`), **LongInt to
Real** (`0AF8:119A`) and **Real to LongInt, truncating** (`0AF8:119E`). See the
[source](../../src/recovered/system_real_cpu.c), the
[ABI](../../include/poolrad/recovered/system_real_cpu.h) and the
[original-byte audit and native oracle](../../tools/overlay/system_real_cpu_audit.py).

This is **readiness evidence, with no identity claim**: it makes three SYSTEM endpoints available to
callers. It edits no ledger, manifest, classifier, checkpoint or credit record, states no completion
percentage, and proposes the readiness declaration only as text (below).

## Why these three, and what they gate

`OVL_0025_1CC3` (301 B, with a merged model) calls exactly these three entries, seven times each
(the audit asserts that census from GAME.OVR). They are its only unready callees. The queue lists
four rows with at least one of the endpoints as an unready callee: `OVL_0025_1CC3` (3 of 3, its only
blockers), `OVL_007F_19B2` (5 of 18), `OVL_001C_1DB1` (4 of 6) and `OVL_001C_0234` (3 of 7). Only
`1CC3` becomes composable from these endpoints alone.

## The six bodies (measured from the loader-expanded START.EXE)

Bodies are the bytes **reachable by control flow** from each entry, with the jump to the SYSTEM
RunError entry `0AF8:00E2` as the stop boundary. They are not first-return spans: a span bleeds into
the neighbouring entry stubs at `1180..11D3` (the `1186` wrapper is 12 B by control flow and 80 B by
span), which is how an earlier estimate of this chain came out at 812 B. The control-flow figure is
**448 B in six bodies**.

| Body | Role | Bytes / instructions | SHA-256 (reachable bytes) |
| --- | --- | --- | --- |
| `+1186` | wrapper: `CALL +0F33`, `JB` to RunError 205, `RETF` | 12 / 5 | 1d4c1ec4d7592a5d7178a9ce8f07ccadef80ffa9674d19d0c9c14276f1dbd8e4 |
| `+0F33` | Real multiply core (`+0F30` is its zero-result tail) | 263 / 123 | 8fe816b3b4264cf5aea4b6adb4d423711814791b193dc656526c2ac19930cf67 |
| `+119A` | wrapper: `CALL +10D9`, `RETF` | 4 / 2 | e962b3c25deea98a57e98aac984c325f9cfc8dacebb4c508fdd5943756fad95a |
| `+10D9` | LongInt to Real core | 63 / 30 | 64b69941b112ea462aea3848edd98e4f3ec85523f1b631a38fda67942f1c9251 |
| `+119E` | wrapper: `MOV CH,0`, `CALL +1118`, `JB` to RunError 207, `RETF` | 14 / 6 | 089e1c58fe8ed92def3d01a88e74c1a045324d38bd62b4b1dc7215de4a4cc2f1 |
| `+1118` | Real to LongInt core (CH selects rounding; the wrapper passes 0) | 92 / 44 | 7ccd6539f95a342fa34f081470130d1ed34749d7761369cb05dc02a5723c1234 |

The two RunError stubs the wrappers jump to (`+11B4`: `MOV AX,00CDh; JMP +00E2`, and `+11AE`:
`MOV AX,00CFh; JMP +00E2`) are asserted by the audit. START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` and GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` are checked first.

**The closure reaches nothing outside the six bodies and the RunError stop.** The only calls are the
three near calls between the wrappers and their cores. No covered or uncovered SYSTEM endpoint, no
resident body and no overlay body is called. **No hardware-touching instruction exists on any path**:
the audit asserts that no `IN`, `OUT`, `INT`, `INTO`, `HLT`, `CLI`, `STI`, string port I/O, `IRET`,
`LOCK`, far load or far call, segment-register write or control/debug-register operand occurs. The
instruction mix, recorded in the receipt, is `MOV`, `PUSH`/`POP`, `XCHG`, `ADD`/`ADC`/`SUB`/`CMP`,
`OR`/`AND`/`XOR`/`TEST`/`NOT`, `INC`/`DEC`, `MUL` (12, all in the multiply), `SHL`/`SHR`/`RCL`/`RCR`,
`CLC`/`STC`, jumps and near call/return. They are pure register arithmetic with the stack and the RunError
stop, which is why the models are exact; none of them is on the `RES_0464_2A0D` (BIOS INT 10h) side of
the line.

## ABI and behavior

Each entry is called far: `SS:[SP]` the return IP, `[SP+2]` the return CS, and each ends in `RETF`. A
Real is three words: the exponent byte in `AL` and the mantissa low byte in `AH`, the mantissa middle
word in `BX` (second operand: `CL`/`CH`, `SI`, `DI`), and the mantissa high 15 bits in `DX` with the sign
in bit 15 (second operand `DI`).

- `1186`: `Real * Real` in AL/AH/BX/DX and CL/CH/SI/DI. A zero exponent on either operand gives the zero
  Real. Otherwise the core forms the product (a short path when a low mantissa part is zero, a general
  path with six saved registers otherwise), normalises, rounds, and ends with `CF` set on exponent
  overflow. On `CF` the wrapper loads `AX=00CDh` (205) and jumps to `+00E2`.
- `119A`: `LongInt DX:AX` to Real. Zero returns unchanged. Otherwise the core takes the magnitude, finds
  the leading bit by byte/word steps and a shift loop, and sets the exponent and the sign.
- `119E`: Real to LongInt, truncating (`CH=0`). `CF` is set when the exponent is `A0h` or more; the wrapper
  then loads `AX=00CFh` (207) and jumps to `+00E2`. Exponents below `80h` give zero.

Every instruction of each closure is modelled in source order, **including the pushes**: the near
`CALL`'s return IP and the multiply core's saved registers stay in RAM below the returned SP (18 bytes
in the general multiply path), and the differential compares them.

**Profile.** Exact 1 MiB A20-off RAM; CPU and RAM are distinct host objects; entry IP equal to the entry
offset; **entry SP in `[0040,FFFC]`** (the frame and the deepest push below it neither wrap nor reach the
near return slot); code immutable (the audit fails on any write to a closure byte); no interrupts or
faults (none exist in the bodies). Status: `RETURNED`, `ARGUMENT` (any profile violation, nothing
changed), `RUNERROR` (the original's jump to `+00E2` was reached; the model stops there with `AX` the
error code, `IP=00E2`, and every effect so far kept) and `TRANSFER` (a near return popped an unexpected
address). `TRANSFER` is unreachable under the profile: the only pushes that could reach the return slot
would need a wrapping SP, which the profile excludes; no Unicorn or native case produced it, and it is
exercised nowhere.

## Flag conventions, and the declared Unicorn split

The model follows **DOSBox-X normal/386 at the pinned revision 5a9c7c53**, read from that revision's
instruction macros (an ignored local copy of the source tree at git `5a9c7c53481c2e933948039c33256776653f4faf`):

- `AND`/`OR`/`XOR`/`TEST` clear `CF`, `OF` and `AF`; `ADD`/`ADC`/`SUB`/`CMP` set the six arithmetic flags with the
  usual nibble carry; `INC`/`DEC` keep `CF`.
- **`SHL` and `SHR` by one set `AF`** (the shift count is non-zero), `OF` is the operand's most significant
  bit for `SHR` and the result/operand MSB difference for `SHL`.
- `RCL`/`RCR` by one change only `CF` and `OF`.
- **`MUL` byte and word:** `ZF` is `(AL==0)` or `(AX==0)`, `PF` is even parity over all 16 (byte) or all 32
  (word) result bits, `CF=OF=(high part != 0)`, `SF` and `AF` are kept.

**Declared split (VP rule i19555):** Unicorn 2.1.4 does not set `AF` after a shift. The audit tolerates exactly
that and nothing else: the only difference allowed between the C model and the Unicorn run is the `AF` bit, only
when the last flag-writing instruction executed was a shift (`SHL`/`SHR`) and only with the model's `AF` set. It
occurs in **2,751 of the 5,684 cases** (the multiply paths that end on `SHR CH,1`; the `+1118` shift loop ends on `DEC CL`, which rewrites `AF`), is counted in the
receipt as `af_after_shift_splits`, and is not a defect of the model: the DOSBox-X run below matches the model in
all 273 cases. The `MUL` flag convention differs from Unicorn's too (`ZF`/`PF`), but that difference is **not
observable here**: every `MUL` is followed by an `ADD` or `ADC` that rewrites all six flags before any branch reads
them or any return or stop leaves them, and the Unicorn comparison found no `MUL`-caused difference. The model
still follows DOSBox-X for `MUL`.

## Measured evidence: original-byte differential

Unicorn 2.1.4 runs the original bytes at `0AF8:<entry>` from the loader-expanded image and stops at the root
return or at the RunError entry. After every case **all 14 CPU words and every byte of the 1 MiB RAM** are
compared with the C model, with **zero masks**, except the declared `AF`-after-shift split above.

- **5,684 cases:** `1186` 3,358 (2,928 returns, 430 RunError 205), `119A` 713 returns, `119E` 1,613
  (1,169 returns, 444 RunError 207).
- **Coverage, by entry (instruction starts; conditional outcomes accounted by successor):** `1186` 128/128
  starts and 20/20 outcomes; `119A` 32/32 and 14/14; `119E` **47/50 and 17/20**.
- **The `119E` exclusion, argued:** the three unreached starts (`+1155`, `+1158`, `+115B`: `ADD AX,1`,
  `ADC DX,0`, `JS +116D`) and the three unreached outcomes (`+1153->+1155`, `+115B->+115D`, `+115B->+116D`) are
  the rounding arm of `+1118`, entered only when `CH != 0`. The wrapper `+119E` executes `MOV CH,0` immediately
  before `CALL +1118`, so from this entry no input reaches it. The audit asserts that the missing set is exactly
  this set, so a change in either direction fails.
- **Fixtures reach their path:** the audit derives an expected stop from the arithmetic and fails otherwise:
  zero operands and every LongInt conversion return; a Real to LongInt with exponent `A0h` or more is a RunError
  and one with exponent `9Fh` or less returns. The case families include all 20 exponent edge values squared with
  three mantissa classes, short-path and general-path operands, four pairs of operands found by probing that round
  up through every mantissa word (they reach the carry-out arm at `+1021`), operands with a zero low byte on
  each side, LongInt values at every power of two and its neighbours of both signs, Real to LongInt over every
  exponent from `60h` to `A3h`, stack-pointer extremes and a random-RAM image.

The C suite passes **56 checks under ASan/UBSan** (`-Werror`) with expectations derived from the Real format:
zero, 1, -1, 3 and 65536 to Real (`81h`/`0`, `81h` with the sign bit, `82h` with `DX=4000h`, `91h`); 1.0, 2.5, -2.5,
0.5 and exponent `A0h` to LongInt; `1.0*1.0`, `3.0*-2.0 = -6.0` (`83h`, `DX=C000h`), zero, and the overflow stop; the
leftover pushes in RAM; the stop registers; and the `ARGUMENT` guards with state unchanged. The focused CTest
target `poolrad_system_real_cpu_tests` builds from CMake and passes 1/1.

## Measured evidence: native DOSBox-X

DOSBox-X normal/386 (the binary declared below) runs the original bytes at `9000:<entry>` from a synthetic COM
image: the harness stages the SYSTEM span `0F30..11C0`, loads registers and flags per case, and far-calls the
entry. Only the RunError entry `+00E2` is patched to a trap that records the state; it is not part of the
compared closure.

- **273 cases: `119A` 81 returns; `1186` 75 returns and 16 RunError; `119E` 67 returns and 34 RunError.** All 14
  CPU words and the 32-byte stack window (which holds the near return IP, the saved registers and the far frame)
  agree with the model, zero masks. RESULT.BIN SHA-256 `4d67bc1f08e67ab6eb223b2e30fbbc6797f0ebca243cfa5563d07b54626f6fa8`.
- **What the native comparison does not cover:** every RAM byte outside the 32-byte window. These bodies touch
  memory only through the stack; that claim rests on the Unicorn run, which compares the whole 1 MiB and found no
  other changed byte. Native cases are a deterministic even sample of the differential families plus zero operands,
  not a re-run of all 5,684.
- Native evidence is synthetic registers and frames, not natural gameplay reachability.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git `5a9c7c53481c2e933948039c33256776653f4faf`).
It is the packaged binary on PATH, `/usr/bin/dosbox-x`, `DOSBox-X version 2026.01.02 SDL2`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, run with `core=normal`, `cputype=386`,
`cycles=max`. The receipt's `dosbox_x` block records path, hash and version. The pinned build was neither
rebuilt nor re-run here. What could differ from it: the lazy-flag conventions above (which this body exercises
directly: shifts, rotates, `MUL`, `ADC`, `INC`/`DEC`), the LES wraparound and stack-offset wrap at FFFF (the
profile excludes the latter and the bodies have no `LES`). The model's conventions were read from the pinned
macros, and all 14 CPU words, FLAGS included, matched in all 273 cases under the packaged binary, so no divergence
between the pinned reading and the binary was observed. That does not prove agreement on states the cases do not
reach. The same executable hash is recorded as the native oracle in the merged `RES_0709_TEXT_06A7`,
`RES_0709_SKIP_SPACES_0714`, `U33_PARTY_ATTACH_12E1` and `U33_1403_EVIDENCE_REFRESH` evidence. A reviewer who
needs the pinned result must re-run the native harness with that binary.

## Checker, gates and reproduction

`system_real_cpu_audit.py` declares `CHECKER_ROWS=('RES_0AF8_1186','RES_0AF8_119A','RES_0AF8_119E')` (the SYSTEM
segment ids in the row-id form the gate accepts) and implements `--compile-only` through the same `library()`
build, printing `CHECKER_BUILD_OK` with no media.

**Gate run:** on main `bc94f64f` with this branch, `check_checker_builds.py RES_0AF8_1186 RES_0AF8_119A RES_0AF8_119E`
reports `PASS: 1 actual checker builds for 3 rows`, `check_body_doc_pairing.py --base bc94f64f` reports every body
and checker paired with evidence and registration, and the focused CTest target builds from CMake and passes 1/1.

The private receipt (`--unicorn --native --output`, generated at code commit `a18e8701`, which the branch head carries
unchanged apart from this document and merges of main) has SHA-256
`a1068edb56929b1db29f60c49f5dba182b31359faa0547ea64d8ffccd98f352e`. It pins the media, the six body hashes, the
instruction mix, the Unicorn report with every named fixture's stop and the native report including the `dosbox_x`
block.


    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/system_real_cpu_audit.py --unicorn --native --output resources/real.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_system_real_cpu.c src/recovered/system_real_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_system_real_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_system_real_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`. Media is read from ignored
`resources/DOS/POOLRAD/`; no original or extracted bytes and no capture binaries are committed.

## Proposed declaration (text only; nothing here edits `RUNTIME_COVERED_ENDPOINTS`)

    (0x0AF8, 0x1186): "system Real multiply (system_real_cpu)",
    (0x0AF8, 0x119A): "system LongInt to Real (system_real_cpu)",
    (0x0AF8, 0x119E): "system Real to LongInt, truncating (system_real_cpu)",

Whether and when to add them is the reviewer's call. The before/after counterfactual is stated above: it makes
`OVL_0025_1CC3` composable and leaves `OVL_007F_19B2`, `OVL_001C_1DB1` and `OVL_001C_0234` gated on their other
callees. `1CC3` is then creditable on its own identity argument and evidence, which this document does not make.

## Selection, reuse and author assessment

Priced as the cheapest unlock per cleared row by the full-closure pricing (448 B for one row, against 2,117 B for
the next cheapest). No SYSTEM Real model existed in the tree. Idioms follow the merged `system_longmul_0a5b_cpu`
model and audit, and the merged `0077_275b`/`12E1` flag helpers. The recall index was unavailable in this
session, so no prior-session result was reused.

| Assessment field | Author finding |
| --- | --- |
| target_function | SYSTEM `0AF8:1186`, `119A`, `119E` with cores `0F33`, `10D9`, `1118` (448 B, six bodies) |
| source_candidates | New raw models; no prior translation |
| signals | Control-flow closure, hardware-instruction assertion, full-RAM original differential, native replay, derived-value semantic checks |
| verdict | likely_match within the documented bounded CPU profile; no identity claim |
| review_notes | Declared AF-after-shift Unicorn split (2,751 cases, AF only); `119E` rounding arm excluded and asserted; native covers 14 words and a 32-byte stack window only; packaged DOSBox-X substitution declared; no accounting edits |
