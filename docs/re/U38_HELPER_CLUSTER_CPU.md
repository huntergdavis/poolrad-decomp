# Unit 0x38 helper cluster: raw-CPU full state

`overlay_u38_helper_cluster_cpu.c` holds raw-CPU, full-state models of seven
complete DOS v1.3 `GAME.OVR` unit `0x38` routines:

- the private helper `+0000`;
- the six public entries that are blocked only on it: 50 (`+1490`),
  92 (`+29B6`), 93 (`+29CB`), 94 (`+29D9`), 95 (`+29F0`) and 96 (`+2A11`).

The entries compose the helper through its real `PUSH AX; PUSH CS; CALL
+0000` frame. [PR #77](OVERLAY_UNIT38_HELPER_0000.md) translated the helper
and entries 93, 94 and 96 with a scalar state struct, but held them out of
the count. That held their role and parity open, and the scalar API cannot
be composed at full state. Those translations are unchanged.

These models add CPU-state ABIs, entries 50, 92 and 95, and parity
evidence. Game identity is the verifying lane's call, as in #77. No
gameplay meaning is inferred.

The VP approved this cluster on 2026-10-17. Astra's Codex branches for
`+2DE4` and `+2E08` also call the helper and are left to that lane; they
can compose this model. The recall attempt for this lane found nothing to
reuse beyond #77.

## Exact originals, callers and ownership

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; START.EXE is
65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. Unit
`0x38` is at GAME file `0x0EC5B`, with memory segment `0x41`.

| Routine | GAME file | Bytes / insns | Exit | SHA-256 |
| --- | --- | --- | --- | --- |
| helper `+0000` | `0x0EC5B..0x0EC79` | 31 / 11 | `RETF 2` | `f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240` |
| entry 50 `+1490` | `0x100EB..0x10145` | 91 / 36 | `RETF 0Ah` | `f4a9c7af804c05fb46ccf500898553904111a5aa65ba36bcd4190eabbca36871` |
| entry 92 `+29B6` | `0x11611..0x11625` | 21 / 12 | `RETF 0Ah` | `4fb3a9c9efb1098c60ddb1f6317dfe93ab7e7755e4534660e7420ee37b0f2820` |
| entry 93 `+29CB` | `0x11626..0x11633` | 14 / 8 | `RETF 0Ah` | `43c37b0e4abefab2d5e537d4a931904ed923a5eb619915cbe25433b166390f26` |
| entry 94 `+29D9` | `0x11634..0x1164A` | 23 / 12 | `RETF 0Ah` | `4428de57a40652837b6b3337f968b56d6fd797c613cc2ff99ec6ab92af12b636` |
| entry 95 `+29F0` | `0x1164B..0x1166B` | 33 / 15 | `RETF 0Ah` | `5aced6012e710406ce05cc9d9d02ca2e0c89b399c406c7c6a4b36cf76b36e005` |
| entry 96 `+2A11` | `0x1166C..0x11682` | 23 / 12 | `RETF 0Ah` | `0782a6473c40d4134208a3d895b145b1c30f4225c5da76588f15eb47d906e84d` |

Each body ends where the next prologue starts, and every branch target is
inside its body. The helper's hash and those of entries 93, 94 and 96 match
#77's table. The only calls are `PUSH CS; CALL +0000`, at:

- `+14E2` in entry 50;
- `+29BD` and `+29C4` in entry 92;
- `+29D2` in entry 93;
- `+29E9` in entry 94;
- `+29F7` and `+29FE` in entry 95;
- `+2A21` in entry 96.

Decoding every unit-0x38 routine from its own prologue finds exactly
twenty aligned `PUSH CS; CALL +0000` sites. The six entries are public
slots 50, 92, 93, 94, 95 and 96, and the helper is not public.

## Complete ABIs and ordered effects

**Helper `+0000`.** It is entered after `PUSH AX; PUSH CS; CALL +0000`,
which builds a far frame:

- `[SP]` IP;
- `[SP+2]` CS;
- `[SP+4]` a word whose low byte is the key k. AH travels in the word.

`PUSH BP; MOV BP,SP; CMP BYTE [BP+6],0`. If k is 0, or `DS:6817`, loaded
into AL and compared with k, equals k, then `DS:6818` := 0 and `DS:6817`
:= 0. Then `POP BP; RETF 2`. The final flags are the last `CMP`'s.

**Public entries.** Each is entered after the far CALL with `[SP]` IP,
`[SP+2]` CS and ten argument bytes that it never reads, and each ends with
`POP BP; RETF 0Ah`.

- **Entry 50** (`+1490`). `SUB SP,2`. If `DS:6819` bit 0 is set, n :=
  `DS:681C`, kept in `[BP-2]`. Then for i = 1..n, kept in `[BP-1]`:
  `DS:6818` -= 2, and `DS:6818` := `DS:681C` if it is now below it. Then
  `DS:6816` += 4, and unless `DS:6819` bit 3 is set, the helper runs with
  key 0. It returns through `MOV SP,BP`.
- **Entry 92** (`+29B6`). Helper with key 0Bh, then with 35h.
- **Entry 93** (`+29CB`). Helper with key 34h.
- **Entry 94** (`+29D9`). Helper with key 0 when `DS:6819` bit 1 is set
  (`AND AL,2; OR AL,AL`).
- **Entry 95** (`+29F0`). Helper with key 37h, then 34h. Then
  `CMP BYTE [682A],0`, and `DS:6816` := 64h when it is 0.
- **Entry 96** (`+2A11`). Helper with key 0 when `DS:6819` bit 0 is set.

Each helper call pushes AX (AL is the key, AH as left), CS and the next IP
relative to the entry IP. It runs the helper on the live CPU at the entry
IP minus the entry's unit offset, and requires its `RETF 2` to return
there. Otherwise the entry stops with `TRANSFER`, CS:IP holding what was
popped.

**Flags.** The final flags are the helper's, the last `AND`/`OR`'s with
AF clear as both oracles leave it, or entry 95's last `CMP`. These flag
writes are folded because they are overwritten before any read:

- `SUB SP,2`;
- `INC BYTE [BP-1]`;
- `SUB BYTE [6818],2`;
- `ADD BYTE [6816],4`.

**Loop cap.** In entry 50, `max_loopbacks` caps the taken `JNE +14CE →
+14B1` loop-backs. On the next one the model stops before the `INC` and
returns `LIMIT`, with IP = entry IP + 21h, SP = BP − 2 and every earlier
effect kept. n passes take n − 1 loop-backs, unless DS stores alias the
frame.

Every operand is reread from RAM in source order. Offsets wrap at 16 bits
and physical addresses at 1 MiB, and the `RETF` frame is read physically
contiguous. Null RAM/CPU, or RAM that is not exactly 1 MiB, is rejected
before mutation.

## Validation and parity review

**Focused C test.** `test_overlay_u38_helper_cluster_cpu.c` runs under CTest
with ASan/UBSan, and compiles with `-Wconversion -Werror`. It passes 546
checks:

- the helper's three paths with exact AL, flags and return;
- every entry's effect, the pushed key words with AH, the return frames and
  the gates;
- entry 50 against an independent reference, over counts 0 to 5, sixteen
  initial values and both bit-3 settings;
- the counter-aliasing world stopping as `LIMIT` at `+14B1`;
- the self-corrupting helper return stopping as `TRANSFER` at `CS:00E5`;
- the argument guards.

The #77 test still passes.

**Unicorn differential.** `tools/overlay/u38_helper_cluster_audit.py`
loads the original unit 0x38 code and compares the entire 1 MiB RAM and all
14 CPU words with no flag mask. The returning cases are:

| Routine | Returning cases | Instruction starts | Conditional outcomes |
| --- | --- | --- | --- |
| `+0000` | 428 | 11/11 | 4/4 |
| `+1490` | 243 | 36/36 | 10/10 |
| `+29B6` | 195 | 12/12 | 0/0 |
| `+29CB` | 195 | 8/8 | 0/0 |
| `+29D9` | 195 | 12/12 | 2/2 |
| `+29F0` | 195 | 15/15 | 2/2 |
| `+2A11` | 195 | 12/12 | 2/2 |

Every body runs 160 random worlds, eight segment-end stacks, SS and DS
across A20, a random background and preserved DF/IF. On top of that:

- the helper runs all 256 keys against matching and non-matching
  `DS:6817`;
- the entries run all sixteen low-nibble `DS:6819` patterns, the key
  values in `DS:6817`, and `DS:682A` = 0;
- entry 50 also runs counts 0 to FFh over initial `DS:6818` values;
- entry 50 runs eighteen frame-aliasing worlds with SS = DS;
- entry 50 runs a counter-aliasing world, with and without a binding cap.

Entry 50 also passes **7 stops**, each equal to the original at the same
point:

- **Six `TRANSFER` stops.** With the frame on `DS:6816..6821`, the helper's
  pushes land on the bytes it clears, so its `RETF 2` leaves for `0000:14E5`
  or `3000:00E5`. The harness stops the original at the first fetch after
  that `RETF`, even when it lands inside the unit.
- **One `LIMIT` stop.** The counter-aliasing world stops at the 51st
  arrival at `+14B1` under a cap of 50. The same world returns under the
  default cap.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **176
cases**: 32 helper calls and 24 per entry. Each case compares all 14 CPU
words, the DS bytes `6810..682F` and 64 stack bytes, with no mask. The COM
holds the original unit code at offset 0 of its own paragraph, so every
entry and every `CALL +0000` runs at its original IP, with IRQs masked.
The result SHA-256 is
`56a01397f5c5af3e040f5ce6712dc91e2474b409e10a2d9f3d6b6057228161cf` with
COM segment `0814`. Original bytes and generated images stay under
ignored `resources/`.

```sh
ctest --test-dir <build> -R 'u38_helper_cluster_cpu|unit38_helper' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u38_helper_cluster_audit.py --unicorn --native --output resources/u38c-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x38` `+0000`, `+1490`, `+29B6`, `+29CB`, `+29D9`, `+29F0`, `+2A11`; the entries compose the helper |
| `source_candidates` | `src/recovered/overlay_u38_helper_cluster_cpu.c` |
| `signals` | Exact hashes/CFGs, the helper's twenty aligned callers, whole-RAM/CPU differential with full instruction and branch-outcome coverage, matched `TRANSFER`/`LIMIT` stops, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Game identity is open (`game_or_other_library_unresolved` for the helper and entries 92 to 96), as in #77. Natural saved-game reach is unmeasured. |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.
