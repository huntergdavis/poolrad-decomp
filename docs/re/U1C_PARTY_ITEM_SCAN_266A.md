# U1C +266A: raw-CPU party item scan (ECL FIND ITEM) composing U25 and FillChar

`pr_u1c_party_item_scan_266a_cpu` is a raw-CPU, full-state model of the
complete GAME.OVR unit `0x1C` routine `+266A`. [ECL_CONTRACTS.md](ECL_CONTRACTS.md)
lists it as the handler of ECL opcode 32h, FIND ITEM, with one operand. The
VP assigned it to this lane on 2026-10-17. In the census-2115 queue its
callees are U25 `+0000` and `+0136`, both strict, and SYSTEM FillChar
(`runtime:5800`), with no inline coverage, model, branch or main file. All
three calls run their merged raw-CPU models:

- [`pr_u25_load_operands_0000`](U25_LOADER_0000.md) and
  [`pr_u25_slot_value_0136`](U25_SLOT_VALUE_0136.md), reached through the
  loaded overlay stub;
- [`pr_system_fillchar_16a8`](SYSTEM_RUNTIME_LEAVES.md).

The classifier lists the routine as `game_verified`, grade B. Identity and
strict credit are the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U1C `+266A` | GAME file `0x33B0..0x3464`; **181 bytes, 61 instructions, five conditional branches**; plain `RETF`; SHA-256 `cc815a5eab06818bf60d80ee6d8aaaeec77760d1b33c93c1e8c703759301117c` |
| Calls | `+2673 LCALL 002B:002A` (U25 entry 2, `+0000`); `+267F LCALL 002B:0025` (U25 entry 1, `+0136`); `+2693 LCALL 0AF8:16A8` (FillChar); every segment word is fixup-backed |
| Caller | U1C `+2F2D`, site `+3108`, through `PUSH CS; CALL` |

## ABI and ordered effects

The routine takes no arguments and returns with a plain `RETF`. After
`PUSH BP; MOV BP,SP; SUB SP,0Ch`:

1. Load one ECL operand: U25 `+0000` with a count of 1. The count's low
   byte comes from AL = 1, and AH is whatever AX held.
2. `[BP-0Bh]` := 0, the found flag.
3. v := AL from U25 `+0136` with index 1, the operand's value. `[BP-1]` := v.
4. `FillChar(DS:6F78, 6, 0)`, then `DS:[6F79]` := 1.
5. Walk two lists:
   - for p := the far pointer `DS:[5D96]`, while p <> nil and the found
     flag is clear;
   - for q := p^[0C8h], while q <> nil and the found flag is clear;
   - if q^[2Eh] = v: `DS:[6F78]` := 1, `DS:[6F79]` := 0 and the found
     flag := 1;
   - q := q^[2Ah], then p := p^[104h] after the inner walk.

**Calls.** Each U25 call is a real far call through the overlay stub. The
site must read `9A 2A 00` or `9A 25 00` plus a segment, and the stub there
must hold its loaded `JMP FAR` (EA) to U25 `+0000` or `+0136`. The callee
runs at the stub's target segment and returns straight to this routine.
FillChar is `9A A8 16` plus the loader-relocated SYSTEM segment. U25
`+0000` takes the limits' SYSTEM segment and budgets, as its own contract
requires.

**Stops.** The model returns one of four stop statuses:

- `CODE`: a site or stub holds other bytes, such as an unloaded `INT 3Fh`
  stub. The model stops before the `LCALL`, with IP at the site.
- `CALLEE`: a U25 model stopped. `*callee` names it, and `*callee_status`
  holds its status.
- `TRANSFER`: a callee returned anywhere but the next instruction.
- `LIMIT`: cyclic lists loop forever in DOS. `max_loopbacks` caps the
  taken `JMP +26CB` and `JMP +26AA`, and the next one stops at that loop
  head, entry IP + 61h or + 40h.

U25 `+0136` has no loop budget of its own, so a cyclic party list it walks
does not return, as in DOS.

**Flags.** The final flags are the last `OR` or `CMP`'s, with DF clear from
FillChar. `SUB SP,0Ch`'s flags are computed for a `CODE` stop at the first
site.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs, `POP`
and this routine's `RETF` frame are read physically contiguous. Null RAM,
CPU or limits, or RAM that is not exactly 1 MiB, returns `ARGUMENT` before
any change.

## Validation

**Focused C test.** `test_overlay_u1c_party_item_scan_266a_cpu.c` runs
under CTest with ASan/UBSan and compiles with `-Wconversion -Werror`. Its
only game bytes are the three five-byte far-call sites and the two
loaded-stub jumps. It drives the merged U25 models with a tag-0 ECL operand
and passes 237 checks:

- matches in the first, middle and last lists, absent values, an empty
  party and members without items, with exact compare flags, the found
  flag, the loaded operand and the script PC;
- `LIMIT` at both loop heads;
- `CODE` from an unloaded stub and from a rewritten site;
- `CALLEE` from U25 `+0000`'s budget;
- the argument guards.

**Unicorn differential.** `tools/overlay/u1c_party_item_scan_266a_audit.py`
places the expanded START.EXE image at load segment 0, so SYSTEM and the
overlay stub table are live. It patches the unit `0x25` stub entries
`002B:0025` and `002B:002A` to their loaded EA jumps. It loads unit `0x25`
at its own segment and unit `0x1C` at the caller's, then compares the
entire 1 MiB RAM and all 14 CPU words with no mask. The original's
behaviour decides each stop, and the C status must match. **170 cases**
pass: 164 returns, 3 `CODE` and 3 `LIMIT`. Coverage is 61/61 instruction
starts and 10/10 conditional outcomes. The cases cover:

- 150 random worlds over operand tags 0, 2, 10h, 55h, terminating 81h
  strings and short 80h strings, the last two reaching U25's string cores,
  SYSTEM string routines and the `+0E0C` chain live;
- empty and itemless parties, and cyclic party and item lists;
- stacks at both segment ends, SS and the lists across A20, and a random
  background;
- a rewritten site, a rewritten stub and a rewritten FillChar site.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **48
cases** with no mask; in 11 of them the operand is found among the items.
The COM copies unit `0x1C` to paragraph `8800` and unit `0x25` to `7800`.
It writes the loaded stub jumps at `7F00:0025` and `7F00:002A`, and copies
FillChar to `9000:16A8`. It relocates the three fixup-backed segment words
loader-style. Native tags avoid SYSTEM string calls. Each case compares all
14 CPU words, a 64-byte stack window and 11 data bytes: the compare flags,
the script PC and the operand cells. The result SHA-256 is
`bcb0850f1655653fb76da37886629d56af0c3d87c68dd30ec7ef0cb58a9d4768` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u1c_party_item_scan_266a_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u1c_party_item_scan_266a_audit.py --unicorn --native --output resources/u1c-266a-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U1C `+266A..+271E` (ECL FIND ITEM), composing U25 `+0000`/`+0136` through the overlay stub and SYSTEM FillChar |
| `source_candidates` | `src/recovered/overlay_u1c_party_item_scan_266a_cpu.c` with the merged U25 sources and `src/recovered/system_runtime_leaves_cpu.c` |
| `signals` | Exact hash and CFG, fixup-backed far calls, whole-RAM/CPU differential with every callee live, full instruction and branch-outcome coverage, `CODE` and `LIMIT` stops, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_verified`, grade B; the ECL contract row names it FIND ITEM. Identity is the verifying lane's call. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.
