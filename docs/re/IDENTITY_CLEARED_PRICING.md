# Identity-cleared rows: measured readiness prices (2026-10-20)

The class-(a) identity sweep ([IDENTITY_SWEEP_CLASS_A.md](IDENTITY_SWEEP_CLASS_A.md))
plus the VP's edge rulings (i19596) left a work queue of thirteen rows. This
record prices them by **readiness**, which is a separate axis from identity.
A row can be identity-cleared and still not buyable, because its callees are
not ready. `OVL_009E_0026` is the cleanest example: its identity is
established, and its readiness is blocked by an indirect dispatch.

**Decision recorded (VP i19611): decline these chains on cost.** Only
`OVL_03B0_11F3` is actionable (0 unready). Do not start `RES_0464_0111`,
`RES_0464_1E5C` or the U3B9 leaves on the strength of this table. Re-derive
the table first if the strict set has grown.

## How the figures were measured

- Queue: `strict_candidate_queue.py` export regenerated on main `8e82b54a`
  (2026-10-20 12:19–12:22 PDT). The export file (ignored) has SHA-256
  `a89e59e1be77033ff92ea91df99952a8d3aa57d49cbf019e3556fc9e67d65da2`.
- **Span**: decoded from GAME.OVR to the first return that no earlier direct
  branch jumps past. A queue extent larger than this span is flagged.
- **Unready**: every callee whose readiness is not
  `strict`/`modeled`/`resident_covered`/`runtime_covered`, using the code's
  own readiness sets, never a hand list.
- **Floors**: an overlay callee is priced at its queue row bytes. A resident
  callee is priced at its first-return span in the loader-expanded START.EXE.
  `runtime_unmapped` endpoints are left **unpriced**.
- **Direct price floor**: the sum over a row's unready callees, one hop only.
  Every unready callee listed here has unready callees of its own (the
  "own" column), so **all prices are floors and errors run upward**, as
  [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md) warns.
- Every row already has three or four implementation files on main (the
  queue's `prior_translation.main_files`), so any purchase is a full-bar
  upgrade, not a new body.

## Per-row prices

| Row | Span (B / insns) | Class | Unready | Unready callees (floor B; own unready) | Direct floor |
| --- | ---: | --- | ---: | --- | ---: |
| OVL_03B0_11F3 | 82 / 29 | helper_candidate | 0/1 | — | **0 (actionable)** |
| OVL_0025_1CC3 | 301 / 81 | helper_candidate | 3/3 | three `runtime_unmapped` endpoints (unpriced) | ≥0 + 3 unpriced |
| OVL_006F_0010 | 391 / 147 (queue 424) | game_evidenced | 7/14 | 006F_02AE 129 (2/7); RES_0802_02EB ≥107; five `runtime_unmapped` (unpriced) | ≥236 + 5 unpriced |
| OVL_0025_0511 | 51 / 21 (queue 61) | unresolved | 2/2 | 03B9_05D9 190 (1/5); 03B9_0697 113 (3/3) | ≥303 |
| OVL_009E_0026 | 282 / 94 (queue 314) | unresolved | 1/2 | 009E_0000 38, `indirect_unresolved` (1/1) | ≥38 + dispatch closure |
| OVL_0077_299C | 369 / 164 | game_evidenced | 2/8 | 0086_008A 515 (2/6); RES_03F1_02E8 ≥127 | ≥642 |
| OVL_0025_0411 | 113 / 36 | unresolved | 1/2 | 006F_0EC4 926 (3/11) | ≥926 |
| OVL_006F_1262 | 452 / 174 (queue 484) | game_evidenced | 2/6 | 006F_0482 498 (4/10; the queue's 540 includes the 42 B tail, corrected here); 006F_0EC4 926 (3/11) | ≥1,424 |
| OVL_0025_1684 | 260 / 100 | game_evidenced | 3/6 | 0025_156D 279 (1/7); 03B0_03BD 830 (3/11); RES_00BA_0767 ≥390 | ≥1,499 |
| OVL_03CE_01C8 | 820 / 353 | game_evidenced | 2/10 | RES_0464_0111 ≥404; RES_0464_1E5C ≥1,586 | ≥1,990 |
| OVL_005E_0123 | 110 / 36 | game_evidenced | 1/1 | 009E_02FA 2,029, `partial` (1/1; HOLD per i19596) | ≥2,029 |
| OVL_006F_22D5 | 267 / 107 | game_evidenced | 5/9 | 006F_1D58 1,405 (11/21); 03B0_03BD 830 (3/11); RES_0802_02EB ≥107; RES_0802_095D ≥11; RES_0899_0000 ≥51 | ≥2,404 |
| OVL_006F_0482 | 498 / 183 (queue 540) | game_evidenced | 4/10 | 03CE_01C8 820 (2/10); RES_0464_1E5C ≥1,586; RES_0802_094A ≥19; RES_0802_095D ≥11 | ≥2,436 |

`unresolved` abbreviates `game_or_other_library_unresolved`. For comparison,
the VP's working ceiling is 132 B per credited row (i19611). The cheapest
priced chain above (0511) is ≥303 B for one row and needs two hops.

## Shared blockers

Buying one of these advances more than one row. It still does not complete
any row alone, because every row listed has other unready callees.

| Blocker | Floor | Rows it gates |
| --- | ---: | --- |
| RES_0464_1E5C | ≥1,586 B (604 insns, 17 calls) | OVL_006F_0482, OVL_03CE_01C8 |
| OVL_006F_0EC4 | 926 B (3/11 own unready) | OVL_006F_1262, OVL_0025_0411 |
| OVL_03B0_03BD | 830 B (3/11 own unready) | OVL_0025_1684, OVL_006F_22D5 |
| RES_0802_02EB | ≥107 B (5 SYSTEM calls) | OVL_006F_0010, OVL_006F_22D5 |
| RES_0802_095D | ≥11 B (call-free) | OVL_006F_0482, OVL_006F_22D5 |
| RES_0464_0111 | ≥404 B (154 insns, 15 calls) | OVL_03CE_01C8 directly; OVL_0025_0511 via 03B9_05D9 |

`OVL_03CE_01C8` and `OVL_006F_0482` are also linked: 0482 calls 01C8. Pricing
0482 therefore includes all of 01C8's price.

## Ruled out: four identity-cleared rows are readiness-blocked (measured floors)

These four rows passed the identity screen but are **not actionable**, and not as a
scheduling choice: each needs unready callees that are themselves blocked, and each
also reaches the 08AE driver (closure table below), which the owner's policy closes.
The floors below add the row's own span to its **one-hop** unready callees, with the
repaired spans used throughout. They are floors; every callee listed has unready
callees of its own, so the true price is higher and unbounded in two cases.

| Row | Own span (B) | One-hop unready callees (floor B; own unready) | One-hop floor incl. own (B) | Still unpriced / beyond one hop |
| --- | ---: | --- | ---: | --- |
| OVL_006F_0482 | 498 | 03CE_01C8 820 (2/10); RES_0464_1E5C ≥1,586; RES_0802_094A ≥19; RES_0802_095D ≥11 | **≥2,934** (1,318 = 0482 + 01C8, plus ≥1,616 of residents) | 01C8's own closure (RES_0464_0111 ≥404 and its 5,653 B chain); the 08AE path |
| OVL_0077_299C | 369 | 0086_008A 515 (2/6); RES_03F1_02E8 ≥127 | **≥1,011** (884 = 299C + 008A, plus ≥127 resident) | 008A needs 03B0_0D9A (784 B measured to its first RETF; 823 B in the queue row, 39 B trailing data), so **≥1,795** with it; 0D9A's own closure is unpriced |
| OVL_006F_0010 | 391 | 006F_02AE 129 (2/7); RES_0802_02EB ≥107 (5 SYSTEM calls) | **≥627** (520 = 0010 + 02AE, plus ≥107 resident) | **five `runtime_unmapped` endpoints (unpriced)** and the closures of 02AE and 02EB |
| OVL_006F_1262 | 452 | 006F_0482 498 (itself blocked, above); 006F_0EC4 926 (3/11) | **≥1,876** | 0482 is not a leaf: counting its own ≥2,436 B of unready callees the chain is **≥4,312**; 0EC4's own 3/11 unready callees are unpriced |

Notes that keep this table from being re-measured wrongly:

- `0EC4` blocks **1262** (with 0482) and `OVL_0025_0411`; it is **not** a callee of 0482.
  0482's unready callees are exactly `01C8`, `1E5C`, `094A` and `095D`.
- Earlier working figures of 1,918 B for 1262 used the queue's 540 B for 0482
  (452 + 540 + 926). With the repaired 498 B the one-hop floor is 1,876 B.
- The sizes are first-RETF spans decoded from the original bytes (0482's 498 and 0010's
  391 are after the trailing-data repairs recorded above); 1E5C, 094A, 095D, 02EB,
  02E8 are floors.
- All four rows also appear in the 08AE closure table above (closures 8,629 B, 8,917 B,
  2,117 B and 10,113 B), reaching the driver through both gateway bodies at segment 0899.
  `0482` is hit twice, because `01C8` is also 08AE-gated. Readiness would have to be
  bought **and** the driver policy reopened before any of the four could be credited.
- The creditable pool at the cheap end is therefore `OVL_0025_1CC3` (par-d, in progress)
  and nothing else from this list.

## Extent findings made while pricing

Four queue extents exceed the first-return span:

- **Trailing data, confirmed:** `OVL_0025_0511` (10 B), `OVL_006F_0482`
  (42 B) and `OVL_006F_0010` (33 B). Each tail is wholly length-prefixed
  printable strings, and none contains an indirect transfer. They are recorded
  with hashes in
  [QUEUE_METADATA_REPAIR.md](QUEUE_METADATA_REPAIR.md#identity-cleared-rows-trailing-data-2026-10-20).
- **Unverified, not proposed:** `OVL_009E_0026` and `OVL_006F_1262` each
  carry a 32-byte tail that is not text (`00 00 20 00 00 60 02 …` and
  `FF 00 …`). These look like Pascal 256-member set constants, but no
  immediate reference to either tail occurs in its body, and the data use
  was not traced. They stay as the queue has them until someone finds the
  referencing instruction.

The static pricing script is not checked in; its output is reproducible from
the queue export and media using the method above.

## Full-closure prices and the 08AE gateway (2026-10-20, par-d)

The tables above price **one hop**. The queue's own rule is that an unready callee
that is itself a chain makes the one-hop figure a floor. This section walks the whole
closure with [`price_row_closures.py`](../../tools/overlay/price_row_closures.py) (overlay
callees from the queue export, resident callees decoded from the loader-expanded
START.EXE, SYSTEM endpoints against `RUNTIME_COVERED_ENDPOINTS`, strict-credited resident
bodies as terminals), on the same queue export as the tables above.

### Correction to the shared-blocker table

`RES_0464_0111` is priced there at "≥404 B (154 insns, 15 calls)". That is the body
alone: 404 B / 154 instructions, sole `RETF 0Eh` at +02A2. Its near call at +0286 goes to
the next body `RES_0464_02A5` (1,583 B / 604 insns), and its far callee `RES_0802_0716` is
564 B / 242 insns with 30 calls. **Closure: 31 bodies, 5,653 B, eight uncovered SYSTEM
endpoints** (`0AF8:00E9`, `04A9`, `1410`, `1491`, `14FB`, `1563`, `1593`, `16BC`) **and a path
into the 08AE driver (below).** `RES_0464_1E5C` is the opposite: its closure is three bodies,
1,933 B (`1E5C` 1,586 B, `007B` 84 B, `248E` 263 B), with no uncovered endpoint and no
driver call. It is clean, but it clears no row alone: `OVL_03CE_01C8` also needs `0111`, and
`OVL_006F_0482` needs `01C8`, `1E5C`, `0802:094A` and `0802:095D`.

### Closure per row (tool output)

| Row | Closure (B) | Bodies | Uncovered SYSTEM endpoints | 08AE entries (gateway bodies) | Dispatch |
| --- | ---: | ---: | ---: | --- | --- |
| OVL_0025_1684 | 7133 | 26 | 3 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |
| OVL_006F_0482 | 8629 | 36 | 8 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |
| OVL_0077_299C | 8917 | 30 | 2 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |
| OVL_006F_0010 | 2117 | 18 | 10 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |
| OVL_006F_22D5 | 10296 | 34 | 17 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |
| OVL_0025_1CC3 | 301 | 1 | 3 | none | none |
| OVL_006F_1262 | 10113 | 39 | 8 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |
| OVL_0025_0511 | 9281 | 40 | 8 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |
| OVL_03CE_01C8 | 8059 | 33 | 8 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |
| OVL_0025_0411 | 4309 | 29 | 8 | 08AE:00DC <- RES_0899_0000, RES_0899_0059; 08AE:017B <- RES_0899_0059 | none |

(`OVL_009E_0026`, `OVL_005E_0123` and `OVL_009E_02FA` are not in this table: their blocker is the
unresolved indirect dispatch `009E_0160 -> 009E_0000`, not size.)

### The 08AE gateway: what the owner's policy decision costs, in rows

In all nine rows that reach the driver segment, **the only path to `RES_08AE` goes through two
resident bodies at segment 0899**: `RES_0899_0000` (51 B / 19 insns), which reaches
`08AE:00DC`, and `RES_0899_0059` (171 B / 61 insns), which reaches `08AE:00DC` and
`08AE:017B`. No other body in any of the nine closures calls the driver segment. Every row
hinges on **both** entries, because every closure contains both gateway bodies; none hinges on
`017B` alone. The nine rows are `1684`, `0482`, `299C`, `0010`, `22D5`, `1262`, `0511`, `01C8` and
`0411`. `OVL_0025_1CC3` is the only identity-cleared blocked row with no driver path.

The [08AE identity policy](RES_08AE_IDENTITY_POLICY.md) records why the driver internals give no
game-credit route and that reopening needs an owner-level decision. This measurement gives that
decision a price: it is the precondition for **nine rows**. It is not sufficient for any of them.
After the two gateways (222 B together), each closure still carries at least 1,895 B (the
smallest is `0010`: 2,117 B minus 222 B) and at least two uncovered SYSTEM endpoints (the smallest
count is `299C` with two; `22D5` has seventeen). Secondary exposures that the walk lists
but this section does not price: far targets `0A32:000B` (all nine) and `0A32:00A2` (`1684`, `299C`,
`22D5`) and `0428:002A` (`299C`).

Method limit: a resident body is priced by its first-return span, which can over-include a
neighbouring entry stub (measured below: 80 B by span, 12 B by control flow), so closure bytes are
not exact floors. The gateway finding does not depend on sizes; it depends on the two decoded far
calls and the absence of any other call to the driver segment in the closures.

### `OVL_0025_1CC3`: the one row with a different blocker

`1CC3` (301 B) already has a model on main. Its three unready callees are SYSTEM 6-byte Real
entry points, `0AF8:1186`, `0AF8:119A` and `0AF8:119E`, with no driver path. By control flow
(following jumps and near calls, treating the jump to the `0AF8:00E2` RunError entry as the stop
boundary) their closure is **448 B in six bodies**: core `0F33` 263 B, `1118` 92 B, `10D9` 63 B,
and wrappers `1186` 12 B, `119A` 4 B, `119E` 14 B. A scan of those bodies finds no `IN`, `OUT`,
`INT`, `HLT`, `CLI` or `STI` and no control-register operand. They are pure register arithmetic
with the stack and the RunError stop. The three entries gate exactly one row outright (`1CC3`; three
more rows list them among other blockers). That is about 448 B per cleared row, against 2,117 B for the next cheapest closure, and VP ruling i19641 started this chain.

## Bought blocker: the `0899` gateways (VP i20006, 2026-10-20)

**Decision.** `RES_0899_0059` is funded as its own task, priced as its own row by par-b; `RES_0899_0000` is bought with it. It is the first time this program has chosen to buy a **blocker** instead of a body. The reasoning is recorded here so it can be read later.

**What is bought: 222 B of game code.** The spans are decoded from the loader-expanded START.EXE, and each ends in `RETF`.

| Body | Size | What it is | Calls |
| --- | --- | --- | --- |
| `RES_0899_0059` | 171 B / 61 insns | ReadKey with hotkeys | reads a key via `0A40:030F`; key `13h` swaps the sound state `DS:5372`/`5373` through `08AE:00DC` and `08AE:017B`; key `04` toggles `DS:49F7` (gated on `0AF8:16BC`); key `03` does `push cs; call 0000`; then drains KeyPressed/ReadKey (`0A40:02FD`/`030F`) |
| `RES_0899_0000` | 51 B / 19 insns | QUIT | restores `5372`, stops the driver via `08AE:00DC` with `[2612]`, builds `AH=0`, `AL=DS:[4A1C]` (saved startup video mode) in the Registers record at `DS:4A02`, calls `Intr($10)` via Dos `0A32:000B`, then Halt `0AF8:00E9` |

- **Identity** is strong on factor 2, through the game globals `5372`/`5373`/`49F7`/`4A1C`/`2612`.
- **Factor 1:** fixup-backed callers inside game_verified functions (`03BD_0E08` and `03BD_10C7` for both, and `001C_19B0` for `0000`). The par-a verdict is i19944, accepted in i19946.

**Reach: NECESSARY, then SUFFICIENT, in that order.** par-a regenerated the strict-candidate export on main `b3fd5718` and priced every candidate's full closure with `price_row_closures.py`.

- **Necessary:** of **447** candidate rows, **303** have a gateway in their closure. 302 reach `0059`, and 303 reach `0000`; the extra row is `OVL_03B9_0708`, which reaches only `0000`. par-b reported **302 of 453**. The 302 agrees with the `0059` count; the denominator of 453 was not reproduced on this export (447) and is recorded as par-b's figure.
- **Sufficient for almost none:** in exactly **one** of the 303 rows (`OVL_007F_0294`, itself a caller of `0059`) are the only non-driver bodies the row and the gateways. Even that row still needs the driver members and its endpoints. **The 303 is not a promise of 303 credits.**

**Non-body costs of the wall:**
1. runtime coverage for Dos `Intr` at `0A32:000B` (in every gated closure);
2. Halt `0AF8:00E9` declared as a boundary terminal;
3. the `08AE` `driver_internal` members (`00DC`, `017B` and their closure) that par-c and par-d are already building (see [DRIVER_INTERNAL_CLASS.md](DRIVER_INTERNAL_CLASS.md)).

So the wall costs 222 B, one coverage model and one boundary declaration, on top of driver work already in flight.

**Cheapest rows it enables** (closure bytes and bodies, from the same run). Each row still carries `08AE:00DC`/`017B` and `0A32:000B`.

| Row | Closure | Uncovered SYSTEM endpoints |
| --- | --- | ---: |
| `OVL_006F_02AE` | 425 B / 4 | 2 |
| `OVL_03D2_0004` | 539 B / 4 | 1 |
| `OVL_007F_0294` | 662 B / 3 | 2 |
| `OVL_006F_01B8` | 671 B / 5 | 6 |
| `OVL_001C_0994` | 726 B / 4 | 1 |
| `OVL_006F_0010` | 956 B / 6 | 9 |

**Halt consequence: read this before reviewing any row that reaches `0000`.** Every path through `0899:0000` is **terminal**. Halt runs the ExitProc chain, restores the interrupt vectors and terminates through DOS. The `pop bp; retf` after the call is dead code, and the `push cs; call 0000` on key `03` in `0059` never returns either.

- A row whose closure reaches `0000` therefore carries a stop that is a **process exit**. It must be named as such and split from returning outcomes, as `CALLEE_NONRETURNING` in the `U3B9_ENTRY7_0708_SPRITE.md` contract.
- **Any model that claims to return from `0000` (or from Halt) is wrong**, and a differential that shows it returning is comparing against a stub.

## `RES_0464_1B52`: priced and declined (2026-10-20)

The requirement for a bus-aware SYSTEM Move was raised by the par-b lane (i19660). This section records the measurement
behind the decline, made read-only from the media and the checked-in sources on the same queue export as above, so
that nobody rebuilds it later believing it is cheap. VP ruling i19674: declined.

**The body.** `RES_0464_1B52` is a resident body with no queue row: **778 B / 305 instructions**, one `RETF`, ledger
class `unresolved`, with 13 overlay far call sites and 5 loader sites, and no model, strict credit or identity. It is the
saved-slot-A page flush, an `A200 -> A000` mode-1 transfer
([U19_A200_MODE1_LATCH_BOUNDARY.md](U19_A200_MODE1_LATCH_BOUNDARY.md),
[SLOT_A_RES464_SECOND_FLUSH.md](SLOT_A_RES464_SECOND_FLUSH.md)). It makes six calls, **all in
`RUNTIME_COVERED_ENDPOINTS`**: `0AF8:1685` Move backward three times and `0AF8:16A8` FillChar three times. It also
executes nine `OUT DX,AL` and eight `POP ES`; there is no `IN`, `INT` or `HLT`, no indirect transfer and no other callee.

**Why "covered" does not mean ready here.** The covered endpoint `0AF8:1685` maps to `pr_system_move_1685`, which takes
flat RAM. `include/poolrad/recovered/memory_bus.h` defines `pr_bus`: byte accesses to physical `A0000..AFFFF` go through
`pr_ega` (a read loads the four latches; a write goes through the write mode, and in mode 1 it writes the latches), the rest
is flat RAM, and `OUT` goes to the port environment and then the EGA. A Move over `A000`/`A200` run on flat RAM would copy
RAM bytes instead of loading and writing latches. A bus-aware FillChar exists (`system_fillchar_bus_16a8_cpu`), but the
covered set maps `16A8` to the flat leaf; **no bus-aware Move exists anywhere in the tree.**

**The three-part cost.**

1. the body: a port-bound `pr_bus` raw-CPU model of 778 B / 305 instructions, like the other port-bound residents;
2. a new endpoint model `pr_system_move_bus_1685` (35 B / 16 instructions: `MOV BX,SP`, `MOV DX,DS`, `LDS SI`, `LES DI`,
   `MOV CX`, `CLD`, `CMP SI,DI`, `JAE`, `ADD SI,CX`, `ADD DI,CX`, `DEC SI`, `DEC DI`, `STD`, `REP MOVSB`, `MOV DS,DX`,
   `RETF 0Ah`) with each byte through `pr_bus_read8`/`pr_bus_write8`;
3. a way for the covered set to say "covered with a bus": today one endpoint maps to one model, so this is a structural
   change to the readiness mechanism, not a model.

**The value.** Eleven queue rows list `1B52` as an unready callee, but **only one has it as its sole unready callee:
`OVL_03BD_05F3`** (10 B, classified `game_or_other_library_unresolved`, so its own identity is unresolved). That is about
**813 B (778 + 35) for one small row**, before the covered-with-bus mechanism, which has no byte price.

**Declare instead of model?** No mechanism that exists allows it. The latch semantics already live as a model (`pr_ega`),
and the declaration mechanisms (the manifest `callback` status, `declared_boundary`) cannot launder an ordinary SYSTEM
helper, as the [08AE identity policy](RES_08AE_IDENTITY_POLICY.md) records. If the bus-aware Move is ever wanted, it is
built as a model, by ruling.

## Parked work with no reachable consumer (recorded so it is not rebuilt)

**`RES_0464_248E`** - finished and parked, never pushed as a PR. Branch **`claude/par-b-0464-248e` @ `4f605bf2`** on origin. Measurements: Unicorn 41 cases over CPU14 + 1,048,576 B, body 95/95 starts and 6/6 outcomes, closure 206/209 starts and 27/30 outcomes (the three `GetMem` HeapError AL>=1 continuations excluded on the same argument as credited `U33`), **eight declared AF-only splits** where the model follows DOSBox-X, confirmed by native 46/46 including seven cases with AF=1, and 22 ASan checks. It has **no evidence doc**, and it **depends on `emit_raw_cpu_c.py` from `#1213`**.

*Independently verified by par-c (2026-10-21) on `4f605bf2`, with the media and the wrapper; every figure above reproduced exactly:* 41 Unicorn cases; body 95/95 starts and 6/6 outcomes; closure 206/209 starts and 27/30 outcomes, the missing three being exactly `03C9->03CB`, `03CB->0383` and `03CB->03CD` (starts `03CB`, `03CD`, `03D0`); eight AF-only splits (model `0016`, Unicorn `0006`, all in mode 1); native 46 cases with `af_set_at_return` 7, no flag masks; 22 ASan/UBSan checks; and `emit_res0464_alloc_248e.py --check` with main's `emit_raw_cpu_c.py` regenerates the committed C and header byte for byte (`EMIT_CHECK OK`). The span was re-derived from the original bytes: 263 B / 95 insns, one RETF at `+2592` (`RETF 0Ch`), SHA-256 `3476a8ae8b3a3c1430c591b9922d493db2e4c475d542f089257464a82ce9d32f`. **Precision to carry with the figures:** the audit's direct RAM comparison is 983,040 bytes (`A0000..AFFFF` is covered by a write-operand replay and an "untouched in C" check, with no aperture reads), not a plain 1,048,576-byte compare; native compares CPU14, a 1,024-byte stack window and two 64 KiB segments (132,096 bytes, so 916,480 backing bytes are not compared natively), result SHA-256 `fdbce0903d01619d0eb8082a51f85a2f989befdb7ea045c3eca69dc2c931b838`; the eight Unicorn splits and the seven native AF=1 cases are different case sets, not a one-to-one match; the model header cites `docs/re/RES_0464_ALLOC_248E_CPU.md`, which does not exist; the branch was cut from an older main and is not gated or rebased.
Why parked: its only consumers were the `0511` chain and `0111 -> 05D9 -> 01C8` (08AE-gated, closed by policy). The `0511` chain is dead for **two independent reasons**: `1B52` is declined, so `U3B9 +0000` cannot complete, and `1951` cannot be declared ready (next section). Re-deriving `248E` does not revive that chain. If either route is ever reopened, start from that branch rather than re-deriving it.

## `RES_0464_1951`: translated and evidenced, but undeclarable (2026-10-20)

`RES_0464_1951` (the bitmap blit, 513 B / 201 instructions) is **translated and evidenced**: the raw-CPU body and its
full evidence merged as #1213 ([RES_0464_BLIT_1951_CPU.md](RES_0464_BLIT_1951_CPU.md)), with the DOSBox-X binary
substitution declared. It is still counted as **untranslated wherever it is a callee**, because it **cannot be declared
ready until it has identity**. It is `unresolved` in both resident ledgers.

Measured on main `057d901f` by the review lane (VP ruling i19703). Each available declaration is refused by the tooling:

- **Strict row:** `RES_0464_1951` in `strict_full_c_addresses.csv` makes `function_queue.py` refuse with "strict
  full-C credit lacks verified game identity".
- **Manifest row:** `RES_0464_1951,full` in `partial_callee_models.csv` leaves the checkpointed function queue unchanged
  (`0936e38e`, 250 / 296), because that queue does not read the manifest. But `strict_candidate_queue.py` refuses it
  with "callee model manifest address is neither an overlay row nor a strict-credited resident id"
  (`strict_candidate_queue.py`, the resident branch of the manifest check).

Resident readiness is **credit-gated**, the same structural gate met at `2595` from the other side. No readiness-only
declaration exists for an unresolved resident.

**Not taken:** loosening the resident manifest gate so that readiness could be declared without credit. That would let
any unresolved resident compose, which is the laundering the gate prevents. Doing it would need a deliberate ruling with
the `08AE` and `2A0D` consistency cost named, not a tooling convenience.

**Consequence:** every caller that lists `1951` keeps it as an unready callee. That includes the `0511` chain above,
and `OVL_040C_015D` under the held `RES_03F1_0000` ([STRICT_CANDIDATE_QUEUE.md](STRICT_CANDIDATE_QUEUE.md)).
