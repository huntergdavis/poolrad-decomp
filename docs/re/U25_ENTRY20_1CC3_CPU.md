# U25 +1CC3 public entry 20: complete raw CPU composition with the SYSTEM Real models

`pr_u25_entry20_1cc3_cpu` translates the complete U25 public entry 20 (far stub `2B:0084`) over exactly 1 MiB of
A20-off RAM and a fourteen-word CPU frame, **composed with the raw SYSTEM Real models** `pr_system_real_*`
([SYSTEM_REAL_CPU.md](SYSTEM_REAL_CPU.md)) through the real far-call frame, with no callback and no shortcut. It
supersedes the hook-based [`pr_u25_entry20_1cc3`](U25_ENTRY20_1CC3.md) as the evidence endpoint: that model reaches the
three Real helpers through fail-closed call-site callbacks and implements no Real arithmetic; this one runs the helpers'
original-byte-verified models, so the whole closure is under one differential. See the
[source](../../src/recovered/overlay_u25_entry20_1cc3_cpu.c),
[ABI](../../include/poolrad/recovered/overlay_u25_entry20_1cc3_cpu.h) and the
[original-byte audit and native oracle](../../tools/overlay/u25_entry20_1cc3_cpu_audit.py).

This is author evidence for independent review, not an accounting change. It edits no ledger, manifest, classifier,
checkpoint or credit record, states no completion percentage, and proposes the identity and strict rows as text only.

## Extent and the call census (measured from the media)

| Item | Value |
| --- | --- |
| Routine | GAME.OVR file [0x9196,0x92C3), unit 0x25 `+1CC3..+1DEF`, **301 bytes / 81 instructions** |
| SHA-256 | cfffb7e933808123e58434886f10226994973bdb120d358f09cfcf97f2ea90ca |
| Exit | the only return is `+1DED RETF 0Ah`; no jumps, no near calls, no locals |
| Neighbours | follows a `RETF 2`; the prologue of `+1DF0` follows directly, so there is no trailing data |
| Outgoing calls | 21 far calls to SYSTEM `0AF8`: `119A`, `1186`, `119E` at `+1CD0`, `+1CDE`, `+1CE3` plus `2Ah*k` for k = 0..6, each with a relocation on its segment word |
| Incoming callers | two fixup-backed far calls through `2B:0084`, at U1C `+1E1F` and `+1E6E` |

Media hashes: START.EXE SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`, GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`. The audit asserts the body hash, the extent, the
absence of jumps, the 21 sites with their relocations, and that the incoming fixup-backed callers are exactly the two
above.

## Behavior

At entry `SS:SP` holds the return IP and CS, the far record pointer R (`[BP+6]`) and a 6-byte Real factor F (`[BP+0Ah]`).
After `PUSH BP; MOV BP,SP`, for each of the seven record words at `R+88h, +8Ah, ..., +94h` in order:

1. `LES DI,[BP+6]`; `AX := ES:[DI+field]`; `XOR DX,DX`;
2. `LCALL 0AF8:119A` (LongInt `DX:AX` to Real);
3. `CX, SI, DI := F` (re-read from `[BP+0Ah]`, `[BP+0Ch]`, `[BP+0Eh]`, because the multiply clobbers them);
4. `LCALL 0AF8:1186` (Real multiply), then `LCALL 0AF8:119E` (Real to LongInt, truncating);
5. `LES DI,[BP+6]` again; `ES:[DI+field] := AX`.

It then pops BP and returns with `RETF 0Ah`. The record pointer and the factor are re-read from RAM at every use, so
the record may alias the frame. Each word is therefore scaled in place by F and truncated, keeping the low 16 bits of the
LongInt result.

**Composition.** Each `LCALL` is entered through the real far-call frame: the operands (`9A`, the entry offset) are
checked before anything is pushed, the caller's CS and the next IP are pushed, CS is set to the call's live segment word
and the matching `pr_system_real_*` model runs. Its pushes (the near return IP, and the saved registers of the general
multiply path) stay in RAM below the stack exactly as the original leaves them, and the differential compares them.

**Profile.** Raw CPU14, exactly 1 MiB A20-off RAM; entry IP `1CC3`; **entry SP in `[0046,FFF2]`** (the saved BP, the far
call frame and the callee's pushes stay above `0040`, and the `RETF 0Ah` frame does not wrap); code immutable; no
interrupts or faults. Status: `OK`; `ARGUMENT` (any violation, nothing changed); `CODE` (a reached `LCALL` is not
`9A <entry>`: stopped at the site, nothing pushed); `CALLEE` (a SYSTEM routine stopped: its RunError exit with `AX=00CD`
or `00CF` and `CS:IP = 0AF8:00E2`, or a refused argument; the diagnostic names the call and keeps its state);
`TRANSFER` (a callee returned somewhere other than the next instruction). `TRANSFER` is unreachable under the profile
and was never produced: stores into the record occur between calls and cannot reach a live return slot.

## Identity under the two-factor standard (an argument, not a credit)

The standard in [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20) asks for
a relocated call site inside game code **and** a subject of documented game state.

1. **Reached from game code, relocated.** Both callers lie in `OVL_001C_1DB1`, which `overlay_classification.csv` calls
   `game_verified` and which is **not strict-credited**, so the basis is the classification, not a credited caller. Both
   sites are `LCALL 2B:0084` with a relocation on the segment word (the audit scans the unit fixup tables for the stub
   address). I decoded the pushes: at `+1E1F` the function pushes `[5D94]:[5D92]` (the active-character far pointer) as the
   record pointer and a local 6-byte Real; at `+1E6E` it pushes a local far pointer and the same local Real.
2. **Operates on documented game state.** The seven words at `+0088..+0094` are the character record's seven unsigned
   resource words, documented with evidence by merged documents for strict-credited bodies:
   [U7F_CLEAR_0508.md](U7F_CLEAR_0508.md) (`OVL_007F_0508`: the seven resource words at `+0088..+0094` against the seven
   32-bit DS totals), [U7F_RECORD_WORD_ADD_003A.md](U7F_RECORD_WORD_ADD_003A.md) (`OVL_007F_003A`),
   [U7F_RESOURCE_SUBTRACT_0A01.md](U7F_RESOURCE_SUBTRACT_0A01.md) (`OVL_007F_0A01`, resource index `+0x88 + 2*index`) and
   [U77_WEIGHTED_28BE.md](U77_WEIGHTED_28BE.md) (`OVL_0077_28BE`, five of the words). The active-character pointer
   `DS:5D92/5D94` is measured natively in
   [SLOT_A_NATIVE_19E3_INSERTION_RETURN.md](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md). This body is the in-place scaling of
   those words by a Real factor.

What is not argued: that the second call's local pointer refers to the same kind of record (its source is a field read
earlier in the caller, not traced here), or the identity of `OVL_001C_1DB1`.

## Measured evidence: original-byte differential

Unicorn 2.1.4 runs the original GAME.OVR bytes of `+1CC3` and the original SYSTEM bytes (the loader-expanded START
image at segment 0) from the entry to the root return, or to the RunError entry. After every case **all 14 CPU words
and every byte of the 1 MiB RAM** are compared with the C model, with **zero masks**, except the declared `AF`-after-shift
split below.

- **1,845 cases:** 1,017 returns, 820 RunError stops, 8 `CODE` stops.
- **Caller:** 81/81 instruction starts. The routine has no conditional, so there is no outcome to account.
- **SYSTEM closure from this caller:** 141 of 210 instruction starts. The 69 unvisited starts are asserted to be
  exactly the union of three groups, each unreachable from this caller for a stated reason (the checker fails if the set
  changes in either direction): the **general multiply path** (62 starts, `+0F5D..+0F68` and `+0F89..+1001`), because the
  first operand is a converted 16-bit word and `+10D9` leaves `AH=0` and `BX=0` (`DX=0` on entry, so `BX` stays 0 through
  the shift loop, and `AH` is 0 from `MOV AX,00A0h`), so `+0F55`/`+0F59` always take the short path; the **negative-LongInt
  arm of `+10D9`** (4 starts, `+10E5..+10EC`), because the caller zeroes `DX` before every `+119A`; and the **rounding arm
  of `+1118`** (3 starts, `+1155..+115B`), because the `+119E` wrapper sets `CH=0` before the call. The SYSTEM bodies'
  own coverage and outcomes are the evidence of `SYSTEM_REAL_CPU.md`; this document accounts only what this caller reaches.
- **The carry-out arm is reached, not excluded:** six word/factor pairs found by probing (words just below a power of two,
  such as `FFFEh` with factor mantissa `8001000200h`) round up through every mantissa word and reach `+1021`/`+1023`.
- **Independent arithmetic oracle on the original's final RAM:** for factor 1.0, 2.0, 0.5, -1.0 and 0 the audit asserts
  every stored word (`w`, `2w mod 65536`, `w div 2`, `-w mod 65536`, `0`), so the result is checked against known
  arithmetic and not only against the model.
- **Fixtures reach their path:** the eight malformed-call fixtures (opcode and offset at four sites) stop `CODE` and the
  huge-factor fixtures stop `RunError`; the receipt stores every named fixture's stop. The original is stopped at a
  malformed site before it executes the mutated bytes, as the model stops there with nothing pushed.
- Other cases: all 20 exponent edge values with both signs and three mantissa classes, against edge and random word
  sets, 1,500 random scenarios, 22 fixtures in which the record aliases the frame (the pointer, the factor or the return
  frame changes mid-run), the stack-pointer extremes `0046` and `FFF2`, and a random-RAM image.

**Declared Unicorn split (VP rule i19555).** Unicorn does not set `AF` after a shift and DOSBox-X does; the SYSTEM models
follow DOSBox-X. The audit tolerates exactly that: only the `AF` bit, only when the last flag-writing instruction executed
was a shift, and only with the model's `AF` set. It occurs in **278 of the 1,845 cases**, is counted in the receipt as `af_after_shift_splits`, and is not a
defect: the DOSBox-X run below matches the model in all 191 cases. In this closure a shift can be the last flag writer only
where the multiply's final `SHR CH,1` is followed by the overflow stop (a returning multiply is followed by `+119E`, which
rewrites the flags); the first mismatch the audit met was such a stop. See [SYSTEM_REAL_CPU.md](SYSTEM_REAL_CPU.md) for the
flag conventions and the `MUL` note.

The C suite passes **63 checks under ASan/UBSan** (`-Werror`): factor 2.0 doubles all seven words, 1.0 keeps them, 0
zeroes them, 0.5 halves and truncates (`3 -> 1`, `255 -> 127`, `40000 -> 20000`); the neighbouring words are untouched; the
first call frame's saved BP stays in RAM; an overflowing factor stops at the first nonzero word's multiply (`AX=00CDh`,
`CS:IP=0AF8:00E2`, call site `+1CDE`, nothing stored), and the stop moves to the next nonzero word when the first is zero;
a malformed `LCALL` stops with the registers and `SP` as specified; and the `ARGUMENT` guards leave state unchanged. The
focused CTest target `poolrad_overlay_u25_entry20_1cc3_cpu_tests` builds from CMake and passes 1/1.

## Measured evidence: native DOSBox-X

DOSBox-X normal/386 runs the original bytes of `+1CC3` at `3000:1CC3` and the SYSTEM span at `9000:<entry>` from a
synthetic COM image (conventional memory: `A000..AFFF` is video RAM under DOS). The harness patches the 21 segment words to
the staged SYSTEM segment, loads the seven record words, the pointer and the factor onto a real stack, and patches only
the RunError entry to a trap.

- **191 cases: 115 returns and 76 RunError stops.** All 14 CPU words, the **64-byte stack window** (the saved BP, the far
  frames and the SYSTEM pushes) and the **32-byte record window** (`R+80h..R+9Fh`, holding the seven words) agree with the
  model, zero masks. RESULT.BIN SHA-256 `47c2e1b8986654db74ca7a32d0b1108c0f242d2c7465277b2661a89652d2fe7c`.
- **Not compared natively:** every RAM byte outside those two windows (the whole of the remaining 1 MiB). The Unicorn run
  compares all of it; the native cases are a deterministic even sample of the factor, special and random families, not a
  re-run of all 1,845.
- Native evidence is synthetic registers, frames and record, not natural gameplay reachability.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git `5a9c7c53481c2e933948039c33256776653f4faf`). It is the
packaged binary on PATH, `/usr/bin/dosbox-x`, `DOSBox-X version 2026.01.02 SDL2`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, run with `core=normal`, `cputype=386`, `cycles=max`. The
receipt's `dosbox_x` block records path, hash and version. The pinned build was neither rebuilt nor re-run here. What could
differ: the lazy-flag conventions (this closure exercises shifts, rotates, `MUL`, `ADC`, `INC`/`DEC` and `XOR`), the LES
wraparound and stack-offset wrap at FFFF (the profile excludes the latter; the SYSTEM bodies contain no `LES`, and `+1CC3`'s
`LES` reads are inside the profile). All 14 CPU words, FLAGS included, matched in all 191 cases. The same executable hash is
recorded as the native oracle in the merged `RES_0709_TEXT_06A7`, `RES_0709_SKIP_SPACES_0714`, `U33_PARTY_ATTACH_12E1`,
`U33_1403_EVIDENCE_REFRESH` and `SYSTEM_REAL_CPU` evidence. A reviewer who needs the pinned result must re-run the native
harness with that binary.

## Checker, gates and reproduction

`u25_entry20_1cc3_cpu_audit.py` declares `CHECKER_ROWS=('OVL_0025_1CC3',)` and implements `--compile-only` through the same
`library()` build (this model with the raw SYSTEM Real source), printing `CHECKER_BUILD_OK` with no media.

**Gate run:** on main `2cb7408f` with this branch, `check_checker_builds.py OVL_0025_1CC3` reports
`PASS: 1 actual checker builds for 1 rows`, `check_body_doc_pairing.py --base 2cb7408f` reports every body and checker paired
with evidence and registration, the ASan/UBSan C suite (`-Werror`) passes its 63 checks, and the focused CTest target builds
from CMake and passes 1/1.

The private receipt (`--unicorn --native --output`, generated at code commit `571f60b9`, which the branch head carries
unchanged apart from this document and merges of main) has SHA-256
`474cd2e00f86ad456fbee82320fdd077a7ff3d316431fde5b6695dd1cfbd8a13`. It pins the media, the SYSTEM body hashes, the Unicorn
report with every named fixture's stop and the excluded-start counts, and the native report including the `dosbox_x` block.
An earlier run on the same code reproduced the same differential numbers and the same RESULT.BIN hash.


    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u25_entry20_1cc3_cpu_audit.py --unicorn --native --output resources/u25-1cc3.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_overlay_u25_entry20_1cc3_cpu.c src/recovered/overlay_u25_entry20_1cc3_cpu.c src/recovered/system_real_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_overlay_u25_entry20_1cc3_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_overlay_u25_entry20_1cc3_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`. A spinning native oracle can write gigabytes of log;
the harness caps the DOSBox-X run at 300 s and truncates an oversized log. No original or extracted bytes and no capture
binaries are committed.

## Readiness counterfactual (measured on the pinned queue)

The SYSTEM endpoints it needs are declared covered on main (`RUNTIME_COVERED_ENDPOINTS`, #1215). Only one queue row lists
this body as a callee: `OVL_001C_1DB1` (`game_verified`), which has **three other unready callees** (`OVL_0025_1DF0`
untranslated, SYSTEM `0AF8:118C` unmapped, and the Real endpoints until they were declared), so crediting `1CC3` makes **no
other row composable**. The value of crediting it is the credit and the identity, not an unlock.

## Proposed rows (text only; nothing here edits a CSV)

    strict_full_c_addresses.csv:
    OVL_0025_1CC3,docs/re/U25_ENTRY20_1CC3_CPU.md

    reviewed_function_identities.csv (review_pr and reviewed_commit are the acceptance PR's to fill):
    OVL_0025_1CC3,docs/re/U25_ENTRY20_1CC3_CPU.md,<review_pr>,<reviewed_commit>,cfffb7e933808123e58434886f10226994973bdb120d358f09cfcf97f2ea90ca,301

## Selection, reuse and author assessment

The cheapest credit on the board by the full-closure pricing, after the SYSTEM Real chain made it ready. The body translation
reuses the merged `overlay_u25_entry20_1cc3.c` unchanged in structure, with its three callbacks replaced by the raw models.
The recall index was unavailable in this session, so no prior-session result was reused.

| Assessment field | Author finding |
| --- | --- |
| target_function | U25 entry 20 `[1CC3,1DF0)` composed with SYSTEM `119A`/`1186`/`119E` |
| source_candidates | the merged callback model (structure only) and the raw SYSTEM Real models |
| signals | Body/fixup/two-caller identity, full-RAM original differential over the whole closure, independent arithmetic oracle, native replay, derived-value semantic checks |
| verdict | likely_match within the documented bounded CPU profile |
| review_notes | Provisional identity argument (second caller's pointer not traced); SYSTEM closure partially reached, exclusions asserted; declared AF-after-shift Unicorn split; native covers two windows only; packaged DOSBox-X substitution declared; no accounting edits |
