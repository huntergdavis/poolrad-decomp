# U6C +3291 record unlink and release: complete raw CPU composition

This implements the complete U6C routine [3291,33D7), composing the merged raw
U6C `+0000` record release and the merged raw U3CE `+014E` bitmap-pair release
(resident `+2595`, SYSTEM FreeMem) unchanged, with no callbacks or skipped heap
effects. It unlinks the active record from a far-pointer list and releases it.
See the [source](../../src/recovered/overlay_u6c_record_unlink_3291_cpu.c),
[ABI](../../include/poolrad/recovered/overlay_u6c_record_unlink_3291_cpu.h),
[original-byte audit](../../tools/overlay/u6c_record_unlink_3291_audit.py) and
[native harness](../../tools/overlay/u6c_record_unlink_3291_native.py).

This is author evidence for independent review, not an accounting change. No
identity promotion, merge, ledger/manifest/checkpoint edit or credit is claimed
here, and no completion percentage is stated.

## Identity and extent

| Item | Measured value |
| --- | --- |
| Target | OVL_006C_3291, descriptor unit 006C, memory segment 0078, public entry slot 2 (stub 0078:002A) |
| Executable span | [3291,33D7), **326 bytes, 95 instructions**, first RETF |
| GAME.OVR file span | [117776,118102) |
| Body SHA-256 | 1e2161950d386c5df7394667e5d6079e692e551fc7610f1582ef06157528e74f |
| Exit | RETF 4 at +33D4 |
| Outgoing calls | `LCALL 03E9:002F` at +3321 and +337B (both segment words carry relocations); `PUSH CS / CALL +0000` at +334A and +33B2 (no relocation) |
| Incoming callers | seven fixup-backed far calls `LCALL 0078:002A`, listed below: units 1C (1), 20 (3), 33 (1), 6A (2) |

Incoming fixup-backed sites, as printed by the audit (unit, site):
`(0x1C,0x394) (0x20,0x6EA) (0x20,0x748) (0x20,0x1239) (0x33,0x1DCF) (0x6A,0x1935) (0x6A,0x19E5)`.
These are the same seven the queue records, so the ownership evidence is
relocation-backed call sites, not an aligned `PUSH CS` count.

### Metadata repair: the queue row's 342 bytes include 16 trailing data bytes

The queue row reports 342 bytes (339 decoded). The decoded first-RETF span is
**326 bytes**. The 16 bytes [33D7,33E7) are `00 0E "Head Body Keep"` (a Pascal
string), SHA-256 `ae794194e8f8761497e9e1d18fba40bf96a7d2bfad78e460ce39a29e2d170ee7`.
They are excluded from the body, the coverage and the identity claim. The audit
asserts the extent, the body hash, that every branch target lies inside the span,
and the RETF 4. This is a candidate for the ledger's executable-span correction
list; this document does not edit that list.

| Private media | SHA-256 |
| --- | --- |
| START.EXE | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |

Queue snapshot read in place (not regenerated, not copied): `queue.json`
SHA-256 `c0b0ce1b6d82c91261902976728492297f3dc41e7639512dc78171b4c4f96c1b`,
generated on main f04d610e. It lists this row `game_evidenced`, grade A,
callees `OVL_006C_0000` and `OVL_03CE_014E` both `strict`, composable, prior
status `source_mention` only (no existing translation).

## Behavior and profile

The frame is a far call: `[SP]` return IP, `[SP+2]` return CS, `[SP+4]` a word
whose low byte is B6 (`SS:BP+6`), `[SP+6]` a word whose low byte is B8
(`SS:BP+8`); only the low bytes are read. The DS globals are the list head
`5D96/5D98`, the active record `5D92/5D94` and the window pointer `49D6`. A
record's next link is at +104h/+106h and its slot byte at +0BFh.

1. Head nil: return. Otherwise copy the head to `BP-4`.
2. Head equals active (segment compare first, then offset): take the head case.
   Otherwise walk, rereading `BP-4`, to the node whose next link equals active.
   A nil next link ends the walk and returns with nothing freed.
3. Walk case: if B6, `LCALL 03E9:002F` with the active record's +0BFh byte
   (pushed as a word, high byte stale); `prev.next := active.next`; free the
   active record with `PUSH CS / CALL +0000`; `active := prev`.
4. Head case: if B6, the same `LCALL`; `head := active.next`; free the active
   record; `active := the old next link`.
5. Both: if B8 is zero, `DEC WORD [window+67Ch]`. Return with `RETF 4`.

The model mirrors this in source order, rereading every operand from RAM, so
records, globals, the frame and the heap may alias. Words and LES pairs are
physically contiguous, including offset FFFF.

**Profile (every limit the evidence relies on).** Exact 1 MiB A20-off RAM, CPU14
(`CPU`, limits and diagnostic are distinct host objects outside RAM), entry
IP=3291, entry SP in [008C,FFF8] (the lower bound keeps the reached `+014E`
frame at or above 0080 and the `+0000` frame at or above 0040; the upper bound
keeps the return frame nonwrapping), code immutable, no interrupts, faults or
self-modification. The callees' own contracts apply to what they reach: heap
well-formedness is the SYSTEM FreeMem contract, the resident and SYSTEM
segments of the `+014E` closure are passed in (`resident_cs`, `system_cs`) and
must satisfy `+014E`'s published contract, and `+0000` reads its FreeMem
segments live. Flags follow the DOSBox-X normal/386 convention already used by
the callee models (OR/CMP/DEC AF as the oracle).

## The stub is declared

`LCALL 03E9:002F` reaches the overlay-manager entry stub of unit 3CE. The model
follows the merged `U6F +1446` convention: the call operands are checked before
the pushes, and the stub is read after them, when the CPU reaches it. A stub
that is the **loaded** `EA` far jump to `+014E` (offset 014E, any target segment
taken from the stub) is followed; anything else, a cold `INT 3Fh` stub included,
is a `CODE` stop at the stub with the call's pushes done. The overlay manager
itself is not modelled and no claim is made about it. In the Unicorn differential
and the native replay the stub is **synthetic**: the harness places
`EA 4E 01 <U3CE segment>` at a free segment. The cold-stub and wrong-offset stubs
are exercised as CODE stops in the differential only.

## Stops, named

| Stop (receipt name) | Meaning | Differential cases |
| --- | --- | ---: |
| `return` | RETF 4 taken | 1,348 |
| `CODE_3321_stub`, `CODE_337B_stub` | malformed `LCALL 03E9:002F` (opcode or offset), or the stub is not a loaded `EA` to +014E; walk site / head site | 4 / 3 |
| `CODE_334A_near`, `CODE_33B2_near` | malformed `PUSH CS` / `CALL +0000` (opcode, `PUSH CS`, or displacement); walk site / head site | 3 / 2 |
| `CALLEE_pair_2_rs2_in2` | `+014E` status 2: resident status 2, FreeMem RunError204 | 148 |
| `CALLEE_pair_2_rs2_in4` | `+014E` status 2: FreeMem heap-walk LIMIT | 9 |
| `CALLEE_record_2_in-1` | `+0000` CODE: its own malformed FreeMem call site, before any FreeMem call | 1 |
| `CALLEE_record_3_in2` | `+0000` status 3: FreeMem RunError204 | 36 |
| `CALLEE_record_3_in4` | `+0000` status 3: FreeMem heap-walk LIMIT | 5 |
| `CALLEE_record_4_in0` | `+0000` TRANSFER: a FreeMem far return changed | 3 |
| `CALLEE_record_5_in0` | `+0000` list budget (LIMIT) | 7 |
| `TRANSFER_334A_near`, `TRANSFER_33B2_near` | `+0000` returned to a CS:IP other than the call's next instruction | 1 / 2 |
| `LIMIT` | walk budget at the taken `JMP +006F->+0034` | 7 |
| ARGUMENT | host-object or profile violation; changes nothing | C test only |

ARGUMENT, and `+014E`'s own ARGUMENT (binding mismatch, surfaced as CALLEE), are
outside the profile and are not compared against the original. The walk budget
counts taken loop-backs; zero stops at the first loop-back.

## Measured evidence: original-byte differential

Unicorn 2.1.4 executes the original caller, `+0000`, `+014E`, resident `+2595`
and SYSTEM FreeMem bytes, with the loader-expanded START image for the resident
and SYSTEM spans. After every case **all 14 CPU words and every byte of the
1 MiB RAM image** are compared against the C model, with **zero masks and zero
emulator substitutions**, plus the stop kind, the callee identity and status,
the call site, both callee diagnostics and the call counts.

- **1,579 cases**: 1,348 returns, 231 named stops (table above).
- Caller: **95/95 instruction starts, 22/22 conditional outcomes**. Outcomes are
  accounted by successor: each conditional is credited per taken or fall-through
  successor actually executed next.
- Combined closure (caller + `+0000` + `+014E` + resident + FreeMem):
  **366/366 starts, 72/72 conditional outcomes**, no exclusions. The checker
  **asserts** the caller and the closure coverage and the equality of the
  incoming-caller list with the seven above, so a later model change cannot drop
  a case silently.
- Cases: the hand set (head nil, head equals active, walks to depth 2, active
  absent, B6/B8 combinations with garbage high bytes, children, merges, top
  shrink, invalid record and bitmap pointers, walk limit and cycle, cold and
  wrong stubs, malformed call sites, relocated SYSTEM, aliased window, `SS=DS`,
  A20 stack, SP edges, heap-resident stack sweeps), 360 random scenarios and one
  random-RAM image, the resident `+2595` audit's targeted cases and the `+0000`
  audit's targeted cases, both reached through this caller, and a DS=0/SS=0
  sweep (below).
- **Fixtures are asserted to reach their path, not trusted by name.** The bitmap
  table defaults to nil, so a fixture that does not provide bitmaps makes
  `+014E` a no-op and execution proceeds to the code the fixture is about (an
  earlier draft left the table at the fill byte, which made `+014E` fail first and
  hid the `+0000` and call-site paths). The audit carries an expected-stop map and
  fails if a named fixture stops elsewhere: the four malformed `LCALL` fixtures stop
  `CODE_3321_stub`/`CODE_337B_stub`; the five malformed near-call fixtures run with
  B6=0 and stop `CODE_334A_near`/`CODE_33B2_near`; the malformed `+0000` site with
  B6=0 stops `CALLEE_record_2`; the cold and wrong-offset stubs stop `CODE_*_stub`;
  the budgets stop `LIMIT` / `CALLEE_record_5`. The receipt records every named
  fixture's measured stop (`named_stops`).

**One closure outcome needed an argued case.** Resident `+25A5->+25A7` (nil slot)
looks infeasible because `+014E` tests the table entry before the call, but it is
feasible: with DS=0 and SS=0 the callee's `PUSH DS` writes 0 over the tested
slot's offset word while the segment word is `+014E`'s own zero local, so the
resident reads a nil slot. The merged `+014E` audit has the same alias. The sweep
sets the slot to (seg 0, off 1000h) and covers entry SP around `5E6C`
(`slot + 18`); the case closes the outcome.

The C suite passes **76 checks under ASan/UBSan**: head nil with its flags, the
single-node and successor head cases, the walk relink and window decrement, the
absent active record, the stub call with nil tables, the walk LIMIT, the stub and
call-site CODE stops, an unfreeable record, and the ARGUMENT guards with state
unchanged. Their expectations were derived by hand from the disassembly. The
focused CTest target `poolrad_u6c_record_unlink_3291_cpu_tests` builds from CMake and passes 1/1. Five C
sources are compiled; the `+0000`, `+014E`, resident and FreeMem sources are
unchanged.

## Measured evidence: native DOSBox-X

DOSBox-X normal/386 (`dosbox-x 2026.01.02` SDL2 from PATH, the invocation the merged
native harnesses use; **not** the pinned 5a9c7c53 source build) runs the original
caller, `+0000`, `+014E`, resident `+2595` and SYSTEM FreeMem bodies as real code
from a synthetic COM image: the harness stages each body at its segment, pushes
B6 and B8, flags and registers, and `CALL`s the entry. Only the external FreeMem
RunError entry is patched, to capture its boundary; it is not part of the compared
closure.

- **144 cases: 131 returns and 13 RunError204 stops.** All 14 CPU words, the
  1,024-byte stack window, the full 65,536-byte DS segment and the 65,536-byte heap
  arena agree with the C model, **zero masks: 132,096 compared RAM bytes**.
- **Excluded from the native comparison: the other 916,480 bytes** of the
  1 MiB address space (all of them outside the captured windows and the staged
  code), because real DOS memory there is not reproducible by a flat-fill model.
  A case is native-eligible only if the C model, run on two RAM images whose bytes
  outside the windows differ (A5 versus 5A), gives identical status, CPU and window
  contents and leaves the outside untouched; that test kept 1,392 of 1,396 candidates,
  and 144 are replayed (every named case plus an even sample of the translated and
  random cases).
- Not replayed natively, by construction: cases with code writes, relocated or
  per-site SYSTEM segments, non-default stubs, a non-default frame, DS or SS,
  non-default caps, cyclic/limit cases, heap-resident stack sweeps and the DS=0
  sweep, and every stop that is not a return or a FreeMem RunError (CODE, LIMIT,
  TRANSFER, budgets). Those are covered by the original-byte differential only.
- The **stub is synthetic**: the harness stages the loaded `EA 4E 01 <U3CE
  segment>` at a free segment. This is synthetic execution, not natural gameplay
  reachability, and it is not a claim about the overlay manager.
- RESULT.BIN SHA-256: `019c0bc25372f45ca911abbaff27a2d95564c1810552061a3bf65cc1e1b95cea`.

## Exclusions and what is not claimed

- ARGUMENT, `+014E` ARGUMENT, SP outside [008C,FFF8], non-immutable code,
  interrupts and faults are outside the profile.
- The overlay manager is not modelled; the stub is synthetic in every execution.
- Native replay is synthetic frames and synthetic RAM, not natural gameplay
  reachability. It is not full-memory evidence (see the native section for the
  compared byte counts and the excluded ranges).
- No gameplay reachability claim, no completion percentage.

## Readiness counterfactual (measured on the pinned queue, not credit)

`OVL_0020_11AC` lists this row as its only unready callee (1 of 2); crediting a
strict translation of `+3291` makes that caller composable. Six further
candidates list it among other unready callees (`OVL_001C_02E0`, `OVL_006C_0161`,
`OVL_0033_19F5`, `OVL_006C_1F37`, `OVL_006A_18E5`, `OVL_0020_04C9`) and stay
gated. No manifest declaration is made here: this body has a raw-CPU endpoint, and
a `full` declaration is the reviewer's call.

## Checker

`tools/overlay/u6c_record_unlink_3291_audit.py` declares
`CHECKER_ROWS=('OVL_006C_3291',)` and implements `--compile-only` through the
same `library()` build (all five C sources, the real entry point bound), printing
`CHECKER_BUILD_OK`. **Gate run:** on main `e63a9bf7` with this branch,
`check_checker_builds.py OVL_006C_3291` reports `PASS: 1 actual checker builds for
1 rows`, and `check_body_doc_pairing.py --base e63a9bf7` reports every body and
checker paired with evidence and registration. `--compile-only` needs no media.

## Reproduction and private receipt

Base: main `e63a9bf7` (the branch was rebased onto it; the composed `+0000`, `+014E`,
resident and FreeMem sources are unchanged from the previous base `f04d610e`).
Provide owner-supplied media in ignored `resources/DOS/POOLRAD/`. Capstone 5.0.7
and Unicorn 2.1.4 are required. Under the crew's host rule every step runs through
`/home/hunter/workspace/poolrad-heavy.sh`.

    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u6c_record_unlink_3291_audit.py --unicorn --output resources/original.json
    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u6c_record_unlink_3291_audit.py --native --output resources/native.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_overlay_u6c_record_unlink_3291_cpu.c src/recovered/overlay_u6c_record_unlink_3291_cpu.c src/recovered/overlay_u6c_record_release_0000_cpu.c src/recovered/overlay_u3ce_bitmap_pair_release_014e_cpu.c src/recovered/resident_0464_bitmap_release_2595_cpu.c src/recovered/system_freemem_0254_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test

The private receipt (`--unicorn --native --output`, generated at code commit `132a82c2`, which the PR head
carries unchanged apart from this document) has SHA-256 `abca845c6eb45e1660630c6bbec3e23693ff0da125af8be31867d493697aedf6`; it pins the media and
the composed-closure hashes, the Unicorn parity report with every named fixture's stop, and the native report.
A second run on the previous commit reproduced the same differential numbers and the same RESULT.BIN hash.

No original or extracted bytes and no generated capture binaries are committed.

## Selection, reuse and author assessment

Selected from the zero-unready rows of units 25/1C/6C/33 on the pinned snapshot.
Skipped for prior art: U1C `+2E7D` (merged CPU text-loop model), U33 `+08A4` and
`+0B97` (merged models). Dropped for no called-entry identity: the four 5-byte
entry-slot-0 stubs. Idioms follow the merged `+0000`, `+014E` and `U6F +1446`
models. No recall index was available in this session (the `deja` server failed to
connect), so no prior-session result was reused.

| Assessment field | Author finding |
| --- | --- |
| target_function | OVL_006C_3291, [3291,33D7) |
| source_candidates | New raw caller; unchanged `+0000` and `+014E` closures |
| signals | Body/fixup/seven-caller identity, full-state original differential, native replay, hand-derived semantic checks |
| verdict | likely_match within the documented bounded CPU profile |
| review_notes | Provisional internal-helper identity; 16 trailing bytes excluded; stub synthetic; no accounting edits or self-credit |
