# OVL_03BD +0155: raw-CPU model of unit 0x3BD public entry 1 (four argument bytes to DS:6A60..6A63; no new credit)

`OVL_03BD_0155` (unit 0x3BD, offset 0155h, public entry 1, run-time stub `03D5:0025`) is **already strict**. Its credit
rests on the semantic translation `pr_rec_u3bd_entry1` (`src/recovered/ecl_helpers.c`; FUNCTION_LEDGER.md function 16).
That translation takes a view-state struct and four bytes, not a CPU.

This change adds the **raw-CPU, full-state model** the hybrid executor needs to run the body in place in the live game:
1 MiB of RAM plus the 14 CPU words, compared unmasked against the original bytes. It proposes **no credit and no ledger
change**. Assigned by par-a in i21416, as batch 6 of the hybrid glue series.

`deja` recall could not be run: the MCP server fails to start (ENOENT). The prior work used is cited below.

## Media and executable extent

- **Media:** START.EXE SHA-256 `d825df2b…a07bd`; GAME.OVR SHA-256 `872964a3…8614`. Both are checked by the audit before
  anything runs.
- **Span:** unit 0x3BD `[0155,0174)`, **31 bytes / 12 instructions**, at GAME.OVR file offset 191,052 (`2EA4Ch`). That is
  the offset FUNCTION_LEDGER.md gives for function 16.
- **Body SHA-256:** `f5fb6f0833aa34a68327cc0cb2ae1576f743a3f0fdb27fe7b8a101f4de8ca402`.
- **Checked by the audit's source step:**
  - unit public entry 1 is `+0155`;
  - **no jump, call, interrupt or port instruction** (a straight-line leaf, so no conditional outcomes to cover);
  - the four stores go to `DS:6A60`, `6A61`, `6A62`, `6A63` in that order;
  - the only return is `RETF 8` at `0171`;
  - **there is no relocation in the span**.

## What the body does

Frame after the far call: `[SP]` IP, `[SP+2]` CS, then four argument word slots. `[SP+4]` holds the **last** pushed and
`[SP+0Ah]` the **first**. Each slot is read as its **low byte**; the high byte is ignored.

```
0155 PUSH BP
0156 MOV BP,SP
0158 MOV AL,[BP+0C]   015B MOV [6A60],AL      the first pushed byte
015E MOV AL,[BP+0A]   0161 MOV [6A61],AL
0164 MOV AL,[BP+8]    0167 MOV [6A62],AL
016A MOV AL,[BP+6]    016D MOV [6A63],AL      the last pushed byte (AL keeps it)
0170 POP BP
0171 RETF 8
```

**No instruction writes the flags.** AH, every other register and the flags leave as they came, except BP (restored),
SP (+12) and CS:IP.

Each load rereads the frame **after** the previous store. A store can therefore change a byte that a later load reads,
when DS:6A60..6A63 overlaps the frame. The model rereads RAM in source order, and the audit places the globals on every
frame byte.

## Role (identity is not claimed here)

The four bytes go to four consecutive DS globals that the unit's view code reads. FUNCTION_LEDGER.md records two callers:

- **The program body**, at loaded image `012Ch..0138h`, pushes `0Bh, 00h, 07h, 06h` just before the gameplay loop.
- **Unit 0x3B4 `+0207`**, inside the view refresh, pushes recomputed bytes.

What the four bytes mean is not asserted, in the ledger or here.

## Membership chain

Facts only; nothing here admits a member.

- `OVL_03BD_0155` is a row of `strict_full_c_addresses.csv`, with FUNCTION_LEDGER.md as its evidence doc.
- This model adds a raw-CPU entry for the same body. It does not replace the semantic translation and changes no count.
- **On the hybrid save walk the body arrives 4 times** (stage (b) observer, walk c). It is one of the five semantic-only
  bodies the walk reaches.

## Raw contract and stops

`pr_u3bd_entry1_0155_cpu(ram, size, cpu, budget)`:

- exactly 1 MiB of A20-off RAM;
- IP `0155h`;
- **entry SP in [0002h, FFF4h]**, so the pushed BP and every word read (the return frame) stay inside the segment
  without straddling its end;
- each instruction consumes one unit of `*budget`.

| Stop | Class | Origin | Pinned by |
| --- | --- | --- | --- |
| RETURNED | RETURN | `RETF 8` to the entry frame's return address | Unicorn: 517 of 536 cases; native: 31 of 31 |
| LIMIT | LIMIT (host cap) | the budget ends at the next unexecuted instruction | Unicorn: 12 cases (budgets 0 to 11 stop, 12 and 13 complete). The C and the original stop at the same instruction with equal state and equal remaining budget. C test: budget 0 changes nothing; budget 3 stops at `015B` with AL loaded and no store made |
| TRANSFER | control transfer | **a DS store replaced the return address** (DS = SS and `6A60..6A63` on the frame's IP/CS words); the CPU is left at the popped CS:IP | Unicorn: 7 cases, all in the `globals-at-bp…` family; C test: `ip 2211, cs 4433` |
| ARGUMENT | ARGUMENT | null RAM/CPU/budget, size, IP, or entry SP outside [0002,FFF4] | C test (ASan/UBSan): CPU, RAM and budget untouched |

The final run has **0 ARGUMENT stops**. TRANSFER occurs only in the named family; it is counted per case, not asserted.

## Pointer census

| Pointer | Source | The body's own access | Bus? |
| --- | --- | --- | --- |
| the DS globals `DS:6A60..6A63` | fixed offsets in the caller's DS | four byte stores | no: flat DS, as for every DS global |
| the entry frame (arguments, return address) and its own saved BP | the caller and the body | flat SS byte reads, `PUSH`/`POP`, `RETF` | no |

**The body dereferences no caller-supplied pointer**, so nothing goes through `pr_bus`. If a caller set DS so that
`6A60..6A63` landed in the EGA aperture, the original's stores would reach the planes and this model's would not. No
caller does that; the stated DS is the program's data segment.

## Checks

The audit is `tools/overlay/ovl_03bd_0155_cpu_audit.py` (`CHECKER_ROWS = ('OVL_03BD_0155',)`). The original bytes run in
Unicorn and the C runs on the same state. The comparison covers **CPU14, all 1,048,576 RAM bytes, the stop and the
remaining budget, with no flag mask**.

**Unicorn: 536 cases, every one equal.** All 12 instruction starts were reached, and there are no branches.

- **Values:** every byte value 00..FF through every slot, plus the program body's own call (`0B 00 07 06`) and high
  bytes 00, 80 and FF.
- **DS:** 0000, 1000, 9000, FFFF and F96A. The last two put `6A60..` above 1 MiB, where A20 is off and the address wraps.
- **Entry SP:** 0002, 0003, 0101, 7FFF, F001, FFF3 and FFF4.
- **Incoming flags:** 11 values, including all arithmetic flags set.
- **Code segments:** three.
- **DS = SS with `DS:6A60` on each byte from BP−10h to BP+13h.** These put the stores on the saved BP, the return address,
  the four slots (so a later load rereads a stored byte) and the caller's words.
- **A budget sweep** from 0 to 13, and 200 random cases.

**Independent statements.** 503 cases are also checked without the C model, from the arguments alone: the four DS bytes,
AL, and SP/BP/CS:IP/flags on return. The rest are the aliased, limited and transfer cases.

**Mutants: 9, of which 8 are caught by the differential.**

| Mutant | Result |
| --- | --- |
| first load at `[BP+0A]` | caught |
| stores shifted by 4 | caught |
| last store writes AH | caught |
| loads clear AH | caught |
| `POP BP` skipped | caught |
| `RETF` pops 6 | caught |
| TRANSFER check dropped | caught |
| a store not charged to the budget | caught |
| **"loads taken from the entry SP, not BP"** | **survives, and is equivalent**: after `MOV BP,SP` the body pushes nothing more, so SP equals BP at every load |

**C test:** `tests/recovered/test_overlay_u3bd_entry1_0155_cpu.c`, run under ASan/UBSan. It covers the program body's call,
argument-slot aliasing (one case shows the reread), the TRANSFER case, the budget, and atomic argument rejection.

**Native.** `tools/overlay/ovl_03bd_0155_native.py` runs the original bytes under DOSBox-X normal/386 (the host's packaged
build), with no masks: 14 CPU words and five 64 KiB regions per case. Results are recorded below.

## What this does not show

- **No credit and no identity claim.** The model is a raw-CPU companion to an existing strict row.
- **TRANSFER is Unicorn-only.** Return-address aliasing cannot return to the native oracle program.
- **No EGA-aperture DS is modelled.** See the pointer census.
- **Not yet glued into the hybrid.** That is the next step, with the same shadow bar as the glue batches.

## Evidence kind (2026-10-22)

Registering `CHECKER_ROWS = ('OVL_03BD_0155',)` moves this row in `docs/re/credit_evidence_kind.csv` from `cites-tool` to `checker`, regenerated with `credit_evidence_kind.py --write`. **The credit count does not move**; only the recorded kind of evidence gets stronger.
