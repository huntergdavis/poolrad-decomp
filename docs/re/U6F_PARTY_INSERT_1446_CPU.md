# U6F +1446: raw-CPU party insertion composing FillChar, set membership and U9B +0000

`pr_u6f_party_insert_1446_cpu` is a raw-CPU, full-state model of the
complete GAME.OVR unit `0x6F` routine `+1446..+157E`, the party
insertion. It composes the merged SYSTEM leaves `pr_system_fillchar_16a8`
and `pr_system_set_in_0dc0` and, behind the loaded stub `00AC:0025`, the
merged raw-CPU U9B composition `pr_u9b_entry_zero_0000`. It returns the
whole CPU14 and 1 MiB RAM through the routine's own `RETF 4`.

The earlier entries stay as they are:

- The [segment-view model](U6F_PARTY_INSERT_1446.md), the [20-bit RAM
  model](U6F_PARTY_INSERT_1446_DOS.md) and the [SS-local
  model](U6F_PARTY_INSERT_1446_SS.md) report list, slot and window
  effects. The SS-local model stops after pushing the U9B call frame.
  None of the three can return CPU14 through `RETF 4`.
- PR #614's table-driven step adapter `pr_u6f_1446_step` ([full-state
  replay](U6F_1446_FULLSTATE.md)) steps the same body for four recorded
  caller frames at fixed loaded segments, with a cold-dispatch callback.
  It claims no credit. Its evidence so far is the [cold-capture
  preflight](U6F_1446_COLD_CAPTURE_PREFLIGHT.md) and the replay.

This entry is a source-ordered translation for any segments. It adds the
whole-routine original-byte differential and native oracle that the VP
named as this lane's evidence gap on 2026-10-18 (i17719). Identity and
strict credit are the verifying lane's decision.

A 90-second `deja "u6f 1446 raw cpu full-state"` recall timed out with no
result. This work reuses the merged documents linked above, the [U9B
transfer map](U6F_U9B_TRANSFER_1574.md), and the [U9B composition
audit](U9B_ENTRY_ZERO_COMPOSITION.md). That audit's leaf-return alias
layout supplies the `CALLEE` case here. The original spans were rechecked
against the ignored images.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. The expanded image at load segment 0 places SYSTEM FillChar at `0AF8:16A8` (20 bytes, SHA-256 `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7`, `RETF 8` at `16B9`) and set membership at `0AF8:0DC0` (33 bytes, SHA-256 `753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c`, `RETF 6` at `0DDE`). The stub at `00AC:0025` is cold in the image: `CD 3F` with entry `0000` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U6F `+1446` | GAME file `0x1F42C..0x1F564`; **313 bytes, 99 instructions, six conditional branches**; `RETF 4`; SHA-256 `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245`. It is a unit entry, and `+19E3` near-calls it after `PUSH CS` |
| Set `+1426..+1445` | 32 bytes; SHA-256 `d22f8e3450655784853879c7ecfee4ff36192352f6ffe0d40593dee56f583350`; members exactly 0..7 |
| Far calls | `+14CA LCALL 0AF8:16A8`, `+14F2 LCALL 0AF8:0DC0` and `+1574 LCALL 00AC:0025`, all fixup-backed. The stub selects U9B `+0000..+01AE` (SHA-256 `260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71`, `RETF 4` at `+01AC`). Its leaves `+01AF`, `+0255` and `+0329` are entered by `PUSH CS; CALL` and each ends in its only `RETF 4`, at `+0252`, `+0326` and `+045B` |

## ABI and ordered effects

The routine is entered after a near `CALL` preceded by `PUSH CS`, or after
a far call. `SS:[SP]` holds the return IP, `[SP+2]` the return CS and
`[SP+4]`/`[SP+6]` the incoming record pointer P. After `PUSH BP; MOV
BP,SP; SUB SP,10h`:

1. `P^[0BFh] := FFh`. If the DS head (`5D96h`/`5D98h`) is nil, head := P
   and active (`5D92h`/`5D94h`) := P. Otherwise loop A walks to the node
   whose link (`+104h`/`+106h`) is nil and sets that link to P.
2. `FillChar(SS:BP-0Ch, 8, 0)` clears the marks. Loop B visits every node
   from the head. When a node's slot byte `+0BFh` is in the 32-byte set at
   `CS:1426h`, it sets `mark[slot] := 1` at `BP-0Ch+slot`. Any slot the
   set admits is stored, so a modified set lets the store reach the frame.
3. active := P, and the local rec := active, reread through `LES` and
   stored at `BP-10h`. `rec^[0BFh] := 0`; then loop C adds 1 to
   `rec^[0BFh]` while the value is below 8 and its mark is set.
4. `INC WORD` at window1 (the far pointer at `DS:49D6h`) `+67Ch`. Then
   `PUSH P`, reread from the frame. `LCALL 00AC:0025` enters the loaded
   stub, whose `EA` jumps to U9B `+0000`. Then `MOV SP,BP; POP BP;
   RETF 4`.

Every operand is reread from RAM in source order, so records, DS globals,
window1, the set and the frame may alias. The slot push keeps AH from the
previous AX. Each `LCALL`'s operands are read before its pushes, as the
CPU fetches them. The stub is read after them, when the CPU reaches it.
The set pointer's `1426h` immediate stays absolute under a relocated
entry.

**Loop bounds.** A cyclic list makes loop A or loop B endless.
`max_loopbacks` caps their taken `JMP +1487` and `JMP +14DC` together. The
next one stops at that loop head with `LIMIT`, and every earlier effect is
kept. Loop C needs no cap:

- Each pass that continues has just read `rec^[0BFh]` below 8.
- Nothing in the pass writes before the `INC`, so the `INC` raises that
  same byte by one and changes no other byte.
- No byte is ever lowered, so each byte is raised at most eight times.
- The target byte changes only when the `INC` lands on one of the four
  pointer bytes at `BP-10h`, which can happen at most 32 times.

So loop C takes at most 33 × 8 = 264 loop-backs.

**Stops.**

- `CODE` has two forms:
  - A reached `LCALL` is not `9A` with its expected offset. The model
    stops at the site with nothing pushed.
  - The stub, read after the call's pushes, is not `EA 0000`. A cold
    `INT 3Fh` stub or a jump to another U9B entry are examples. The model
    stops at the stub, `00AC:0025` as read from the site, with the far
    return frame pushed.
- `CALLEE`: the U9B model stopped, and `*callee_status` holds its status.
  The audit's case is U9B's own `TRANSFER`: a U9B leaf's `RETF 4` reads a
  frame that the leaf overwrote through a record on the stack.
- `TRANSFER`: a callee's `RETF` came back somewhere other than the call's
  next instruction. In one audit case U9B clears record `+2Dh`, which is
  its own return IP. In the other, FillChar's frame straddles `SS:FFFFh`,
  so its CS is read physically contiguous, past the segment.
- `LIMIT`: the host cap on loops A and B.

`*callee` and `*callee_status`, either of which may be NULL, receive the
last callee run and its status. If no callee ran, they receive
`NO_CALLEE` and −1.

**Flags.**

- The final flags are U9B's.
- At the FillChar site they are `OR`'s with a zero result. This comes from
  either the nil-head `OR` or the last loop A `OR`.
- At the set site they are loop B's `OR` of a nonzero cursor.
- At the U9B site and at the stub they are `INC WORD`'s, with CF clear.
  Both loop C exits leave CF clear, and `INC` keeps it.
- At `LIMIT` they are one of these: loop A's `OR` of a nonzero link, loop
  B's `XOR AH,AH`, or loop B's set test.
- `OR` and `XOR` leave AF clear, as Unicorn and DOSBox-X both do.
- The flags of `SUB SP,10h` and of loop C's `CMP` and `INC BYTE` are
  folded.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs,
`POP` and this routine's `RETF` frame are read physically contiguous.
Each callee keeps its own contract. Null RAM or CPU, or RAM that is not
exactly 1 MiB, returns `ARGUMENT` before any change.

**U9B's own contract.** U9B `+0000` has no cap. Its document notes that an
alias can change a local loop index and loop forever, and this
composition inherits that. While searching for a `CALLEE` layout, some
stack-placed records made the original spin inside U9B, at one of these
offsets:

- the 56-index scan at `+012B`;
- `+00BC`;
- the `+0255` leaf at `+027E`.

None was in this routine's body, and no audit case uses such a layout.

The saved first insertion reaches the stub cold (see the
[preflight](U6F_1446_COLD_CAPTURE_PREFLIGHT.md)). This model stops there
with `CODE`, because the overlay manager's `INT 3Fh` load is outside the
routine.

## Validation

**Focused C test.** `test_overlay_u6f_party_insert_1446_cpu.c` runs under
CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. The test uses no game bytes: only
the three `LCALL` encodings, the {0..7} set and the stub. It passes 248
checks.

Seven insertions run twice:

- The insertions are: an empty head, three nodes, duplicate slots,
  out-of-set slots, all eight slots occupied, and two seven-node gaps.
- With a cold stub, the stop after the U9B call's pushes must equal the
  merged `pr_u6f_party_insert_1446_ss` across the whole 1 MiB. All 14
  registers are predicted by hand. The list, marks, locals, counter and
  frames are checked.
- With the warm stub, the result must equal that state carried through
  `pr_u9b_entry_zero_0000` and the epilogue, across the whole 1 MiB and
  all 14 words.

The remaining checks cover:

- `LIMIT`, with exact registers and flags:
  - loop A, with a self-linked head;
  - loop B, with P linked back to the head;
  - an exact cap, a cap one short, and cap 0 on an empty head.
- `CODE` at each site with exact SP and flags, and a stub that jumps to
  another U9B entry.
- Three stack-alias stops:
  - `TRANSFER` from U9B's clobbered return;
  - `TRANSFER` from FillChar's straddling frame;
  - `CALLEE` from a U9B leaf's clobbered frame.

  The two U9B stops must equal the U9B model's own stop, carried from the
  cold-stub state.
- A relocated entry, with the set still at `CS:1426h`.
- A set read from RAM, where slot 12 marks the saved BP.
- The argument guards.

**Unicorn differential.** `tools/overlay/u6f_party_insert_1446_cpu_audit.py`
works as follows:

- It places the expanded START.EXE image at load segment 0, so both SYSTEM
  leaves run from their original bytes.
- It patches the stub to `EA 0000` and loads units `0x9B` and `0x6F`.
- It compares the entire 1 MiB RAM and all 14 CPU words, with no mask.

| Stops | Cases |
| --- | --- |
| Returns | 319 |
| `CODE` | 6 |
| `CALLEE` | 1 |
| `TRANSFER` | 3 |
| `LIMIT` | 4 |

The original decides each stop:

- **Return:** the fetch after its own `RETF 4`.
- **`CODE`:** the live site bytes at the site's fetch, or the live stub
  bytes at the stub's fetch.
- **`LIMIT`:** a fetch of a loop head straight from its `JMP`, past the
  cap.
- **`CALLEE`:** CS:IP after a U9B leaf's `RETF 4` that is not its `+0000`
  continuation.
- **`TRANSFER`:** CS:IP after a SYSTEM leaf's or U9B's `RETF` that is not
  the call's next instruction.

Coverage is 99/99 instruction starts and 12/12 conditional outcomes. The
three sites were fetched 331, 1,615 and 324 times. The cases cover:

- 300 random lists of 0–10 nodes, over random records, slots, windows,
  counters, stacks and flags (some seeded with AF set);
- all eight slots occupied, and one gap;
- out-of-set slots and an empty head;
- counter wrap and sign;
- stacks near both segment ends, and SS across A20;
- a random background, a relocated entry, and preserved DF/IF;
- each site rewritten, a cold stub, and a stub to `+01AF`;
- both `LIMIT` loops, an exact cap, a cap one short, and cap 0;
- these aliases:
  - loop A links P into the set, so a mark lands on the incoming pointer
    in the frame;
  - DS = SS, with the head words on the cursor;
  - window1's counter on the return IP;
  - the incoming slot byte on a mark, and on the local pointer;
  - record `+2Dh` on U9B's return IP;
  - FillChar's frame straddling `SS:FFFFh`;
  - the U9B leaf-return alias.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **160
cases** with no mask.

- **Code.** The COM copies unit `0x6F` to paragraph `8800` and relocates
  the three `LCALL` segment words: the SYSTEM leaves go to `7000` and the
  stub to `7F00`. It writes the loaded stub `EA 0000:8000` and copies unit
  `0x9B` to `8000`.
- **Data.** U9B's DS tables, a random 0x1106-byte blob, go to `1900:31FA`.
- **Each case.** It builds a list of 0..8 nodes plus P in a 4 KiB record
  arena at `6000`, over a random background. It sets head, active and
  window1, seeds the window counter and flags, and calls `+1446` with
  IRQs masked.
- **Comparison.** Each case compares all 14 CPU words, a 256-byte stack
  window, the DS head/active/window1 words and the whole arena.
- **Spread.** Every list length 0–8 and every selected slot 0–8 is
  reached:

  | Value | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
  | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
  | List length | 13 | 8 | 20 | 27 | 12 | 20 | 17 | 24 | 19 |
  | Selected slot | 36 | 29 | 29 | 22 | 18 | 8 | 8 | 6 | 4 |
- **Result.** The result SHA-256 is
  `0f26695436bba3719797b8c0b92aeab06e5e8a497b1dfb93ecd04c0ff0e12da6`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u6f_party_insert_1446_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u6f_party_insert_1446_cpu_audit.py --unicorn --native --output resources/u6f-1446-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U6F `+1446..+157E`, composing SYSTEM FillChar and set membership and, through the loaded stub `00AC:0025`, U9B `+0000` |
| `source_candidates` | `src/recovered/overlay_u6f_party_insert_1446_cpu.c` with `src/recovered/system_runtime_leaves_cpu.c` and the merged U9B `+0000`, `+0255` and `+0329` sources |
| `signals` | Exact hash and CFG; a whole-RAM/CPU differential with both SYSTEM leaves and U9B live; full instruction and branch-outcome coverage; every stop kind; an independent C test that is full-RAM equal to the merged SS-local model; native synthetic execution |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Identity and strict credit are the verifying lane's call. The segment-view, DOS-RAM and SS-local entries and the #614 step adapter stay as they are. Natural saved-route reach through a warm stub is unmeasured; the saved first insertion meets the stub cold |

No classifier, queue or count ledger is changed.
