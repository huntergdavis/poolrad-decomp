# RES_08AE +1742: raw CPU model of the four-row table writer (driver-internal candidate, never a credit)

This is a raw-CPU14, full-state C model of resident `08AE:[1742,17AE)`, the call-free four-row table writer of the 08AE
timer/audio driver. It is the full-bar member body for the [driver-internal class](DRIVER_INTERNAL_CLASS.md): translated and
evidenced, never counted in a strict or identity total. See the
[source](../../src/recovered/resident_08ae_table_1742_cpu.c),
[ABI](../../include/poolrad/recovered/resident_08ae_table_1742_cpu.h) and the
[original-byte audit and native oracle](../../tools/overlay/res08ae_table_1742_cpu_audit.py).

**Identity remains unresolved** ([RES_08AE_IDENTITY_POLICY.md](RES_08AE_IDENTITY_POLICY.md)). This change adds no strict, identity
or resident-manifest row, and the class refuses one by test.

## Scoping correction: "a table model exists" was not effort saved

The class breakdown lists `RES_08AE_1742` as "full bar (a table model exists)". That note overstated what exists. The merged
[`resident_8ae_table_1742`](RESIDENT_8AE_1742.md) is an **address-only, bounded memory-effects model over a flat 64 KiB
`DS=08AE` view**: it takes a segment buffer and one word, and has **no CPU frame, no stack, no saved-register restore, no
flags, no relocated segment immediate and no return**. It is not the credited-body evidence bar (an unmasked CPU14 + 1 MiB
differential of the original bytes), and nothing in this document relies on its evidence. This body is therefore a **new
raw-CPU14 model**. Only the offset logic (the table at `055F + 8*A`, the 48-byte records from `022D`, the selector bytes at
`0209 + row`) was reused, and it is used as an independent reference in the audit, not as evidence. One difference found
while doing so: the old model wraps a word access inside the 64 KiB segment, while the original, and this model, read and
write words physically contiguous, so a table word at `DS:FFFF` straddles into the next paragraph (a fixture below).

## Media, extent and callers

START.EXE SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; loader-expanded image 68,592 bytes,
SHA-256 `a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e`. GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

The span is **08AE `[1742,17AE)`, 108 bytes / 43 instructions**, physical `[0A222,0A28E)`, ending in `RETF 2` at `17AB`; the next
prologue starts at `17AE`. Body SHA-256 `6128c5126be22f692937ccbb5e5eb2b07c85081a04f086e9bb84579486258484`. The audit verifies the
whole span, the single return, the instruction count, that the body contains no call, interrupt or port instruction, the three
conditional jumps and their targets, and that the **only loader relocation in the body is the data-segment immediate at `174B`**
(`MOV SI,08AEh`), which the model reads live.

Callers: the resident dispatcher `00DC` makes **four near calls** to this entry (`+0107`, `+0132`, `+0144`, `+0170`); a
hash-locked scan of every overlay segment fixup finds **zero direct far calls** to it. Both facts are asserted by the checker.
They establish use, not game-specific source identity.

## Behavior and contract

At entry `SS:[SP]` holds the return IP and CS and `[SP+4]` one WORD argument A (`BP+6`). After `PUSH BP; MOV BP,SP` the body saves
`AX`, `BX`, `DI`, `SI` and `DS`, loads `DS` from the relocated immediate, stores A at `DS:01FF`, forms `SI = 8*A` by three
`ADD SI,SI`, sets the bytes `DS:01D8 = 1` and `DS:01D9 = 4` and `DI = 022D`. For each of four rows it reads the word `DS:[SI+055F]`;
when nonzero it clears the 24 words from `[DI+2E]` down to `[DI]`, stores the word at `[DI+2]`, stores the low byte of the word
`DS:01FF`, **re-read at that point**, at `DS:[0209 + row]` (`row = 4 - DS:01D9`), and stores the word 1 at `[DI]`; every row then adds
`30h` to `DI`, `2` to `SI` and decrements `DS:01D9`. It ends with `DS:01D8 := 0`, pops the six saved registers from live RAM and
returns with `RETF 2`.

Every operand is reread from RAM in source order, so the stack, the table, the records and the control bytes may alias: a later
row's source word can observe an earlier row's writes, and a store can change a saved register or the return frame.

**Profile.** Exactly 1 MiB of A20-off ordinary RAM; CPU and RAM distinct host objects; entry IP `1742`; **SP in `[000C,FFFA]`**
(the six pushes and the `RETF 2` frame do not wrap); code immutable; no interrupts, faults or arbitrary-IP resume. Offsets wrap
at 16 bits and physical addresses at 1 MiB; words are physically contiguous. The final flags are those of the last
`DEC BYTE [01D9]` (result 0) with `CF` kept from the preceding `ADD SI,2`. Status: `OK` (the only executable outcome; the body has
no call and no port access) and `ARGUMENT` (any violation, nothing changed).

## Measured evidence: original-byte differential

Unicorn 2.1.4 runs the original bytes at `8800:1742` (the relocated immediate names the data segment) until the `RETF` has run,
with both A20 aliases mapped onto the same memory, then compares **all 14 CPU words and every byte of the 1 MiB RAM** with the C
model, with **zero masks** and no flag mask.

- **603 RETURN cases and 4 ARGUMENT rejections.** The `ARGUMENT` cases (SP `000B` and `FFFB`, a wrong IP, a short RAM view) are API
  rejections that leave CPU and RAM unchanged; they are not claimed original executable paths.
- **43/43 instruction starts and 6/6 conditional outcomes**, the latter accounted by successor (`JE +1770`, `JGE +177C`,
  `JNE +179E`, each credited per taken and fall-through successor actually executed next). No exclusions.
- **Independent reference:** in 275 cases that do not alias the stack, a Python reference of the documented table effect (written
  from the offsets of the merged model, not from the C model) is compared with the original's final data segment.
- Cases: every one of 16 hit/miss patterns of the four table words for 14 arguments (edge, high-bit and wrap values), generated
  alias arguments that make each row's source word land on a chosen target (record bases, the selector bytes, `01D8`, `01D9`,
  `01FF`) with three high-bit variants each, **row 0's source word at `DS:FFFF`** (`A = 1F54h`, twelve variants of the straddling
  halves), the stack inside the data segment at every even SP from `01C0` to `02FE` (the saved registers and the return frame are
  overwritten by record clears), SP at `000C`, `000D`, `FFF9` and `FFFA`, an A20-aliased stack and data segment, a data segment equal
  to the code segment, and a random-RAM image.

The C suite passes **273 checks under ASan/UBSan** (`-Werror`), with expectations from the documented effect: hit and miss rows, the
selector bytes, the whole WORD versus its low byte, the 16-bit source wrap (`A = 1FA0h`), the register restore and `RETF 2` frame,
the final flags (`0044` under `08D5`), an alias in which row 0's store makes row 3 hit (`A = 1F99h`), and the `ARGUMENT` guards with state
unchanged. The focused CTest target `poolrad_resident_08ae_table_1742_cpu_tests` builds from CMake and passes 1/1.

## Measured evidence: native DOSBox-X

DOSBox-X normal/386 runs the original instructions relocated to `8800:1742`, with the segment immediate patched to data segment
`5000`. The COM entry, return frame, registers and data are synthetic and **no callee stub exists**.

- **48 synthetic direct calls** pass: all 14 CPU words (flags included, no mask), a 256-byte stack window and the **whole 64 KiB data
  segment**, i.e. **65,792 RAM bytes captured; 982,784 bytes omitted from the native comparison**. For measured COM segment `0814` the
  captured physical half-open ranges are `[17040,17140)` and `[50000,60000)`; the exact native masks are `[00000,17040)` 94,272 bytes,
  `[17140,50000)` 233,152 bytes and `[60000,100000)` 655,360 bytes, total 982,784. The harness does not replay DOS, BIOS, code or capture
  machinery as identical RAM outside its controlled windows; that is a harness limitation, not a DOSBox-X limitation. Full RAM comparison
  comes from the original-byte differential above. RESULT.BIN SHA-256
  `541945f91bbab0c99ed67d7074f2311c4c1519006b2a3cb43886e35bae028fa1`.
- **Not covered natively:** the table word straddling `DS:FFFF` (its high byte lies outside the captured data segment); that behavior rests
  on the Unicorn run. Native cases are synthetic frames and data, not natural gameplay reachability or device behavior.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git `5a9c7c53481c2e933948039c33256776653f4faf`). It is the packaged
binary on PATH, `/usr/bin/dosbox-x`, `DOSBox-X version 2026.01.02 SDL2`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, run with `core=normal`, `cputype=386`, `cycles=max`. The receipt's
`dosbox_x` block records path, hash and version. The pinned build was neither rebuilt nor re-run here. What could differ: the
lazy-flag conventions, the LES wraparound and stack-offset wrap at FFFF. This body uses only `ADD`, `SUB`, `CMP` and `DEC` for flags
(no shift, rotate, `MUL`, or logical operation whose `AF` convention differs), has no `LES`, and the SP bounds exclude stack-offset wrap;
all 14 CPU words, FLAGS included, matched in all 48 native cases. The same executable hash is recorded as the native oracle in the merged
`RES_0709_TEXT_06A7`, `RES_0709_SKIP_SPACES_0714`, `U33_PARTY_ATTACH_12E1`, `U33_1403_EVIDENCE_REFRESH`, `SYSTEM_REAL_CPU` and
`RES_08AE_BYTE_181A_CPU` evidence. That does not prove agreement with the pinned build on states the cases do not reach. A reviewer who
needs the pinned result must re-run the native harness with that binary.

## Driver-internal declaration (class membership proposal)

| Item | Value |
| --- | --- |
| Member id | `RES_08AE_1742` (segment-scoped to `RES_08AE_XXXX`) |
| Evidence document | this document |
| Body | 08AE `[1742,17AE)`, **108 bytes / 43 starts**, SHA-256 `6128c5126be22f692937ccbb5e5eb2b07c85081a04f086e9bb84579486258484`, from the loader-expanded image |
| Closure | **complete: nothing to compose.** No calls, interrupts or port operations |
| Never a credit | yes: no strict or identity row is proposed |

Proposed row (text only; **the acceptance PR adds it, not this change**, and fills the two provenance fields that only the accepting
review can supply):

    RES_08AE_1742,docs/re/RES_08AE_TABLE_1742_CPU.md,<review_pr>,<reviewed_commit>,6128c5126be22f692937ccbb5e5eb2b07c85081a04f086e9bb84579486258484,108

## Checker, gates and reproduction

`res08ae_table_1742_cpu_audit.py` declares `CHECKER_ROWS=('RES_08AE_1742',)` and implements `--compile-only` through the same `library()`
build, printing `CHECKER_BUILD_OK` with no media.

Gate results on the merge of current main (`81e2cbec`): `check_checker_builds.py RES_08AE_1742 --require-current-main` PASS (1 actual
checker build for 1 row); ASan/UBSan C test 273 checks, exit 0; focused CTest `poolrad_resident_08ae_table_1742_cpu_tests` 1/1 passed;
combined receipt `--unicorn --native`: 603 RETURN + 4 ARGUMENT, 43/43 starts, 6/6 outcomes, 275 reference-checked, 0 masked RAM bytes
(Unicorn); native 48 cases, RESULT.BIN `541945f9...8fa1`, 982,784 bytes omitted. The pairing gate is run on the committed change.
The analysis tools need the `capstone` and `unicorn` Python modules (the receipt was produced with them from a local virtualenv).


    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/res08ae_table_1742_cpu_audit.py --unicorn --native --output resources/1742-evidence.json
    python3 tools/overlay/check_checker_builds.py RES_08AE_1742 --require-current-main --output resources/1742-checker-build.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_resident_08ae_table_1742_cpu.c src/recovered/resident_08ae_table_1742_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_resident_08ae_table_1742_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_resident_08ae_table_1742_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`, one checker build at a time, after a load check. Media is read from
ignored `resources/DOS/POOLRAD/`; no original or extracted bytes and no capture binaries are committed.

## Selection, reuse and author assessment

Part of the `OVL_006F_0010` proving-row program in the class document; no row becomes composable from this body alone. The offsets
reuse the merged `resident_8ae_table_1742` model (logic only); the raw-model, audit and native patterns follow the merged
`RES_08AE_BYTE_181A_CPU` evidence. The recall index was unavailable in this session, so no prior-session result was reused.

| Assessment field | Author finding |
| --- | --- |
| target_function | RES_08AE `[1742,17AE)` |
| source_candidates | The merged bounded memory-effects model (offset logic only) and the merged raw 181A model (pattern) |
| signals | Span/hash/relocation identity, caller census, full-RAM original differential with outcome accounting, independent table reference, native replay |
| verdict | likely_match within the documented bounded CPU profile; no identity claim |
| review_notes | Driver-internal candidate, never credited; the old table model is not evidence; word straddle at DS:FFFF evidenced by Unicorn only; packaged DOSBox-X substitution declared; no accounting edits |
