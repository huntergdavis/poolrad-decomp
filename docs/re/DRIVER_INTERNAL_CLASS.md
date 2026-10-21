# Driver-internal bodies: a declared readiness class that is never a credit

**Status: nine members, one deferred.** Members on main after this change, in landing order:
- `181A` (#1247);
- `0000` (#1255);
- `1742` and `17AE` (#1287);
- `137C` and `13B1` (#1292);
- `006C` and `00DC` (#1296);
- `017B` (this change).

`01A5` is **deferred**, and the reason is recorded below. Design was approved in principle by the VP (i19925, after design
i19920), and the reviewer roles were ruled in i19927.

**Members buy readiness, never credit.** Each acceptance measured the strict-candidate export with and without the
row:
- Every member left the totals unchanged (447/58 at the first; 440/51 at the last, after unrelated credits).
- The only member that moved any row is `00DC`. Eleven rows that call `08AE:00DC` directly each lost exactly one unready
  callee, and none became composable.
- Strict and identity counts are untouched by every member.

A member counts toward a row only when every other unready callee on that row's path is also ready.

## What it is

**`driver_internal`** is a fully translated, fully evidenced body of the documented **08AE timer/audio driver**, whose authorship is the driver's, not the game's ([RES_08AE_IDENTITY_POLICY.md](RES_08AE_IDENTITY_POLICY.md)).

- A member **composes like a ready dependency**.
- A member is **NEVER a credit**: it is never counted in strict or identity totals.

It is a sibling of `declared_boundary`, not a reuse of it:

| Class | What it is | Behaviour |
| --- | --- | --- |
| `declared_boundary` | a thin environment contract | partly supplied from outside (BIOS) |
| `driver_internal` | a body whose **composed** run reproduces the original (definition below) | modelled, and composed end to end |

The 08AE policy states that driver helpers "do not become environment endpoints eligible for declared_boundary merely by belonging to a driver".

## Membership: narrow, enforced in code (`strict_candidate_queue.load_driver_internal`)

- **Where members live:** only ids listed in [`driver_internal_models.csv`](driver_internal_models.csv). The columns are `id, evidence_doc, review_pr, reviewed_commit, body_sha256, body_bytes`, with exactly that header.
- **Every member moves a pinned set under review.** `test_committed_table_loads_with_exactly_the_reviewed_members` (`test_strict_candidate_queue.py`) asserts the exact member set, currently the nine members above. So a table row added without the matching test change fails CI.
  - Before #1247 that test asserted the table was empty. That was a vacuous truth written by par-a in #1241: it would have kept passing for as long as nobody added a member, and it proved nothing about the members.
  - par-e replaced it with the pinned set.
- **Segment-scoped:** an id must match `RES_08AE_XXXX`. Widening the class to another segment is a policy change to this rule, never a table row.
- **Disjoint from:**
  - `DECLARED_BOUNDARY_ENDPOINTS`;
  - `resident_inventory.GAME_SPECIFIC_TARGETS`;
  - `strict_full_c_addresses.csv`;
  - `reviewed_function_identities.csv`;
  - `partial_callee_models.csv`.

  The loader refuses a credit or manifest row that names a member **before any other check**, so an attempt to credit a member fails on the class itself.
- **Provenance:** a 64-hex body hash, a positive body size, a 40-hex reviewed commit and a positive review PR. The evidence document must exist under `docs/re/` and name both the member and its body hash.
- **A missing declaration** leaves the target `resident_unmapped`, which is unready. This is fail-closed, and the same as before the class existed.

## Evidence bar for a member (the same as a credited resident body)

- The span is hashed from the loader-expanded image, and the return is located before sizing.
- An unmasked CPU14 + 1 MiB differential covers 100% of starts and every feasible outcome. Exclusions are argued and asserted, and stops are split and named.
- Native corroboration, with any stub declared.
- A `CHECKER_ROWS` registration whose checker runs on current main.
- **Closure-complete, enforced by the loader, not by review memory:**
  - With the loader-expanded START image, `load_driver_internal` decodes each member and refuses it unless every transfer
    out of it is ready. A far transfer must reach a runtime-covered endpoint, a declared boundary, a member or a strict
    resident. A near transfer must reach a member or a strict resident (#1255).
  - Directness comes from the decoded operand types, never from printed text (#1283).
  - Indirect transfers are refused.
  - An interrupt is refused unless it is `INT 21h` with AH set by a literal `MOV` in the same basic block to a
    `DECLARED_DOS_SERVICES` entry (`25h`, `35h`). The member's evidence document must then carry the exact marker,
    for example `INT 21h AH=25h` (#1288, VP i20129).
  - `strict_candidate_queue.py` refuses to run when the table has members but no hash-checked START.EXE.
  - The 08AE set exits to `0A32:00A2` (Dos.GetTime) and `0A32:000B` (Dos.Intr), both runtime-covered, to SYSTEM LongMul
    `0AF8:0A5B`, and through `0899:0000` to the terminal `Halt` boundary `RES_0AF8_00E9`.
- **Roles (i19927):** an author lane writes each member, par-e accepts it, and par-a accepts the class and the end-to-end row.

## Definition: a member is a composed body (VP i20169, i20176)

**A member is a body for which a COMPOSED differential shows that its segment, its REAL callee members and the resume
points together reproduce the original bytes' behaviour end to end.** Concretely:
- the models are chained through their real interfaces (each CALL stop, each resume, each environment callback);
- the result is compared against the original image executing the real bodies;
- every chained callee is a member carrying its own evidence;
- the run's per-member executed-start counts show that each one actually ran.

A stop/resume or hook model qualifies only when such a composed checker drives the original bytes at each stop. The
earlier wording, "a complete model held to the credited-body bar", read as end to end and was not.

**Two kinds of fidelity are kept apart (i20175).** A member's own behaviour rests on its own evidence at the bar. The
composition rests on the composed run, which does not replace any member's evidence. Composed runs cited so far:
- `RES_0899_0000` (#1291): 0000, 006C, 00DC, 137C, 13B1, 1742, 17AE and 181A, each fully executed;
- `RES_0899_0059` (#1295): additionally 017B at 20/21. The missing start is a branch infeasible from that caller and
  covered by 017B's own 21/21.

**How this was found:** by the review lane (par-e) checking what a member actually claims, not by a failure in the field.
The loader's closure check asks whether each callee is ready, not whether the composition reproduces the original. A
member that stops at a scripted hook and resumes from a scripted stub would have read as composable although nothing
composable had been run. This was the second laundering path found in the class, one level below the first: the first
was a member admitted with an unready callee, which led to the loader closure check in #1255.

## Deferred member: `RES_08AE_01A5` (VP i20201)

**Its evidence is complete, its composition is unprovable today, and the wall does not need it.**
- Nothing in either composed run executes 01A5. 00DC's stops are 137C, 17AE, 1742, 181A, 006C and 13B1. 017B's stops are
  137C, 1742, 17AE and 13B1. No member calls 01A5.
- Its only callers are the resident function at image `2B8B`, which is not modelled, so no composed run can show 01A5
  running until 2B8B is driven or modelled.
- The wall is `0899:0059` → 00DC/017B → their six callee members. 01A5 is not on that path, so the deferral blocks no
  credit.

**Path to completion:** a composed run from 2B8B, the resident caller that reaches both 00DC and 01A5. This is a recorded
decision, not an omission.

## Reciprocity: resident `2B8B` calls the spine

The resident function at image `2B8B` calls both `08AE:00DC` and `08AE:01A5` (par-d's census). So **any resident row
that ever composes 2B8B needs both 00DC and 01A5 to be members first**. 00DC is a member; 01A5 is deferred, as above.
Whoever composes 2B8B owns both the 2B8B composed run and 01A5's membership.

## How a consuming row declares it

- A credited row whose closure reaches a member lists each member in its own evidence document under the heading **`Composes driver internals:`**, giving the ids and the entry it reaches them through.
- The queue export shows `readiness=driver_internal` on those targets.
- The acceptance of the consuming row checks the list against the closure.

## The proving row: `OVL_006F_0010`

Closure measured on main `0f8dba7c` with `price_row_closures.py` after #1240, which treats declared boundaries as terminals. That gives **956 B / 6 bodies** outside the driver. Every other 08AE-gated row needs 2.9–8.8 KB of non-driver work.

| # | Body | Bytes | Kind | Needs | Lane |
| --- | --- | ---: | --- | --- | --- |
| 1 | `RES_08AE_00DC` | 159 | driver_internal | full bar (a bounded dispatch model exists, `resident_8ae_dispatch.c`) | author TBD, accepted by par-e |
| 2 | `RES_08AE_017B` | 42 | driver_internal | full bar | |
| 3 | `RES_08AE_0000` | 108 | driver_internal | full bar | |
| 4 | `RES_08AE_006C` | 112 | driver_internal | full bar | |
| 5 | `RES_08AE_137C` | 53 | driver_internal | full bar | |
| 6 | `RES_08AE_13B1` | 57 | driver_internal | full bar | |
| 7 | `RES_08AE_1742` | 108 | driver_internal | full bar (a table model exists) | |
| 8 | `RES_08AE_17AE` | 108 | driver_internal | full bar (a table model exists) | |
| 9 | `RES_08AE_181A` | 22 | driver_internal | **member** (#1243 / #1247; zero readiness delta on its own) | author lane, accepted by par-e |
| 10 | `RES_0899_0059` | 171 | **game** (gateway; two-factor identity passed, VP i19946) | strict | |
| 11 | `RES_0899_0000` | 51 | **game** (gateway; two-factor identity passed, VP i19946) | strict | |
| 12 | `RES_0709_0BF5` | 74 | game resident | identity + strict | |
| 13 | `RES_0802_02EB` | 107 | game resident | identity + strict | |
| 14 | `OVL_006F_02AE` | 129 | overlay | full bar (it becomes `modeled` once its callees are ready) | |
| 15 | `OVL_006F_0010` | 424 | overlay, **the proving row** | full bar; identity already passes the two-factor test (it reads `DS:5D96`, the party head) | par-a accepts |
| L | SYSTEM `00E9`, `0567`, `05C1`, `081C`, `08E1`, `0A04`, `1410`, `1491`, `17D6`; DOS unit `0A32:000B` (Intr, called by `RES_0899_0000`) and `0A32:00A2` (called from the 08AE set) | unpriced | 11 library endpoints | runtime-coverage models | |

The driver set is **769 B over 9 bodies**. Non-driver work is **956 B over 6 bodies**. There are also **11 library endpoints**. The total is **at least 1,725 B plus the 11 endpoints**.

**Correction (2026-10-20):** the first version of this table listed 10 endpoints and omitted `0A32:000B`. The pricer collected far calls outside 0AF8, the resident segments and 08AE into a set it never printed. It now prints them in an "Other far targets" column with the calling bodies. `0A32:00A2` does not appear in that column for this row, because the pricer records 08AE entries without walking the driver; it comes from the driver set's own exits (see the evidence bar above).

**The class is necessary but not sufficient.**
- Had the `0899` gateways failed the two-factor identity test, the proof would have failed **at the gateway, not at the driver**. Both passed (par-a verdict i19944, accepted by the VP in i19946):
  - `0059` is the game's key-read wrapper; its Ctrl-S branch saves and sets the sound-mode byte `DS:5372`, which the game writes and the driver only reads as a gate.
  - `0000` is the game's quit routine: it restores `DS:5372`, restores the startup video mode `DS:4A1C` through the Registers record at `DS:4A02`, and calls Halt.
  - Both have fixup-backed callers inside game_verified functions.
  - The class stays `08AE`-only.
- If the proof completes, `0010` becomes creditable with **no strict or identity row for any 08AE body**. `02AE`, `0709_0BF5`, `0802_02EB` and the two gateways also become eligible on their own identity.
