# U31 +0255: raw-CPU candidate check composing U3C2 +0909 and the resident zero test

`pr_u31_candidate_check_0255_cpu` is a raw-CPU, full-state model of the
complete GAME.OVR unit `0x31` routine `+0255`. It composes two callees and
returns the whole CPU14 and 1 MiB RAM through the routine's own `RETF 8`:

- U3C2 entry 6, `+0909`, the merged [candidate-list caller](U3C2_CALLER_0909.md),
  reached through the loaded stub `03DB:003E`. Its `pr_u3c2_caller_0909`
  is linked unchanged.
- The resident routine `00BA:23B7`, a zero test of record byte `+10Eh`.
  Its merged host adapter `pr_rec_res_00ba_23b7` returns only a byte, so the
  13-instruction routine is translated here in full state. No resident file
  is changed.

The routine calls U3C2 `+0909` around an origin. It then compares each
listed candidate record's byte `+10Eh` with the zero-test result for the
active character, and returns 1 when any candidate differs. The gameplay
meaning of that byte is not asserted.

On the refreshed schema queue at main `a00538d`, this was the only
game_evidenced row whose callees were all ready and which had no prior
translation. The VP assigned it to this lane on 2026-10-18 (i17757), after
Astra confirmed no claim. A 90-second `deja "u31 0255"` recall found
2026-10-16 ownership notes: this lane's earlier replacement session and
Codex worker 2 had U31 `+0255` queued behind U31 `+0203`. Those notes give
the same extent and hash as below, but no body was published. Identity and
strict credit are the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. The expanded image at load segment 0 places resident `00BA:23B7..23DA` (36 bytes, 13 instructions, `RETF 4` at `23D8`, SHA-256 `1591e4ada52c8b030f52ef6631d5274e94a2fb1df636f9d58fce5a15cc743c50`) and SYSTEM Move at `0AF8:0A3F` (SHA-256 `5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa`). Both overlay stubs are cold in the image: `03DB:003E` is `CD 3F` with entry `0909`, and `03E0:0025` is `CD 3F` with entry `0000` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U31 `+0255` | GAME file `0xAE15..0xAEAA`; **150 bytes, 58 instructions, three conditional branches**; `RETF 8` at `+02E8`; SHA-256 `908b407691e90b37e132380ee3ad17d42209a64ecbbb2d29ce0fca2333abf552`. Its only caller is U31 `+03C6`, via `PUSH CS; CALL` |
| Far calls | `+0287 LCALL 03DB:003E` (fixup `+028A`), which reaches U3C2 entry 6 `+0909..+0BD8` (SHA-256 `38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed`, `RETF 0Eh` at `+0BD6`); `+02A9 LCALL 00BA:23B7` (fixup `+02AC`) |

## ABI and ordered effects

The routine is entered after a near `CALL` preceded by `PUSH CS`, or after
a far call. `SS:[SP]` holds the return IP and `[SP+2]` the return CS. Four
argument words follow; the first is unused. After `PUSH BP; MOV BP,SP; SUB
SP,4` and flag := 0 at `BP-1`:

1. It pushes U3C2's seven words, in this order:
   - the low bytes of `[BP+0Ch]` and `[BP+0Ah]` (origin X and Y), with AH
     kept;
   - the byte `DS:[3209h + 16 * low byte of [BP+8]]`, with AH := 0;
   - `00FFh` (direction mode) and `0001h` (shape type);
   - `DS:[6718h]` and `DS:[6716h]` (the map segment and offset).

   `LCALL 03DB:003E` then enters the loaded stub, whose `EA` jumps to
   `+0909`. U3C2 leaves its candidate count at `DS:671Ah` and 3-byte
   records after it.
2. count := `DS:[671Ah]` is stored at `BP-3`. For i := 1 to count (i at
   `BP-2`; no pass when count is 0):
   - It pushes the active character `DS:[5D94h]`/`[5D92h]` and calls
     `00BA:23B7`, which sets AL = 1 when the record's byte `+10Eh` is
     zero and 0 otherwise. DL := AL.
   - The candidate's record pointer is `DS:[65B9h + 4 * DS:[6718h + 3i]]`,
     where the record byte is its party index. When that record's byte
     `+10Eh` differs from DL, flag := 1.
3. AL := flag; `MOV SP,BP; POP BP; RETF 8`.

The resident routine runs `PUSH BP; MOV BP,SP; SUB SP,2; LES DI,[BP+6];
CMP BYTE ES:[DI+10Eh],0`. It sets its local `[BP-1]` to 1 or 0, loads AL
from that local, and ends with `MOV SP,BP; POP BP; RETF 4`. Its local and
its frame stay below `BP-4`.

Every operand is reread from RAM in source order, so the list, the pointer
table, the records and the frame may alias. Each `LCALL`'s operands are
read before its pushes, as the CPU fetches them. The stub is read after
them, when the CPU reaches it.

**Loop bound.** The loop needs no cap. Only `INC BYTE [BP-2]` writes i or
count, because the pushes and the resident frame stay below `BP-4`. So the
loop makes exactly count passes.

**U3C2's contract.** The composition passes `pair_cs`, the U3C6 `+0000`
code segment that the stub `03E0:0025` names, and U3C2's three budgets
through unchanged. U3C2's preconditions also carry over: loaded code and
the pair binding stay immutable, and writes avoid code. Its entry SP must
lie in `[008Eh, FFEEh]`. That entry SP is this routine's entry SP minus
`18h`, so an entry SP outside the range, or a zero budget, returns
`ARGUMENT` before any change.

**Stops.**

- `CODE` has two forms:
  - A reached `LCALL` is not `9A` with its expected offset. The model stops
    at the site with nothing pushed.
  - The stub, read after the call's pushes, is not `EA 0909`; a cold
    `INT 3Fh` stub is one example. The model stops at the stub with the far
    return frame pushed.
- `CALLEE`: U3C2 stopped, and `*callee_status` holds its status. That
  covers its caller, line or sort budget, its `CALLEE`, and its `TRANSFER`.
- `TRANSFER`: U3C2's `RETF 0Eh` or the resident routine's `RETF 4` popped a
  CS:IP other than the call's next instruction. In the audit, U3C2's
  candidate records overwrite its own return frame when SS = DS and its
  entry SP is `671Bh`, the merged U3C2 test's layout. Within the accepted
  SP range, the resident routine's frame cannot straddle the segment end.

`*callee` and `*callee_status`, either of which may be NULL, receive the
last callee run and its status (0 for the resident routine). If no callee
ran, they receive `NO_CALLEE` and −1.

**Flags.**

- The final flags are the last `CMP`'s: i against count (equal), or 1
  against a zero count.
- At the U3C2 site and at the stub they are `XOR AH,AH`'s.
- At the resident site they are `CMP 1,count`'s on the first pass. On later
  passes they are `INC BYTE`'s, with CF from the `CMP` before it.
- After the resident routine they are its `CMP BYTE …,0`'s.
- `XOR` leaves AF clear, as both oracles do.
- These flags are folded:
  - `SUB SP,4`;
  - the first `XOR AH,AH` and `SHL DI,CL`;
  - the loop's `XOR`, `SHL` and `ADD`;
  - `CMP AL,DL`;
  - the resident routine's `SUB SP,2`.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs,
`POP` and every `RETF` frame are read physically contiguous. Null RAM or
CPU, or RAM that is not exactly 1 MiB, returns `ARGUMENT` before any
change.

## Validation

**Focused C test.** `test_overlay_u31_candidate_check_0255_cpu.c` runs
under CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. The test uses no game bytes: only the
two `LCALL` encodings, the stub, U3C2's three SYSTEM Move call operands,
and the synthetic U3C2 world from its merged test. It passes 158 checks.

Eight party layouts run with no candidates, one, four and ten:

- With a cold stub, the stop after U3C2's frame is pushed is predicted by
  hand. That covers all 14 registers, the flag local, the seven pushed
  words and the return frame.
- With the warm stub, the result must equal that state carried through
  the merged `pr_u3c2_caller_0909`, followed by an independent reference
  of the candidate loop. The comparison covers the whole 1 MiB and all 14
  words.
- The returned AL is also checked against the rule: DL is 1 when the
  active record's byte is zero, and any candidate byte that differs sets
  the flag. A type-0 member is not a candidate.

The remaining checks cover:

- `CODE` at both sites, with exact SP and flags, and a stub that jumps to
  another U3C2 entry;
- U3C2's caller, line and sort budgets as `CALLEE`;
- U3C2's records over its own return frame as `TRANSFER`, equal to U3C2's
  own carried return;
- pointers aimed at the resident routine's local;
- a relocated entry, and an idle party;
- the argument, budget and SP guards.

**Unicorn differential.** `tools/overlay/u31_candidate_check_0255_cpu_audit.py`
works as follows:

- It places the expanded START.EXE image at load segment 0, so the
  resident routine and SYSTEM Move run from their original bytes.
- It patches both overlay stubs to their loaded `EA` forms and loads units
  `0x3C2`, `0x3C6` and `0x31`.
- It compares the entire 1 MiB RAM and all 14 CPU words with no mask. U3C2's
  world follows its merged audit fixture, with coordinates inside its
  55 × 30 map.

| Stops | Cases |
| --- | --- |
| Returns | 166 |
| `CODE` | 4 |
| `CALLEE` (U3C2 caller, line and sort budgets) | 3 |
| `TRANSFER` | 1 |

The original decides each stop:

- **Return:** the fetch after its own `RETF 8`.
- **`CODE`:** the live site bytes at the site's fetch, or the live stub
  bytes at the stub's fetch.
- **`CALLEE`:** a U3C2 budget point reached with that budget spent,
  counted as the merged U3C2 audit counts it.
- **`TRANSFER`:** CS:IP after U3C2's or the resident routine's `RETF` that
  is not the call's next instruction.
- Any fetch outside the loaded code is an error.

Coverage is 58/58 instruction starts and 6/6 conditional outcomes. The
resident site was fetched 286 times. The returned candidate counts are:

| Candidates | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Cases | 76 | 20 | 21 | 17 | 17 | 1 | 5 | 5 | 2 | 1 | 1 |

Every count from 0 to 10 is reached. Results are 87 zero and 79 one.

The cases cover:

- 150 random worlds, varying the origin, party size, positions and member
  types, radius, map mode, line heights, stacks and flags;
- empty, matching, differing and mixed parties, and ten candidates;
- the lowest accepted entry SP and frames at the segment top;
- SS across A20, a random background, a relocated entry and preserved
  DF/IF;
- both sites rewritten, a cold stub, and a stub to `+0422`;
- the three U3C2 budgets;
- U3C2's return-frame alias;
- candidate pointers aimed at the resident routine's local.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **48
cases** with no mask.

- **Loading.** The COM copies:
  - unit `0x31` to paragraph `8800`, with its sites relocated to the stub
    at `7F00:003E` and the resident span at `7A00:23B7`;
  - unit `0x3C2` to `3000`, with its pair calls relocated to the stub at
    `7E00:0025` and its Move calls relocated to `7000`;
  - unit `0x3C6` to `4000`.
- **Each case.** It refills the whole DS segment at `5000` and the record
  segment at `6000`. It rebuilds U3C2's world and the candidate records,
  then calls `+0255` with IRQs masked.
- **Comparison.** Each case compares all 14 CPU words, a 512-byte stack
  window and the whole 64 KiB DS segment.
- **Spread.** Candidate counts 0–9 are all reached. Results are 12 zero and
  36 one.
- **Result.** The result SHA-256 is
  `8ba5519e93b03bfa3e911ef1b992bfa7f8a9bec46857e15037049234f745761b`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u31_candidate_check_0255_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u31_candidate_check_0255_cpu_audit.py --unicorn --native --output resources/u31-0255-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U31 `+0255..+02EA`, composing U3C2 entry 6 through the loaded stub `03DB:003E` and the resident zero test `00BA:23B7` |
| `source_candidates` | `src/recovered/overlay_u31_candidate_check_0255_cpu.c` with the merged U3C2 `+0909` sources (its U3C6 pair, sort, line-check and SYSTEM leaves) |
| `signals` | Exact hash and CFG; a whole-RAM/CPU differential with U3C2, U3C6, SYSTEM Move and the resident routine live; full instruction and branch-outcome coverage; every stop kind; an independent C test carried through the merged U3C2 model; native synthetic execution |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Identity and strict credit are the verifying lane's call. The resident routine is translated locally; its host adapter is unchanged. The meaning of byte `+10Eh` and natural saved-route reach are not asserted |

No classifier, queue or count ledger is changed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0031_0255 --require-current-main
```
