# OVL_007F +0130 and +0185: raw-CPU models of the div 5 leaves (no new credit)

`OVL_007F_0130` (unit 0x7F entry 10) and `OVL_007F_0185` (entry 11) are **already credited**. Their credit rests on the semantic
translations `overlay_u7f_div5.c` ([U7F_DIV5_ENTRY10.md](U7F_DIV5_ENTRY10.md)) and `overlay_u7f_div5_dwords.c`
([U7F_DIV5_DWORDS_ENTRY11.md](U7F_DIV5_DWORDS_ENTRY11.md)), whose evidence is their unit tests. Those are record and block APIs, not CPUs.

This change adds the **raw-CPU, full-state models** the hybrid executor needs. Both bodies are reached by the R4 shop and R4 expanded observers.
**No credit or ledger change is proposed.** Assigned by par-a (i22988), claimed in i22991.

**Pre-claim grep** (VP i22025, widened by PM i22086), on main 0979d90a: `(\+|0x|:)0130\b`, `(\+|0x|:)0185\b` and the row IDs over
`src/recovered`, `tools/overlay`, `include` and `docs`. Every hit was opened:
- the semantic translations, their docs, the ledger and classification rows, `classify_overlay.py`'s constants;
- unrelated `+0130` / `+0185` offsets in other units' models (u25 loader, u3b0, u33, u3c2, u9e, 03f1), an `ecl_helpers.c` comment, the u7f +00A8 END bound,
  and two audits' fixture offsets.

No raw translation existed.

## Executable extent

GAME.OVR unit 0x7F, from `overlay_inventory.find_units` (ignored `resources/`); no relocation in `[0130,01DB)`:
- **+0130 (entry 10): [0130,0185), 85 bytes / 31 instructions**, `RETF 2` at 0182;
- **+0185 (entry 11): [0185,01DB), 86 bytes / 34 instructions**, `RETF 2` at 01D8. The 40 bytes after it are +0203's literal, never read here.

No call, interrupt or port in either. Both take one pushed word, the value at `[BP+6]`.

## What the bodies do

**+0130:** `PUSH BP; MOV BP,SP; SUB SP,2`; the byte `[BP-1]` counts 0..4 (a do-while: `JMP` over the first `INC`, `CMP [BP-1],4; JNE`).
Each pass: `CBW; SHL AX,1; LES DI,DS:[5D92]; ADD DI,AX; XOR AX,AX`; the word `ES:[DI+88h]` := 0. Then the value `DIV 5`: `LES` again,
`ES:[DI+90h]` := the quotient; once more `DIV 5; XCHG DX,AX; LES`, `ES:[DI+8Eh]` := the remainder. `MOV SP,BP; POP BP; RETF 2`.

**+0185:** the same frame and loop. Each pass: `CBW; MOV DI,AX; SHL DI,1; SHL DI,1; XOR AX,AX`; the words `DS:[DI+67F4h]` and `DS:[DI+67F6h]`
:= 0. Then `DIV 5; XOR DX,DX`; `DS:[6804]` := the quotient, `DS:[6806]` := 0; once more `DIV 5; XCHG DX,AX; XOR DX,DX`; `DS:[6800]` := the
remainder, `DS:[6802]` := 0. `RETF 2`.

## Raw contract and stops

`pr_u7f_div5_0130_cpu(bus, size, cpu, budget)` and `pr_u7f_div5_0185_cpu(...)`:
- exactly 1 MiB of A20-off RAM; IP at the entry; entry SP in `[0004h,FFFAh]`;
- **each instruction costs one unit of `*budget`**;
- **the bus:** +0130's record (the far pointer DS:[5D92]) is written through `pr_bus`; the stack and DS globals are flat. +0185 is all flat.

The flags follow the pinned DOSBox-X stored rules:
- **a word DIV writes the six arithmetic flags**: AF=SF=OF=0; ZF = remainder 0 and quotient odd; CF = (remainder AND 3) in 1..2;
  PF = even parity of the four bytes of remainder:quotient. In +0130 the second DIV is the last flag writer at RETURNED;
- SHL by 1 sets AF and OF = CF XOR bit 15 of the result;
- XOR clears CF, OF and AF; INC keeps CF.

| Stop | Pinned by |
| --- | --- |
| RETURNED | Unicorn and native |
| LIMIT | Unicorn: the budget sweeps; the record or the block over the loop counter, which re-zeroes it so the loop never ends; C test |
| TRANSFER | Unicorn: the record or the block over the return address, in both runs; C test |
| ARGUMENT | C test: null bus or budget, short RAM, IP, SP. CPU and RAM untouched |

## Checks

**Original-byte Unicorn differential: 496 cases, 0 mismatches.** Every case's world, including its 1 MiB background, is built from the
case's own name. Compared after every case, **unmasked**: CPU14, **all 1,048,576 RAM bytes**, the EGA state and the ordered VRAM write operands,
the stop and **the remaining budget**.
- 307 RETURNED, 171 LIMIT, 18 TRANSFER.
- **instruction starts 31/31 (+0130) and 34/34 (+0185); conditional outcomes 2/2 and 2/2**
- **283 independent statements** on plain returned cases: the five record words or the ten block words and the return, from the
  stated rule without the C model.
- **FLAGS:** exact, except where the last flag writer is a word DIV or a non-zero SHL. There the declared pinned split applies (counted, never
  a mask): 169 states under the DIV rule and 36 under the SHL AF rule. The native run checks the DIV rule against DOSBox-X itself.

Cases, for each entry:
- 17 values (0..11, 7FFFh, 8000h, FFFEh, FFFFh, 12345, 0F0Fh); 8 flag images; 6 entry SPs; a zero background;
- a budget sweep over every instruction; 30 random cases;
- +0130: record offsets wrapping at 16 bits and across A20; **the record in the EGA aperture** (five placements, two straddling A000h and
  B000h); the record over each byte of the frame; the record over DS:[5D92] itself, so a later `LES` reads the rewritten pointer;
- +0185: DS = SS with the block over each byte of the frame; DS at FFFFh, F980h and 0.

**Mutation (rule-14 baseline first): the unmutated model passed the same 42-case subset (26 RETURNED, 13 LIMIT, 3 TRANSFER); then 13 of 14
built mutants were caught, with 1 survivor stated below.** Caught:
- the clear from +86h; quotient and remainder swapped; the loop running to 5; the record written flat (caught on the EGA aperture case);
- the block at 67F0h; no `XOR DX,DX` after the DIV; the DIV flags left unchanged; the DIV CF from remainder bit 0; SHL without AF;
- INC clearing CF; `RETF 4`; the return-address check dropped; the loop INC not charged to the budget.

Earlier rounds, all stated:
- the first round (on the same model) left three survivors;
- **"INC clears CF" and "the return-address check dropped" were case gaps.** The CMP before the INC borrows (counter < 4), so CF=1 shows at
  a stop right after the INC: `0130-budget-15` and `0185-budget-18` were added. A record over the return address (`0130-rec-on-frame-02`)
  was already in the subset;
- **"CBW as zero-extension" is equivalent and is not counted.** The counter byte is only ever 0..4 (the loop writes zeros only, and it
  holds the value at all times), so sign- and zero-extension always agree;
- **the survivor, "+0130 LES once (the third LES dropped)", is not observed.** It would matter only if the quotient store at `ES:[DI+90h]`
  rewrote DS:[5D92] between the second and third LES. But the loop's ten zero bytes `[P+88h, P+92h)` cover that same word first, so any
  placement that reaches the pointer has rewritten it before the quotient store. Every `rec-on-pointer` placement (14 offsets) is equivalent
  under the mutant. A bounded arithmetic search of 2,000,000 placements near the pointer, with random segment aliasing and A20 wrap, found no
  observable case. This is evidence, not a proof; the mutant is reported as a survivor, not counted as caught.

**Native (synthetic DOSBox-X normal/386; packaged DOSBox-X 2026.01.02, sha256 99d74601, the same release as pinned 5a9c7c53):** **62 cases, 20,317,896 bytes compared, 0 mismatches, FLAGS exact** (CPU14 and the five 64 KiB segments after the code). The cases: 13 values, 5 flag images and 5 SPs per entry; record offsets wrapping at 16 bits; and the frame-aliasing placements that RETURN. The 42 candidates that end in TRANSFER or a never-ending loop cannot come back to the oracle, so the native runner skips them by name before launching (the Unicorn differential covers them). The native FLAGS check is what confirms the pinned DIV rule against DOSBox-X itself: +0130 returns with the DIV's flags.

**C test** (CHECK macro, independent of NDEBUG; 197 checks, also run with `-DNDEBUG`):
- both entries over 15 and 12 values; the DIV flags at +0130's return (7: CF|PF; 5: ZF); +0185's XOR flags;
- a record over the return address (TRANSFER), a record over the counter (LIMIT);
- the budget stop after SHL with AF set; budget 0; the guards.

**Census:** no new flag rows. `inc8` hashes to the existing row `34e2e29e…` (INC r/m8, CF kept; classified for 23DB by #1540), which now lists this file too, with both callers named. The helpers' comments sit above their signatures; #1540 also taught `flag_census.py` to see a comment between `)` and `{` (reported in i23004). The models' arithmetic merge hashes to the shared row `670398ea…`, which now lists five files (2E03, 2E46, 23DB, 23B7 and this one); its caller description and pinned basis were widened to name ADD r16, XOR r16, SHL r16 by 1 and DIV r16 (pinned DIVW, `instructions.h:655-673`).

**Aperture census:** a new row `overlay_u7f_div5_leaves_cpu.c,bus,yes-with-ega-oracle,no`: +0130's record writes go through the bus, the differential observes the A000 aperture with the independent EGA, and there is no flat callee to declare.

**credit_evidence_kind.csv:** regenerated by the tool. Both rows move from `unit-test` (`poolrad_overlay_u7f_div5_tests`,
`poolrad_overlay_u7f_div5_dwords_tests`) to `checker` (this audit), because a registered original-byte checker now covers them. The unit
tests remain registered in CTest. That touches a credit table, but **the credits themselves are unchanged**.

## What this does not show

- Credit or identity: unchanged.
- The hybrid glue: par-b's next step, after merge.
- What the five words and the block hold: unassigned, as in the semantic headers.

## Reproduction

    python3 tools/overlay/ovl_007f_div5_leaves_cpu_audit.py --unicorn --mutate --native --output resources/u7f-div5-leaves-evidence.json
    python3 tools/overlay/ovl_007f_div5_leaves_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main OVL_007F_0130 OVL_007F_0185
