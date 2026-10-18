# Unit 0x3B4 +00C7, +00FB and +012F case maps

`pr_u3b4_case_map_00c7`, `pr_u3b4_case_map_00fb` and
`pr_u3b4_case_map_012f` translate three complete DOS v1.3 routines in
`GAME.OVR` unit `0x3B4`. Each is a nested Pascal function that maps one
argument byte through a short chain of compares to a result byte in AL.
They are leaves: no calls, no interrupts. No gameplay meaning is inferred,
and "case map" names the effect. The recall attempt for this lane found
nothing to reuse.

## Exact originals, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit | memory segment `0x3CA`, code at GAME file `0x2DB87` (187,271), `0x2A5` bytes; public entries `+0000`, `+016D`, `+02A0` |
| `+00C7` | GAME file `0x2DC4E..0x2DC81`; **52 bytes, 21 instructions, three conditional branches**; SHA-256 `40e4906ad1e43ccb6aac60fb31fc22439ae685cb75b0dad1fc9817b423566c63` |
| `+00FB` | GAME file `0x2DC82..0x2DCB5`; **52 bytes, 21 instructions, three conditional branches**; SHA-256 `1c6533364a6c2a5cdd98204054f09ef401640829386337636b943fdd69fe9174` |
| `+012F` | GAME file `0x2DCB6..0x2DCF3`; **62 bytes, 25 instructions, four conditional branches**; SHA-256 `c10ba3f539b4157aab88a4e4bd572aae8f439c3bef2b29403a8f8664a5eb17ba` |
| Exits | `RETF 4` at `+00F8`, `+012C` and `+016A`; each body ends where the next prologue starts |

Every branch target lies inside its own body. None of the three is a
public entry. Decoding every unit-0x3B4 routine from its own prologue
finds exactly one aligned `PUSH BP; PUSH CS; CALL` for each, all inside the
public entry `+016D`: `+01D9` calls `+00C7`, `+01E5` calls `+00FB` and
`+01F1` calls `+012F`. When `DS:84E5` is nonzero, `+016D` passes
`DS:6F69` to the first two and `DS:6F68` to the third. It stores the three
results at `DS:6A61`, `6A62` and `6A63`.

## Complete ABI and ordered effects

Each API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS, `[SP+4]` the caller's frame link and `[SP+6]` a word whose low
byte b is the argument. The link is never read. `RETF 4` removes both
argument words and leaves SP eight bytes above entry.

1. `PUSH BP; MOV BP,SP; SUB SP,2` reserves `[BP-2]` and `[BP-1]`. Only
   `[BP-1]` is ever written.
2. `MOV AL,[BP+8]` loads b. AH keeps its entry value through the return.
3. A chain of `CMP AL,imm8; JNE` tests constants in order. The first match
   stores its byte with `MOV BYTE [BP-1],imm8`. With no match, the last arm
   stores the default.
   - `+00C7`: 08h gives 00h, 41h gives 00h, E8h gives 0Fh, otherwise 0Fh.
   - `+00FB`: 08h gives 08h, 41h gives 07h, E8h gives 07h, otherwise 07h.
   - `+012F`: 0Bh gives 08h, 09h gives 06h, DBh gives 06h, 06h gives 01h,
     otherwise 06h.
4. `MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 4`.

The final flags are those of the last `CMP` executed: the matching one, or
the last in the chain when none matches. The `SUB`'s flags are overwritten
before they are observed. BX, CX, DX, SI, DI, DS and ES are unchanged.
The only RAM writes are the saved BP and `[BP-1]`.

Every operand is reread from RAM in source order. Offsets wrap at 16 bits
and physical addresses at 1 MiB. The `RETF` frame is read physically
contiguous, as the Unicorn oracle reads it; a real 8086 wraps SP inside
the segment instead. The segment-end stack cases write the frame in both
views, so they do not decide between the two. Null RAM/CPU, or RAM that
is not exactly 1 MiB, is rejected before mutation.

## Validation and parity review

The focused C11 test runs under CTest with ASan/UBSan, and compiles with
`-Wall -Wextra -Wpedantic -Wconversion -Werror`. It passes 10,801 checks.
For each routine it runs all 256 argument bytes under two entry FLAGS
values. Each call checks AL, AH, the last `CMP`'s flags, every other CPU
word, the return, and the whole 1 MiB RAM against an independent
reference. It also covers stacks at both segment ends, where `[BP-1]` is
`SS:FFFF` or the argument sits at `SS:FFFC`, and the argument guards.

The Unicorn original-byte differential runs the original unit code and the
C model on copies of one 1 MiB image. Every case compares the entire RAM
and all 14 CPU words with no flag mask, and checks AL against the
documented table. Each routine passes **320 returning cases**:

- all 256 argument bytes, with random high bytes and four FLAGS values;
- 48 random worlds, varying the argument word, link, DS, ES, FLAGS and
  every entry register;
- twelve stack offsets at both segment ends, odd SPs included;
- SS across A20, including the segment end;
- a random background, and preserved DF and IF.

Every instruction start and conditional outcome is reached:

| Routine | Instruction starts | Conditional outcomes |
| --- | --- | --- |
| `+00C7` | 21/21 | 6/6 |
| `+00FB` | 21/21 | 6/6 |
| `+012F` | 25/25 | 8/8 |

The DOSBox-X normal-core/386 synthetic oracle passes **768 cases**: all 256
argument bytes for each routine, under six entry FLAGS values in turn. Each
case compares all 14 CPU words and the 16 stack bytes around the frame,
with no mask. The COM holds the original unit 0x3B4 code at offset 0 of
its own paragraph, with IRQs masked. The result SHA-256 is
`f44a17101fd8ff2ef3cd82c35ebcbc63e068fe368baf0536f357d22c608d5e36` with
COM segment `0814`. Original bytes and generated images stay under
ignored `resources/`.

```sh
cmake -S . -B resources/build-u3b4 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all'
cmake --build resources/build-u3b4 --target poolrad_overlay_u3b4_case_maps_tests
ctest --test-dir resources/build-u3b4 -R poolrad_overlay_u3b4_case_maps --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1800s \
  python3 tools/overlay/u3b4_case_maps_audit.py --unicorn --native \
  --output resources/u3b4-case-maps-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x3B4` `+00C7..+00FA`, `+00FB..+012E` and `+012F..+016C`, three leaves |
| `source_candidates` | `src/recovered/overlay_u3b4_case_maps.c` |
| `signals` | Exact hashes/CFGs, one aligned caller each, whole-RAM/CPU differential with full instruction and branch-outcome coverage, independent C test over every argument byte, native synthetic execution over every argument byte |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Classified `helper_candidate`, so the reviewer decides game identity. The caller `+016D` is not translated. Natural saved-game reach is unmeasured. |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.
