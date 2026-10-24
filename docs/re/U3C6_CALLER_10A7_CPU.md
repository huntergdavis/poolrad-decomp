# U3C6 +10A7: raw-CPU slot placement composing entries 15, 16 and 5

`pr_u3c6_caller_10a7_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x3C6` routine `+10A7`, public entry 18. It returns the whole
CPU14 and 1 MiB RAM through the routine's own `RETF 0Ah`. It composes:

- Entry 15, `+0C5F`: the slot search. Its merged API in
  `overlay_u3c6_slot.h` is semantic only, so its 32 original instructions
  are folded here in full state, as the merged `+0CB9` and `+1298` models
  also fold them.
- Entry 16, `+0CB9`: the merged raw-CPU [scan](U3C6_0CB9_SCAN.md)
  `pr_u3c6_0cb9_cpu_run`, under its `WRAPPED_SS` policy.
- Entry 5, `+03A0`: Astra's merged raw-CPU [occupancy
  rebuild](U3C6_REBUILD_03A0.md) `pr_u3c6_rebuild_03a0` (#977).
- SYSTEM FillChar: the merged `pr_system_fillchar_16a8`.

The VP assigned the routine on 2026-10-18 (i17775), after Astra confirmed
no claim. On the refreshed schema queue it was the only game_evidenced row
whose callees were all ready once #977 merged. Astra's ABI note is
followed: #977 runs `+0000` under `WRAPPED_SS` with non-wrapping return
frames, so `+0CB9` runs under the same policy, and this entry's SP bound
keeps every frame of the chain unwrapped. A 90-second `deja "u3c6 10a7"`
recall timed out with no result. Identity and strict credit are the
verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. The expanded image at load segment 0 places SYSTEM FillChar at `0AF8:16A8` (20 bytes, SHA-256 `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7`) |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U3C6 `+10A7` | GAME file `0x317E1..0x319D1`; **497 bytes, 191 instructions, 17 conditional branches**; `RETF 0Ah` at `+1295`; SHA-256 `50821d75da31383ab9920ef18edaadb16d9ffdea34cc4348c8e86d70903fd1fe`; unit entry 18 |
| Calls | `+10C1 PUSH CS; CALL +0C5F`; `+1119 PUSH CS; CALL +0CB9`; `+11DF LCALL 0AF8:16A8` (fixup-backed, in loop 1); `+1285 PUSH CS; CALL +03A0`. Each near callee ends in `RETF` |

## ABI and ordered effects

The routine is entered after a near `CALL` preceded by `PUSH CS`, or after
a far call. `SS:[SP]` holds the return IP and `[SP+2]` the return CS. The
arguments follow:

- `[SP+4]`: flag byte F;
- `[SP+6]`, `[SP+8]`: the low bytes are Y and X;
- `[SP+0Ah]`/`[SP+0Ch]`: a record far pointer Q.

After `PUSH BP; MOV BP,SP; SUB SP,8`:

1. Unless `DS:[49F3h]` = 5, the result is 1 at once.
2. s := entry 15 of Q: Q's slot in the `DS:65B9h` table, searching from slot
   1 up to the count `DS:[5F2Ah]`, or 0 if absent. Slot s is written as
   follows: type at `5F2Ah + 4s` := `Q^[6Ch] & 7Fh`, X at `5F27h + 4s`, Y at
   `5F28h + 4s`. Slot 0 is the count byte itself.
3. Entry 16 runs with Q, 8, and pointers to the locals c (`BP-4`) and v
   (`BP-5`). The slot is cleared (type := 0) and the result is 0, with no
   `+03A0` call, in any of these cases:
   - c ≠ 0;
   - v = 0;
   - `DS:[2744h + 4v]` = FFh.
4. Otherwise the result is 1. When F ≠ 0 and byte `+13h` of the record at
   `Q^[108h]` is 0, two loops run:
   - **Loop 1** runs k := 1 to `DS:[6715h]` over 7-byte entries at
     `DS:66D6h + 7k`. Each entry whose far pointer equals Q has its byte
     `+6` kept at `BP-5` (unless that byte is 1Fh). The entry is then
     cleared by FillChar.
   - **Loop 2** runs over the same entries. An entry with a nonzero pointer
     and coordinates (X, Y) at `+4`/`+5` sets a seen flag.

   If no entry was seen, the map byte at (`DS:6716h` far pointer) `+ 7 +
   50Y + X` becomes the byte at `BP-5`. Entry 5 then rebuilds the occupancy
   grid and relative-coordinate arrays.
5. AL := result; `MOV SP,BP; POP BP; RETF 0Ah`.

**Calling contract.**

- **Fixed entry.** The near callees sit at their original offsets in this
  CS, so the entry must be `+10A7`.
- **Callee settings.** `+0CB9` runs with `WRAPPED_SS`. `+03A0` gets
  `max_slots`, `max_cells`, and the SYSTEM segment read from its own
  FillChar site operand at `CS:03B5h`, as the CPU will fetch it. Each callee
  keeps its own contract and profile: code and bindings stay immutable, and
  writes avoid code.
- **SP bound.** An entry SP in `[0100h, FFF0h]` keeps every far frame of
  the chain unwrapped. There, `WRAPPED_SS` and physically contiguous frame
  reads agree, and `+03A0`'s SP stays in its accepted range.
- **Operands.** Every operand is reread from RAM in source order.

**Loop bounds.**

- **Loop 1:** each FillChar, and the call's own pushes, can rewrite its
  counter, its count and the scanned pointers through DS = SS aliases.
  Termination is not proved under such aliases, so `max_loopbacks` caps its
  taken `JNE +117F` and the next one stops there with `LIMIT`. This bounds
  alias behaviour; it does not assert that the original runs forever.
- **Loop 2:** writes only its seen flag, so it makes exactly `DS:[6715h]`
  passes.

**Stops.**

- `ARGUMENT`: null RAM or CPU, RAM that is not exactly 1 MiB, IP not
  `10A7h`, SP outside `[0100h, FFF0h]`, or a zero `+03A0` budget. Nothing
  changes.
- `CODE`: the reached FillChar `LCALL` is not `9A A8 16`. The model stops at
  the site with nothing pushed.
- `CALLEE`: `+0CB9` or `+03A0` stopped, and `*callee_status` holds its
  status. For example, `+03A0`'s slot or cell budget.
- `TRANSFER`: a callee's `RETF` popped a CS:IP other than the call's next
  instruction. In the audit, with SS = DS, `+03A0`'s own grid FillChar
  clears `+03A0`'s return frame at `DS:60DBh`.
- `LIMIT`: the loop 1 cap; IP is at `+117F`.

`*callee` and `*callee_status`, either of which may be NULL, receive the
last callee run and its status (0 for the folded entry 15). If no callee
ran, they receive `NO_CALLEE` and −1.

**Flags.**

- On paths through `+03A0`, the final flags are `+03A0`'s.
- When `DS:[49F3h]` ≠ 5, they are `CMP [49F3h],5`'s.
- On the slot-clearing path they are those of `SHL DI,1` (DI := 4s). That
  instruction's AF is architecturally undefined. The model sets it to 1, as
  the pinned DOSBox-X core does; this is the profile of the merged `+0CB9`
  model (see its [scan notes](U3C6_0CB9_SCAN.md)).
- At the FillChar site they are `ADD DI,66D6h`'s.
- At `LIMIT` they are loop 1's last `CMP`.
- The 13 branch-only CMP/OR writers feed ZF branches, whose decisions the
  existing C conditions reproduce. Their lazy and stored ZF rules agree.
- The remaining folds have an original-byte liveness certificate, including
  the partial flag overwrites of `MUL` and `INC`. The [14-group evidence
  record](U3C6_10A7_FOLDED_FLAGS.md) identifies each writer and its observer
  or overwrite. It also distinguishes the two live SHLs already computed.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs,
`POP` and this routine's `RETF` frames are read physically contiguous.

## Validation

**Focused C test.** `test_overlay_u3c6_caller_10a7_cpu.c` runs under CTest
using the repository CMake configuration: no `CMAKE_BUILD_TYPE`,
`-UNDEBUG` so assertions remain active, and the project warning flags.
The test uses no game bytes: only the FillChar `LCALL`
encodings at `+11DF` and `+03B2`. Every shape point is negative, so entry
16 leaves c = 0 and v = 17h. It passes 52 checks.

The paths covered:

- **[49F3h] not 5:** exactly three RAM bytes change.
- **F = 0:** slot writes, plus `+03A0`'s relative coordinates for both
  slots.
- **Type 0:** `+03A0` skips the slot.
- **Rank FFh:** the slot is cleared, with exact flags (AF = 1).
- **Q absent:** slot 0 is the count byte; ZF and PF are set as well.

The entry handling covered:

- Loop 1 clears both entries naming Q and keeps 7 at `BP-5`.
- Loop 2 sees an entry at (X, Y), so no map write happens.
- With no entry seen, the map cell takes 17h.
- A nonzero `R^[13h]` skips the entry work.

The stops and guards covered:

- `LIMIT` at cap 0, with exact IP, SP and flags, and cap 1 completing.
- `CODE` with the pushed FillChar arguments.
- `+03A0`'s slot budget as `CALLEE`.
- `+03A0`'s return alias as `TRANSFER` to `0000:0000`.
- The argument, entry, SP and budget guards.

**Unicorn differential.** `tools/overlay/u3c6_caller_10a7_cpu_audit.py`
works as follows:

- It places the expanded START.EXE image at load segment 0 and loads unit
  `0x3C6` at `3000`.
- Every reachable original function is pre-decoded: `+0000`, `+03A0`,
  `+04BE`, `+0C5F`, `+0CB9`, `+10A7` and SYSTEM FillChar. Any fetch
  outside them is an error.
- It compares the entire 1 MiB RAM and all 14 CPU words.

| Stops | Cases |
| --- | --- |
| Returns | 139 |
| `CODE` | 1 |
| `CALLEE` (`+03A0` slot and cell budgets) | 2 |
| `TRANSFER` | 1 |
| `LIMIT` | 1 |

A call/return stack separates a direct callee's diverted `RETF`
(`TRANSFER`) from a nested one (`CALLEE`). `+03A0`'s budgets are counted as
its merged audit counts them, at fetches of `+03CF` and `+03ED`.

Coverage is 191/191 instruction starts and 34/34 conditional outcomes;
results are 72 zero and 67 one (144 cases total).

The one declared FLAGS split is AF at the 72 slot-clearing returns.
There Unicorn leaves 0 and the checker requires the pinned core's 1;
an AF=0 model now fails even if it matches Unicorn. No bit is masked.
The checker also observes FLAGS/DI immediately after the two live SHLs
(133 observations at `+1100` and 72 at `+1144`). Four negative
controls reject AF=0 and whole-word PF independently at both sites;
see the [folded-flags evidence](U3C6_10A7_FOLDED_FLAGS.md).

The cases cover:

- 120 random worlds: slots, Q present or absent, types, positions, shapes,
  map cells, occupancy, ranks, 7-byte entries, flags and stacks;
- every gate deterministically;
- both SP bounds, a random background, and preserved DF/IF;
- a rewritten FillChar site;
- the loop 1 cap and an exact cap;
- both `+03A0` budgets;
- the `+03A0` return alias;
- slot 64 with DI=0100h, distinguishing low-byte stored PF from whole-word parity.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **48
cases** with no mask. The fresh 2026-10-23 run explicitly selected the pinned
normal/386 build, source revision `5a9c7c53481c2e933948039c33256776653f4faf`,
binary SHA-256 `c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`.
Its existing local instrumentation is described in the
[folded-flags evidence](U3C6_10A7_FOLDED_FLAGS.md); the report records the executable hash.

- **Loading.** The COM copies unit `0x3C6` to paragraph `3000`. Both
  FillChar sites are relocated to `7800:16A8`, which holds the SYSTEM span.
- **Each case.** The COM rebuilds the DS segment, the map window and the
  record segment, writing in the audit's own order; the slot table overlaps
  the entries, the map pointer and the grid. It then calls `+10A7` with
  IRQs masked.
- **Comparison.** Each case compares all 14 CPU words, a 512-byte stack
  window, the whole 64 KiB DS segment and a 700h-byte map window.
- **Results.** 29 cases return 0 and 19 return 1. All 29 slot-clearing
  returns match the model's AF = 1 unmasked.
- **Result.** The result SHA-256 is
  `6fa0411461f81622f35571719a7362b348a5a60f6f149c567be5e121206f7c86`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
cmake -S . -B resources/t44-10a7/cmake-ci -DBUILD_TESTING=ON -DCMAKE_C_FLAGS=-UNDEBUG
cmake --build resources/t44-10a7/cmake-ci --target poolrad_overlay_u3c6_caller_10a7_cpu_tests --parallel 2
ctest --test-dir resources/t44-10a7/cmake-ci -R '^poolrad_overlay_u3c6_caller_10a7_cpu_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u3c6_caller_10a7_cpu_audit.py --unicorn --native --negative-tests --dosbox-x "$PINNED_DOSBOX_X" --output resources/u3c6-10a7-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U3C6 `+10A7..+1297` (entry 18), composing entry 15 (folded), entry 16 (`+0CB9`), entry 5 (`+03A0`) and SYSTEM FillChar |
| `source_candidates` | `src/recovered/overlay_u3c6_caller_10a7_cpu.c` with the merged `overlay_u3c6_0cb9_cpu.c`, `overlay_u3c6_rebuild_03a0.c`, `overlay_u3c6_table_pair_0000.c` and `system_runtime_leaves_cpu.c` |
| `signals` | Exact hash and CFG; a whole-RAM/CPU differential with every callee live; full instruction and branch-outcome coverage; every stop kind; an independent focused C test; native synthetic execution that is unmasked, including the AF profile |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract and the declared SHL-AF profile, the same as the merged `+0CB9` |
| `review_notes` | Identity and strict credit are the verifying lane's call. Entry 15 is folded locally; its semantic API is unchanged. Field meanings and natural saved-route reach are not asserted |

No classifier, queue or count ledger is changed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_03C6_10A7 --require-current-main
```
