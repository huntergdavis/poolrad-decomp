# U1C +0AED: raw-CPU ECL COMPARE AND composing U25 +0000/+0136 and SYSTEM FillChar

`pr_u1c_compare_and_0aed_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x1C` routine `+0AED`, the ECL opcode `0x14` COMPARE AND
handler ([ECL_CONTRACTS.md](ECL_CONTRACTS.md)). It is the raw-CPU
counterpart of the provisional hook model
[`pr_u1c_compare_and_0aed`](U1C_COMPARE_AND_FULLSTATE_0AED.md), which takes
callee effects from an external callback. This model runs the merged callee
models instead and leaves that file unchanged.

The VP assigned it to this lane on 2026-10-17 (i17678), after Astra
cleared it. In the census on main `3234979` its callees are U25 `+0000`
and `+0136` (both strict, with raw-CPU interfaces) and SYSTEM FillChar.
None of them has an interface caveat. The classifier lists the routine as
`game_verified`, grade B, so this is a strict-credit upgrade with no new
identity row, like U1C `+266A`. Strict credit is the verifying lane's
decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; expanded image at load segment 0 places SYSTEM at `0AF8` and the overlay stub table at `002B` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U1C `+0AED` | GAME file `0x1833..0x1897`; **101 bytes, 40 instructions, three conditional branches**; plain `RETF`; SHA-256 `c45465b78fc299d0f7e52d7bab18c58568e7c5c87759ae9f231561c35c24161f` |
| Far calls | `+0AFF` `LCALL 0AF8:16A8`; `+0B07` `LCALL 002B:002A` (stub `EA 0000:U25`, U25 `+0000`); `+0B19` `LCALL 002B:0025` (stub `EA 0136:U25`, U25 `+0136`); each segment word is fixup-backed |

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP and
`[SP+2]` the return CS. It takes no arguments and returns with plain
`RETF`. After `PUSH BP; MOV BP,SP; SUB SP,0Ah`:

1. `FillChar(DS:6F78h, 6, 0)` clears the six compare flags.
2. `MOV AL,4; PUSH AX` and the load stub run U25 `+0000`. This loads four
   ECL operands from the script stream into the tag, high and low tables
   and advances the PC.
3. For i := 1 to 4 (a byte at `BP-9`), `MOV AL,[BP-9]; PUSH AX` and the
   value stub run U25 `+0136`, which returns operand i's value.
   `MOV DX,AX` then stores it at `BP-0Ah + 2i`.
4. If v[1] = v[2] and v[3] = v[4], it sets `DS:[6F78h]` := 1. Otherwise it
   sets `DS:[6F79h]` := 1. Then `MOV SP,BP; POP BP; RETF`.

AX ends as v[1] when the first pair differs and as v[3] otherwise. DX ends
as v[4] and DI as 8. The pushed index words keep AH from the previous AX:
on the first pass that is U25 `+0000`'s returned AH, and later it is 0
after `XOR AH,AH`. Every operand is reread from RAM in source order.

**No host loop cap.** U25 `+0000` writes the operand tables and the PC
before i is set. U25 `+0136` and its chain write only stack frames below
this one. Nothing but this routine therefore writes i, and the loop makes
exactly four passes. A cyclic party list inside U25 `+0136`'s load-word
chain does not terminate, as in the original; that callee's API has no
budget. The limits pass U25 `+0000`'s SYSTEM segment and its two positive
budgets.

**Stops.**

- `CODE`: a reached `LCALL` whose opcode or offset word differs, or whose
  stub is not `EA` to the expected entry, stops before it with IP at the
  site. The sites move with the entry IP.
- `CALLEE`: a callee model stopped. `*callee` names it and
  `*callee_status` gives its status, for example U25 `+0000`'s operand
  budget.
- `TRANSFER`: a callee's `RETF` popped a CS:IP other than the call's next
  instruction.

**Flags.** The final flags are the last `CMP`'s: v[1]:v[2], or v[3]:v[4]
when the first pair is equal. At a `CODE` stop they are those of the last
flag writer before the site:

| Site | Flags at a `CODE` stop |
| --- | --- |
| FillChar site `+0AFF` | `SUB SP,0Ah` |
| load site `+0B07` | `SUB SP,0Ah`, with DF cleared by FillChar |
| value site `+0B19`, first pass | U25 `+0000`'s |
| value site `+0B19`, later passes | `INC BYTE [BP-9]`'s, with CF from `CMP BYTE [BP-9],4` |

The `XOR` and `SHL` writes are overwritten first and are folded. Offsets
wrap at 16 bits and physical addresses at 1 MiB. `POP` and this routine's
`RETF` frame are read physically contiguous, and each callee keeps its own
contract. Null RAM, CPU or limits, or RAM that is not exactly 1 MiB,
returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u1c_compare_and_0aed_cpu.c` runs under
CTest with ASan/UBSan, and this file compiles with `-Wconversion
-Wsign-conversion -Werror`. It uses no game bytes: only the three
`LCALL` sites, the two loaded-stub `EA` jumps and four synthetic tag-0
operands. An independent reference predicts all of the following:

- the compare flags;
- the stored values, i and the operand tables;
- the PC advance (2 per operand, then 1);
- AX, DX, DI and the final `CMP` flags.

It passes 6,142 checks:

- 408 operand sets;
- a `CODE` stop at each site, including bad stub bytes. One construction
  places the value stub over the frame so that the first pass's store
  clears its `EA`, and the second pass then stops with `INC`'s flags;
- a U25 `+0000` operand-budget `CALLEE`;
- a relocated entry;
- the argument guards.

**Unicorn differential.** `tools/overlay/u1c_compare_and_0aed_cpu_audit.py`
places the expanded START.EXE image at load segment 0. It patches the U25
stub entries to their loaded `EA` jumps and loads units `0x25` and `0x1C`,
so U25, its string cores and FillChar run live from original bytes. It
compares the entire 1 MiB RAM and all 14 CPU words with no mask. Site and
stub bytes are read live at each site fetch. The operand budget is mirrored
by counting fetches of U25 `+0022`.

| Stops | Cases |
| --- | --- |
| Returns | 175 |
| `CODE` | 5 |
| `CALLEE` (operand budgets 1–3) | 3 |
| `TRANSFER` | 1 |

Coverage is 40/40 instruction starts and 6/6 conditional outcomes. The
cases cover:

- 160 random four-operand streams over tags 0, 2, 81h, 10h and 55h,
  with pairs made equal more often than chance;
- ten tag shapes in equal and unequal forms;
- stacks at both segment ends, including FillChar's return frame across
  FFFFh (the `TRANSFER`);
- SS across A20 and a random background;
- four rewritten sites or stubs, and the value stub over the frame.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **64
cases** with no mask, 33 of them equal.

- The COM copies unit `0x1C` to paragraph `8800`, unit `0x25` to `7800`
  and FillChar to `9000:16A8`. It writes the loaded stub jumps at
  `7F00:0025/002A` and relocates the three site segment words.
- The cases use tags 0, 2, 10h and 55h, which make no SYSTEM string calls.
- Each case compares all 14 CPU words, a 256-byte stack window and 20 data
  bytes: the compare flags, the PC and the operand tables.
- The result SHA-256 is
  `c89368ac80e7029265dc31ad838ac43d00e02babe692549e3ce6aa9658eafc79`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u1c_compare_and_0aed_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u1c_compare_and_0aed_cpu_audit.py --unicorn --native --output resources/u1c-0aed-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U1C `+0AED..+0B51` (ECL COMPARE AND), composing U25 `+0000`/`+0136` through the loaded stub and SYSTEM FillChar |
| `source_candidates` | `src/recovered/overlay_u1c_compare_and_0aed_cpu.c` with the merged U25 loader, slot-value, string-core, load-word and party-index models and `src/recovered/system_runtime_leaves_cpu.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with U25 and SYSTEM live and full instruction and branch-outcome coverage, every reachable stop kind, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_verified` (ECL opcode 14h): a strict-credit upgrade only. The provisional hook model stays as it is. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.
