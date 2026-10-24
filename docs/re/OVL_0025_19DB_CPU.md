# OVL_0025 +19DB: raw-CPU model of the party step (no new credit)

`OVL_0025_19DB` (unit 0x25 entry 19) is **already credited**. Its credit rests on the semantic translation `pr_u25_entry19`
([U25_MOVE_ENTRY19.md](U25_MOVE_ENTRY19.md)), checked against the original bytes by `u25_move_original_audit.py`. That model is a state-struct
API composing the translated 3BD entries, not a CPU.

This change adds the **raw-CPU, full-state model** the hybrid executor needs. The body is reached by the R4 shop and R4 expanded observers.
**No credit or ledger change is proposed.** Assigned by par-a (i22988), claimed in i22991.

**Pre-claim grep** (VP i22025, widened by PM i22086), on main 0979d90a: `(\+|0x|:)19DB\b` and the row ID over `src/recovered`,
`tools/overlay`, `include` and `docs`. Every hit was opened:
- the semantic `pr_u25_entry19` (`overlay_u25_move.h/.c`), its doc, its original-byte checker and shim;
- the ledger and classification rows, `classify_overlay.py`'s constant;
- the 3BD docs naming it as a caller; `u25_alloc_1951_cpu_audit.py` and `flag_fold_liveness.py`, which use 19DB only as the end bound of +1951.

No raw translation existed.

## Executable extent

GAME.OVR unit 0x25 **[19DB,1A65), 138 bytes / 50 instructions**, `RETF` at 1A64, no arguments. Two far calls, both through overlay stub 03D5
(unit 3BDh): `LCALL stub:0034` at 1A47 (slot 4, entry 4, +057A, the cell lookup, RETF 4) and `LCALL stub:002A` at 1A5B (slot 2, entry 2,
+047B, the nibble lookup, RETF 6). Their segment words (fixups 1A4A and 1A5E) are the unit's only relocations in the span.

## What the body does

1. `PUSH BP; MOV BP,SP`.
2. `MOV AL,[6AAF]` (the facing), then `CMP AL,0/2/4/6`:
   - 0: `CMP BYTE [6AAE],0; JLE` → y := 0Fh, else `DEC` y;
   - 2: `CMP BYTE [6AAD],0Fh; JGE` → x := 0, else `INC` x;
   - 4: the same for y; 6: as 0, for x;
   - any other facing moves nothing.
   The compares are signed, so 80h..FFh count as below 0.
3. `PUSH x; PUSH y` (each `MOV AL,[..]; PUSH AX`: AH is whatever AX held), the cell lookup, `[6AB1] := AL`.
4. `PUSH x; PUSH y; PUSH facing`, the nibble lookup, `[6AB0] := AL`.
5. `POP BP; RETF`.

## Raw contract and stops

`pr_u25_move_19db_cpu(bus, size, cpu, budget)`:
- exactly 1 MiB of A20-off RAM; IP 19DB; entry SP in `[0030h,FFFCh]`, so the callees' frames stay inside the stack segment;
- **each instruction of this body costs one unit of `*budget`**. The stub's `JMP FAR` and the callees do not; they are bounded;
- **the stub:** each `LCALL` reads its relocated segment word; the slot there must be the overlay manager's `JMP FAR` (EAh) to the expected
  3BD entry, which the model follows. **An unloaded slot (INT 3Fh) stops STUB at the LCALL, before its frame**, so the host can let the
  overlay manager load unit 3BDh and resume (the 0214 stub-precondition lesson);
- **the callees** are the raw models `pr_u3bd_cell_057a_cpu` (its map byte through the bus) and `pr_u3bd_nibble_047b_fullstate` (flat).
  **Declared exclusion:** +047B reads the map flat, so a map inside A000 is exercised only where +047B does not read it (an unmatched facing).

The flags follow the pinned DOSBox-X stored rules (CMP: the six arithmetic flags; INC and DEC keep CF). At RETURNED they are +047B's.

| Stop | Pinned by |
| --- | --- |
| RETURNED | Unicorn and native |
| LIMIT | Unicorn: the budget sweeps on two paths, with equal state and budget at the same unexecuted instruction; C test |
| STUB | Unicorn: slot 4, slot 2 and both unloaded; the oracle stops at the same LCALL. C test: an unloaded slot, and a loaded slot to another entry |
| TRANSFER | Unicorn: with SS = DS and SP at 6AB0h or 6AAFh, this body's own `MOV [6AB1],AL` / `MOV [6AB0],AL` writes into its return address, so RETF goes elsewhere in both runs |
| CALLEE | kept as a stop: a callee's ARGUMENT. Unreachable from a valid entry (the entry SP range covers the callees') |
| ARGUMENT | C test: null bus, CPU or budget; short RAM; IP; SP. CPU and RAM untouched |

## Checks

**Original-byte Unicorn differential: 410 cases, 0 mismatches.** It runs on `u25_move_original_audit.media()`: the relocated START.EXE at
1000h, units 25h and 3BDh loaded with every fixup applied, the stub loaded, plus each case's state bytes, map id, 1,024-byte map, stack residue
and registers, all from the case's own name. Compared after every case, **unmasked**: CPU14, **all 1,048,576 RAM bytes**, the EGA state and the
ordered VRAM write operands, the stop and **the remaining budget**.
- 349 RETURNED, 56 LIMIT, 3 STUB, 2 TRANSFER.
- **50/50 instruction starts, 16/16 conditional outcomes; 349 independent statements** (the position after the step from the
  stated signed-wrap rule, without the C model).
- **FLAGS:** exact, except where the last flag writer is a shift, DIV or MUL in a callee, which gets the declared pinned split: 93 states under the shift AF rule, 0 DIV, 0 MUL. They come from +047B: for a facing of 0 or 4 (selector 0 or 4), its `SHR AX,4` is the last flag writer before `RETF 6`, so the pinned AF=1 reaches 19DB's RETURNED state; raw Unicorn leaves AF clear.

Cases:
- 10 facings (0, 2, 4, 6 and six unmatched) × 9 edge values of x and of y;
- 5 map ids (0 and 0Ah close the gate) × 5 facings, in and out of the grid;
- 7 flag images on a move and on an unmatched facing; 6 entry SPs;
- the map pointer wrapping at offset FFFFh (in segment 5000h) and across A20. A first round placed the wrap at 9400:FF00, which reaches A3FEFh: there
  +047B's flat read and the original's EGA read differ, which is the declared exclusion, so those cases moved below A000;
- this body's return address under the state bytes (SS = DS), reaching TRANSFER;
- **the map in the EGA aperture** (three placements; +057A reads it through the bus and the independent EGA);
- unloaded stub slots; the map over each byte of the frame;
- budget sweeps on two paths; 60 random cases.

**Mutation: the unmutated model passed the same 30-case subset first (5 LIMIT, 21 RETURNED, 2 STUB, 2 TRANSFER; PM rule i22687), then **12/12 caught**, none unbuilt. The first round left one survivor, "the return-address check dropped", a case gap: nothing in the subset rewrote this body's own frame. `ret-under-6ab0` and `ret-under-6aaf` (SS = DS, so the state-byte stores land on the return address) were added, and they catch it. That round's baseline also passed.**

**Native (synthetic DOSBox-X normal/386; packaged DOSBox-X 2026.01.02, sha256 99d74601, the same release as pinned 5a9c7c53):** **51 cases, 16,713,108 bytes compared, 0 mismatches, FLAGS exact**.
The six segments are unit 25h's code, the loaded stub, unit 3BDh's code (no relocation in the bytes used), the data segment, the stack and the
map. The two LCALL segment words are written as the stub segment, as the loader's relocation does. The five segments after the code and the
14 CPU words are compared, FLAGS exact.

**C test** (CHECK macro, independent of NDEBUG; 89 checks, also run with `-DNDEBUG`):
- the step rule for the four facings × six edge values; the cell result; an unmatched facing;
- STUB for an unloaded slot (after the step, before the frame) and for a slot to another entry;
- the budget; budget 0; the guards.

**Census:** one new row, `58fbca52…` (`incdec8`, INC/DEC r/m8 keeping CF), classified `agree` with the pinned citations (`instructions.h:194-221`, `flags.cpp:154-162,373-381`). The byte compare hashes to the existing row `e047db03…`, now listing this file beside +057A. The helpers' comments sit above their signatures, so `flag_census.py` sees them: a comment between `)` and `{` hid a helper from the census (reported to the PM, i23004; fixed by wR:p1's #1540, on which this branch is rebased).

**Aperture census:** a new row `overlay_u25_move_19db_cpu.c,bus,yes-with-ega-oracle,yes`, and the (caller, flat callee) pair with
`pr_u3bd_nibble_047b_fullstate` records the declared exclusion.

**credit_evidence_kind.csv:** regenerated. `OVL_0025_19DB` now lists this checker beside `u25_move_original_audit.py`. That touches a
credit table, but **the credit itself is unchanged**.

**Callee flag note:** wR:p1's census fix (#1540) records one DISAGREE in the composed callee: +057A's `shl16` OF rule (`SHL AX,CL` at 05D8).
19DB cannot observe it. In +057A, `ADD DI,AX` and `ADD DI,DX` (05DE, 05E0) always follow that SHL and overwrite every arithmetic flag
before `RETF 4`, and this audit's stops (budget, STUB, RETURNED) are all inside 19DB's own body.

## What this does not show

- Credit or identity: unchanged.
- The hybrid glue: par-b's next step, after merge. On STUB the glue must let the overlay manager load unit 3BDh.
- A map inside A000 read by +047B: declared exclusion.

## Reproduction

    python3 tools/overlay/ovl_0025_19db_cpu_audit.py --unicorn --mutate --native --output resources/u25-19db-evidence.json
    python3 tools/overlay/ovl_0025_19db_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main OVL_0025_19DB
