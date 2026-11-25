# Unit 0x1C +0x2B7E: ECL ADD NPC (opcode 0x36), translation evidence

**Scope.** A bounded model of the handler of ECL opcode `0x36` (ADD NPC, reference name) at GAME.OVR unit `0x1C`
`+0x2B7E`. Translation only: no parity, function-identity, or credit claim. The model is over caller-supplied
callbacks for the operand loader, the slot value, the add-NPC routine and two resident routines, and over a
caller-supplied far record buffer.

## Identity and bounds

| item | value |
| --- | --- |
| owner input | GAME.OVR (not committed; read at run time), sha256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| span | file `0x038C4`, unit `+0x2B7E..+0x2C02`, 133 bytes, 43 instructions, one plain `retf` as the last instruction |
| span sha256 | `5b711971cd12bfbcd60d87f2ac6a6cb836292b4729229417fa73962d48efeca1` |
| bytes after span | the next routine's prologue (`push bp; mov bp,sp; sub sp,...`) at `+0x2C03` |
| ledger row | `overlay_function_ledger.csv` line 53: `0x1c:0x2b7e`, 133 bytes, 43 instructions, `retf` |
| ledger caller | ECL dispatch case for opcode `0x36` (ECL_CONTRACTS.md section 3) |
| callee identity | `0x7C:0x43` = unit `0x6F` entry 7 (`+0x1262`, `pr_u6f_1262_add_npc`, merged model) |

## Call contract (instruction-level, from the decode)

| # | address | target | argument pushed | role in the model |
| --- | --- | --- | --- | --- |
| 1 | `+0x2B87` | `0x2B:0x2A` | count `2` (one word) | `load_operands(2)` (unit `0x25` entry 2) |
| 2 | `+0x2B8F` | `0x2B:0x25` | slot `1` | `slot_value(1)`, full AX word `v1` (unit `0x25` entry 1) |
| 3 | `+0x2B9B` | `0x7C:0x43` | `v1` (the full AX word; low byte is the NPC id) | `add_npc(v1)` |
| 4 | `+0x2BA3` | `0x2B:0x25` | slot `2` | `slot_value(2)`, full AX word `v2` |
| 5 | `+0x2BED` | `0xBA:0x0BB8` | far pointer `[0x5D92]:[0x5D94]` (segment pushed first) | `resident_0bb8(seg, off)` |
| 6 | `+0x2BFA` | `0xBA:0x0767` | the same far pointer | `resident_0767(seg, off)` |

Writes: record byte `+0x84` := `((low(v2) >> 1) | 0x80)`, and record byte `+0x10E` := `1` when `low(v1) == 0x18`,
else `0`, both through the far pointer `[0x5D92]:[0x5D94]`. The slot-2 value is zero-extended (`xor ah,ah`) before
the shift. The first store follows the add-NPC call; the second follows the store of `+0x84`. The two residents
follow both stores.

Argument-word note: the value pushed for the add-NPC call is AX after the slot-1 return, because `mov al,[bp-5]`
restores the low byte the slot call returned and the high byte is untouched. The model therefore passes the full
slot-1 word. The callee reads its low byte as the NPC id (its evidence: `U6F_1262_ADD_NPC.md`).

## Model and status semantics

- `pr_u1c_op_add_npc(hooks, record, report)`: the order is loader(2), slot(1), add-NPC, slot(2), record store
  of `+0x84`, store of `+0x10E`, resident `0xBA:0x0BB8`, resident `0xBA:0x0767`.
- A loader status other than OK ends the handler before any call. A BOUNDS loader status reports handler
  BOUNDS; COUNT reports ARGUMENT.
- A slot-1 BOUNDS, UNRESOLVED or ARGUMENT status ends the handler before the add-NPC call.
- A slot-2 BOUNDS, UNRESOLVED or ARGUMENT status ends the handler after the add-NPC call and before any record
  store or resident call. The add-NPC effect has already run.
- A HOOK status from a slot completes all writes and calls, then reports core_status HOOK.
- A null hook, a null record, or a record smaller than `0x11D` returns ARGUMENT before any call. `0x11D` is the
  character record frame (docs/formats/DOS_CHARACTER.md), so the model's minimum is the full frame.

## Evidence

| check | command (paths outside the repo) | result |
| --- | --- | --- |
| focused behaviour (strict warnings, Debug CMake target) | `poolrad_overlay_u1c_add_npc_2b7e_tests` | PASS: full path, flag edge cases (id 0x18 with AH set), class boundaries (only the low byte of slot 2 counts), loader/slot status paths, HOOK continuation, argument failures, composed run on the real loader |
| static decode audit (owner media) | `tests/overlay/ovl1c_add_npc_2b7e_static_test.py --game-ovr <owner GAME.OVR>` | PASS: span length and sha256, 43 instructions tile the span, one `retf` last, six calls in order, the slot, shift, or, store, compare and resident-push instructions |
| differential oracle, 400 cases, seed 1 | `tests/overlay/ovl1c_add_npc_2b7e_oracle.py --cases 400 --seed 1` | 400/400 equal; 2400 emulated calls; 130 add-NPC calls with id 0x18, 270 with another id; 0 stub arity mismatches; 6/6 mutants killed |
| differential oracle, 2000 cases, seed 2 | `tests/overlay/ovl1c_add_npc_2b7e_oracle.py --cases 2000 --seed 2` | 2000/2000 equal; 12000 emulated calls; 614 with id 0x18, 1386 with another id; 0 arity mismatches; 6/6 mutants killed |

The oracle runs the 133 owner bytes under Unicorn at unit offset `0x2B7E`, with the five callee targets stubbed. The stubs log
each call's arguments, return the case's slot words, and pop their arguments with `RETF`. Compared per case: the call
log, the status (SP and BP balance and the stub arity), and every record byte that differs from its input. The
C model runs through `tests/overlay/ovl1c_add_npc_2b7e_harness.c`.

Mutants (one edit each): class without the `| 0x80`; class shifted by 2; flag compared with 0x19; flag inverted;
add-NPC word reduced to its low byte; class byte stored at the flag offset. All six are killed on the seed-2 batch.

## Limits

- The loader and slot value callbacks are supplied by the caller. The composed test binds them to
  `pr_rec_ecl_load_operands` and `pr_rec_ecl_slot_value`, but the oracle stubs them: the oracle checks the handler's
  calls, arguments and writes, not the loader's internals.
- The oracle stubs return OK statuses only. HOOK, BOUNDS, UNRESOLVED and ARGUMENT paths are covered by the
  focused test, not by the oracle.
- The far record pointer is read once on entry by the model. A callee that rewrites `DS:0x5D92` or `DS:0x5D94` during the
  add-NPC call or the residents is not modelled; the original re-reads the pointer for the resident pushes.
- The residents `0xBA:0x0BB8` and `0xBA:0x0767` are callbacks. Their bodies are not translated or checked here.
- The add-NPC callee is a callback in this model; the merged `pr_u6f_1262_add_npc` model is not bound in these tests.
- The ledger and classification registries (`overlay_function_ledger.csv`, `overlay_classification.csv`, the CHECKER_ROWS and
  evidence-kind files) are not updated by this change, and the aperture census was not run.
- The handler's operand values are whatever the loader returns; the model does not check the ECL operand grammar.
- Not a strict full translation. No game-function credit or identity is claimed.
