# Unit 0x1C +0x2B7E: ECL ADD NPC (opcode 0x36), translation evidence

**Scope.** A bounded model of the handler of ECL opcode `0x36` (ADD NPC, reference name) at GAME.OVR unit `0x1C`
`+0x2B7E`. Translation only, modeled hooks, not a strict translation. No parity, function-identity, or game-function
credit is claimed. The model is over caller-supplied hooks for the operand loader, the slot value, the add-NPC routine,
the live active far pointer, the host record bytes, and the two resident routines.

Notation: a far pointer is written `segment:offset`. Unit offsets are `+XXXX`. File offsets are `file 0xXXXXX`.

## Identity and bounds

| item | value |
| --- | --- |
| owner input | GAME.OVR (not committed; read at run time), sha256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| span | file `0x038C4` through `0x03948` inclusive; unit `+0x2B7E` through `+0x2C02` inclusive; 133 bytes; 43 instructions; one plain `retf` as the last instruction |
| span sha256 | `5b711971cd12bfbcd60d87f2ac6a6cb836292b4729229417fa73962d48efeca1` |
| bytes after span | the next routine's prologue (`push bp; mov bp,sp; sub sp,...`) at unit `+0x2C03` |
| ledger row | `overlay_function_ledger.csv` line 53: `0x1c:0x2b7e`, 133 bytes, 43 instructions, `retf` |
| ledger caller | ECL dispatch case for opcode `0x36` (ECL_CONTRACTS.md section 3) |
| callee identity | `0x7C:0x43` = unit `0x6F` entry 7 (`+0x1262`, `pr_u6f_1262_add_npc`, merged model) |

## Call contract and use points (instruction-level, from the decode)

| # | address | target | argument pushed | role in the model |
| --- | --- | --- | --- | --- |
| 1 | `+0x2B87` | `0x2B:0x2A` | count `2` (one word) | `load_operands(2)` (unit `0x25` entry 2) |
| 2 | `+0x2B8F` | `0x2B:0x25` | slot `1` | `slot_value(1)`, full AX word `v1` (unit `0x25` entry 1) |
| 3 | `+0x2B9B` | `0x7C:0x43` | `v1`, the full AX word whose low byte is the NPC id | `add_npc(v1)`; may retarget the active pointer |
| 4 | `+0x2BA3` | `0x2B:0x25` | slot `2` | `slot_value(2)`, full AX word `v2` |
| U1 | `+0x2BC0` | `les di,[0x5D92]` | class store | CLASS use point: `record[0x84] := ((low(v2) >> 1) \| 0x80)` |
| U2 | `+0x2BCF` or `+0x2BDB` | `les di,[0x5D92]` | flag store | FLAG use point: `record[0x10E] := (low(v1) == 0x18) ? 1 : 0` |
| U3 | `+0x2BE5`, `+0x2BE9` | `push [0x5D94]`, `push [0x5D92]` | resident pair | RES_0BB8 use point, then call 5 |
| 5 | `+0x2BED` | `0xBA:0x0BB8` | `segment:offset` read at U3 | `resident_0bb8(segment, offset)`; may retarget |
| U4 | `+0x2BF2`, `+0x2BF6` | `push [0x5D94]`, `push [0x5D92]` | resident pair | RES_0767 use point, then call 6 |
| 6 | `+0x2BFA` | `0xBA:0x0767` | `segment:offset` read at U4 | `resident_0767(segment, offset)` |

The active far pointer is `DS:0x5D92` (offset) and `DS:0x5D94` (segment). Each use point reads it at the moment of
use, after the preceding calls. A successful insertion by the add-NPC routine (`0x7C:0x43`, unit `0x6F` entry 7) can
select a new active record, and a resident can do the same, so the pointer is never taken from an entry snapshot.
Each use point resolves its `segment:offset` pair through `record_bytes` before any store or call; a pair the host
cannot resolve stops the handler there. The stores and calls already made are not undone.

Argument-word note: the value pushed for the add-NPC call is AX after the slot-1 return, because `mov al,[bp-5]`
restores the low byte the slot call returned and the high byte is untouched. The model therefore passes the full
slot-1 word. The callee reads its low byte as the NPC id (its evidence: `U6F_1262_ADD_NPC.md`).

## Model and status semantics

- `pr_u1c_op_add_npc(hooks, report)`. The order is loader(2), slot(1), add-NPC, slot(2), CLASS use point and store,
  FLAG use point and store, RES_0BB8 use point and call, RES_0767 use point and call.
- A loader status other than OK ends the handler before any slot is read, so no slot-1 callback runs after a loader
  failure. A BOUNDS loader status reports handler BOUNDS; COUNT reports ARGUMENT.
- A slot-1 BOUNDS, UNRESOLVED or ARGUMENT status ends the handler before the add-NPC call.
- A slot-2 BOUNDS, UNRESOLVED or ARGUMENT status ends the handler after the add-NPC call and before any use point. The
  add-NPC effect has already run.
- A use point whose `read_active` fails, or whose `record_bytes` returns NULL, reports handler UNRESOLVED and ends the
  handler. A CLASS failure makes no store. A FLAG failure leaves the class store in place and makes no flag store. A
  RES_0BB8 failure makes no call. A RES_0767 failure leaves the first resident's call in place.
- A HOOK status from a slot completes all writes and calls, then reports core_status HOOK.
- A null hook returns ARGUMENT before any call. `record_bytes` is asked for `0x11D` bytes at each use point, the character
  record frame (docs/formats/DOS_CHARACTER.md); a host that cannot provide that size returns NULL and the handler stops.
- Policy, not engine behaviour: the model does not call a resident with a `segment:offset` pair the host cannot resolve.
  The original makes that call without checking. This is a stated modeling precondition.

## Evidence

Revised successor of `bf9129a80b4d4e095f987481c8cd9d0901759437`, which read the active pointer once at entry.

| check | command (paths outside the repo) | result |
| --- | --- | --- |
| focused behaviour (strict warnings; Debug CMake target `poolrad_overlay_u1c_add_npc_2b7e_tests`) | `tests/recovered/test_overlay_u1c_add_npc_2b7e.c` | PASS: single-record full path; two-record insertion regression (old record A untouched; incoming class and flag stores on B; both residents receive B's `segment:offset`); retarget between the residents (first resident sees A, second sees C); invalid replacement at CLASS (no store, no call); invalid replacement at RES_0767 (first resident ran, class and flag stores stand, second not called); failed live read; loader failure before slot 1; flag and class edge cases; slot-position failures; HOOK continuation; argument failures; composed run on the real ECL loader |
| static decode audit (owner media) | `tests/overlay/ovl1c_add_npc_2b7e_static_test.py --game-ovr <owner GAME.OVR>` | PASS: span length and sha256 over file `0x038C4..0x03948` inclusive, 43 instructions tile the span, one `retf` last at `+0x2C02`, six calls in order, the slot, shift, or, store, compare, and both resident pushes at each use point |
| differential oracle, 400 cases, seed 1 | `tests/overlay/ovl1c_add_npc_2b7e_oracle.py --cases 400 --seed 1` | 400/400 equal; 2400 emulated calls; 229 add-NPC retargets; 237 `0xBA:0x0BB8` retargets; 155 cases where the second resident sees a pointer moved from the entry record; 0 stub arity mismatches; 7/7 mutants killed |
| differential oracle, 2000 cases, seed 2 | `tests/overlay/ovl1c_add_npc_2b7e_oracle.py --cases 2000 --seed 2` | 2000/2000 equal; 12000 emulated calls; 1155 add-NPC retargets; 1196 `0xBA:0x0BB8` retargets; 789 cases where the second resident sees a moved pointer; 0 arity mismatches; 7/7 mutants killed |

The oracle runs the 133 owner bytes under Unicorn at unit offset `+0x2B7E`, with the five callee targets stubbed. Three
far records (A at `0x3000:off`, B at `0x4000:off`, C at `0x5000:off`, each 0x11D bytes) are written into far memory; the
active pointer is written to `DS:0x5D92`/`DS:0x5D94` at entry. The add-NPC stub and the `0xBA:0x0BB8` stub may retarget
that pointer after logging their arguments, so the second resident's arguments and the later stores depend on the live
value. Compared per case: the call log (every resident with its `segment:offset`), the status (SP and BP balance and the
stub arity), and every byte of each record that differs from its input. The C model runs through
`tests/overlay/ovl1c_add_npc_2b7e_harness.c`.

Mutants (one edit each): class without the `| 0x80`; class shifted by 2; flag compared with 0x19; flag inverted;
add-NPC word reduced to its low byte; class byte stored at the flag offset; RES_0767 given the RES_0BB8 pair (a stale
resident pointer). All seven are killed on both seeds.

## Limits

- The loader, slot value, add-NPC routine, active pointer, record bytes, and residents are supplied as hooks. The
  composed test binds the loader and slot value to `pr_rec_ecl_load_operands` and `pr_rec_ecl_slot_value`; the oracle
  stubs them, so it checks the handler's calls, arguments and writes, not the loader's internals.
- The oracle stubs return OK slot statuses only, and it always supplies resolvable pointers. HOOK, BOUNDS, UNRESOLVED,
  ARGUMENT and unresolvable-replacement paths are covered by the focused test, not by the oracle.
- A retarget that lands between two use points with no call between them cannot occur in the original; the FLAG and
  CLASS sites are adjacent with no call, so the model reads the pointer twice, and the focused test checks the CLASS
  replacement and the RES_0767 replacement only.
- The residents `0xBA:0x0BB8` and `0xBA:0x0767` are callbacks; their bodies are not translated or checked here.
- The add-NPC callee is a hook in this model; the merged `pr_u6f_1262_add_npc` model is not bound in these tests.
- The ledger and classification registries (`overlay_function_ledger.csv`, `overlay_classification.csv`, the CHECKER_ROWS
  and evidence-kind files) are not updated, and the aperture census was not run.
- The model does not check the ECL operand grammar; operand values are whatever the loader returns.
- Not a strict full translation. No game-function credit or identity is claimed.
