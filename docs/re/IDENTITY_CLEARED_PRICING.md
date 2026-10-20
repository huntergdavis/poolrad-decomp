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
| OVL_006F_1262 | 452 / 174 (queue 484) | game_evidenced | 2/6 | 006F_0482 540 (4/10); 006F_0EC4 926 (3/11) | ≥1,466 |
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

