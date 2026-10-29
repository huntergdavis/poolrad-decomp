# Review lane handoff (Poolrad strict-C)

You verify other lanes' PRs. You do not own credit, you do not merge your own
body, and you report every outcome to the VP pane. Current checkpoint on main:
**235 strict originals / 281 verified identities**, 214 overlay credits. The
whole-game denominator is unknown — never state a completion percentage.

## Seven standing conditions

1. **Verify before merge.** Never merge an unverified PR, with one named exception:
   merge-then-verify for a blob-identical restack of an accepted, media-gated head
   (see [Verdict states](#verdict-states-accept-withdrawn-suspended-vp-i25889-cos-i24714)).
   Breaking the claim is the standard: delete a source, inject a bogus id, re-run.
2. **Doc truth.** A row is not creditable while its own evidence doc misstates
   its masking or extent. Check the doc sentence against the checker or the
   media bytes *before* crediting, not after.
3. **Runnable checker.** The row's checker must build and run on current main
   before the PR is offered. A touched row whose checker cannot run is a
   finding to record, not a silent skip.
4. **Take the PR number from `gh pr list` output**, never from memory. Two
   off-by-one credits happened by typing it.
5. **New PR after a merge.** Never force-push a merged acceptance branch, and
   regenerate a conflicting accounting pin on current main instead of hand-
   resolving it.
6. **Credit is mechanical.** The classifier returns `game_verified` iff the id
   is in `BASE_VERIFIED` ∪ reviewed identities, and only `game_verified`
   credits. The partial-callee manifest is callee *readiness*, never a credit
   gate. A strict-credit upgrade applies only when the identity row already
   exists.
8. **Every limit the evidence relies on must be stated in the document that
   carries the claim**, and a reviewer-added limit is labelled as such. An
   unstated pre-condition is silently load-bearing: the U33 caller's document
   stated its stack-pointer range but not heap well-formedness, so the reviewer
   added that limit to the dependency contract and attributed it. Credit is
   never granted on an assumption the document does not carry.
9. **Record resident credits in the right place.** Resident identities live in
   `GAME_SPECIFIC_TARGETS` plus `strict_full_c_addresses.csv` and nowhere else;
   `reviewed_function_identities.csv` is overlay-only, and adding resident rows
   there double-counts the resident total.
   `function_accounting_refresh.py --apply` refuses residents, so a resident
   credit is done by hand, the RES_00BA_1693 way (b0cd541a; #1302 for 0899
   +0000/+0059). Each commit carries six things:
   - the identity;
   - the pinned counts in `resident_inventory.collect()` and
     `resident_callable_coverage.audit_coverage()`;
   - the strict row;
   - a resident manifest `full` row;
   - the regenerated resident ledger, adopted only when exactly that row changed;
   - the checkpoint and battle card, built with the refresh tool's own
     `make_checkpoint`/`render_battle_card`.

7. **No self-credit, no self-merge.** An acceptance PR records strict + identity
   + the schema-9 checkpoint together, quotes author caveats verbatim, and
   leaves merging to the VP when it touches accounting.

## Identity standard (two-factor), adopted 2026-10-20

This is a **correction**, not a relaxation (VP inbox i19579). A body is game code when **both** hold:

1. **Reached from game code.** A call site lies inside a `game_evidenced` or strict-credited function.
   - A **far** call must carry a relocation on its segment word.
   - A **near** call must lie inside the caller's verified ledger extent. A near call has no segment word, so the relocation clause cannot apply to it (ruling i19576, `OVL_0038_101D`).
2. **Operates on documented game state.** The body's subject is a structure that a merged document defines with evidence: a field ledger, a native measurement, or a credited body's evidence doc. State **passed as an argument counts** when the body dereferences or uses it, not when it only forwards it.
   - To check this, trace the pushes at the call site into the body's frame slots, then name the document.

**Why it corrects the `0x7F` verdict.** That verdict required the body itself to address DS game state, which looked in the wrong place.
- `OVL_007F_0054` receives the active-character pointer (`DS:5D92/5D94`) from its relocated caller `0x77:+1E3C` and reads that record's `+0x102`.
- `OVL_0077_275B` (#1191) does the same from `0x23:+024D`.
- Both qualify. The original verdict text stays in [STRICT_CANDIDATE_QUEUE.md](STRICT_CANDIDATE_QUEUE.md), with the dated correction under it.

**What it does not reopen:**
- `RES_0464_2A0D` stays refused: its subject is a BIOS INT 10h service, not game data.
- The four `08AE` driver internals stay refused: they have a driver role and no game state.
- Caller class alone is never identity, and neither is unit co-location or calling a credited resident.

**Table entries (design A2).** A dispatch table is game-owned when its installer is credited. Example: the U38 effect table at `DS:6828+4*type` is game-owned because its installer `OVL_0038_31A2` is identity- and strict-credited (#467).
- Check the slot arithmetic: the entry stub is `0x20 + 5*i`, the installer stores that stub, and the slot is `(store - 6828)/4`.

**Read the CSVs, not the prose, for a row's credit status.** On 2026-10-20 a reviewer took "31A2 is unresolved" from two stale document sentences, while `reviewed_function_identities.csv` and `strict_full_c_addresses.csv` had credited it since 21b65e9a.

## RETF stack-top wrap: a recorded class, deliberately not swept (2026-10-20)

**The class.** Most recovered models read a far-return frame physically contiguous: the CS word is read at `(at + 2u) & MASK`. On 2026-10-20 that held for **96** source files under `src/recovered/`, against **4** that read CS at `SS:((SP+2) mod 64K)`. At a return-time SP of FFFE or FFFF, DOSBox-X normal/386 (binary `99d74601…`) wraps the CS word within SS, while the contiguous models read `SS*16+10000h`. At FFFD, where the word straddles FFFF, every oracle reads it contiguous.

**Why the evidence system was blind.** Unicorn 2.1.4 shares the contiguous reading, so no original-byte Unicorn differential can expose this. Only a DOSBox-X probe with the wrapped and contiguous frame copies given *different* values can. [`tools/overlay/retf_stack_edge_probe.py`](../../tools/overlay/retf_stack_edge_probe.py) is that probe. It ran on SYSTEM Move and GetMem's exit, and it reproduces the 0714 table from #1192.

**Decision (VP i19691): record, state limits where reachable, do not sweep.**
* Two credited rows can reach the band inside their stated contracts: `OVL_0025_1951` (entry SP 0006/0007 and FFFE/FFFF) and `OVL_009E_0E52` (000E/000F and FFFE/FFFF). Both now carry a stated limit, and none of those SPs was exercised.
* The runtime endpoints SYSTEM GetMem `023F` and the Move leaves (`1685`, which shares `retf()` with `0B57`, `0C10` and `0C3B`) carry the limit too.
* **Reachability (reviewer-added 2026-10-20, VP i19720).** The band is unreachable under the game's stack, not merely unexercised. EXEPACK restores SS:SP = `14F2:4000` (16 KiB, even, growing down), and a static sweep finds no `MOV SS`/`LSS`. Reaching the band needs ≥16,370 B of depth or a wrapped SP, and both already corrupt memory above the stack, where Turbo Pascal places the heap. It rests on that stack geometry, on no in-game stack switch, and on no ISR running these bodies on a private stack. **Reopen the model fix only on one of three triggers:** (a) an ISR or driver body that runs on a private stack; (b) a program or segment whose stack lets SP start near the top, such as `$M 65520` with SP starting near FFF0; (c) a game path demonstrated to reach depth >16,370 B without corrupting memory.
* Fixing these means re-accepting evidence for a divergence that needs a stack wrap nothing reaches. A sweep of the 96 files would mean a fortnight of re-acceptance for an unexercised edge. Revisit only when a body needs the band.

**Acceptance rule (from this finding).** A body that composes a stack-frame reader, i.e. any callee whose RETF or POP frame it reaches, must have its **entry-SP window stated at acceptance**. An ARGUMENT guard that checks the RAM, CPU, size and IP but **not the entry SP** is the pattern to flag: `1951` and `0E52` were accepted that way. Compute the callee's entry SP from the caller's stack delta at the call, and either bound the caller's entry SP so the callee cannot wrap, or state the limit.

## A share of test cases is never a frequency of behaviour (VP i20118, 2026-10-20)

Every x/y in an evidence doc is one of two things: a share of the **cases chosen** for coverage, or a
measured **frequency of behaviour**. A reader cannot tell them apart unless the sentence says which. Write
case shares as "(n/m cases by construction)". State a frequency only when it was measured, and give the
measurement.

Precedent: in ReadKey (`CRT_READKEY_030F_CPU.md`), BREAK is 25/92 Unicorn cases by construction. Its
trigger in play is the user pressing Ctrl-Break; its frequency in play is not measured. A draft instruction
to call it "a live outcome of ordinary keyboard state" was withdrawn for exactly this reason.

## Named exclusion: "a negative load base" (VP i20208, 2026-10-20)

Some alias states need a relocated segment value that the loader can only produce by loading the program **below its
own start**. In real mode, a segment `S` in the image relocates to `load_base + S`, and `load_base` is never negative. So
a value `< S` is not constructible for that segment.

When a feasibility argument reduces to "this needs segment `S` to relocate below `S`", it is finished in one line: cite
this section, state the required value, and state the segment. Construct the case first, as the precedent did, because
the reduction is only trustworthy once the alias itself has been built and observed.

Precedent: `RES_0899_0059`, 017B's `JMP` at +018B (#1298, `RES_0899_READKEY_0059_CPU.md`).
- The alias was **built**: load base 1069h, SS = DS, SP 5478h.
- 017B was never entered. The earlier ReadKey call's CS push, at the same SP, set [5373] to CS's high byte, so the
  set-IN test failed.
- Passing that test needs CS = 0002h or 0102h for segment 0899, which is a negative load base.

The run disproved the author's prediction (outcome 1, reachable) and the doc reports the run.

## Four false-prior classes (each has a filter and a regression test)

1. **Boundary assertions** — `address + size == 0x…` end-of-previous-body and
   next-prologue comments. Example: an address-range comment `+2A87..+2AE6`
   counted as an inline.
2. **Call-site maps** — `0x0829: 0x047A` proves a *call*, not a translation.
3. **Span hashes** — a recorded `(0xA,0xB): "hex…"` digest is evidence of
   decoding, not of inlining.
4. **`ip = 0x…` redirects and bare comment mentions** — a loader handing
   control to an address (`07BC`) looked modelled. The inline-prior filter
   re-ranked 23 rows; largest correction 48→4.

## Known queue state (do not re-derive by hand)

On main 6bdc0219 the credits U9E +115C, RES_00BA_0BB8/+1168/+13AB and U33 +12E1 unlocked **no** new caller: the queue stays at 457 candidates / 59 composable / 9 interface caveats, because the two callers those bodies reach are blocked on other callees — OVL_0033_1E31 (nine unready: three untranslated, one partial, one modeled, four resident) and OVL_0038_2F57 (OVL_009E_0026 plus RES_00BA_16EC).
Sizing them shows no cheap leaf: the resident blockers are 26-263 bytes and only the 26-byte RES_0899_0122 gates nothing alone, while RES_00BA_16EC carries 17 rows but a 6,312-byte/65-routine closure, so its earlier hold stands.

## Two readiness facts that look like progress but are not

**Every readiness claim must consult every product that defines readiness.**
A tool that consults three of four produces a number that is true of its own
inputs and false of the tree (VP i20231). The VP counts five instances on
2026-10-20, every one overstating what is missing. Three of them:
- `006C_3291` stayed `untranslated` after it was credited.
- `indirect_unresolved` was left out of `unready`.
- The declared `0AF8:16BC` read `runtime_unmapped` at its overlay call sites.
  The call graph resolves SYSTEM targets as runtime, and the runtime branch never
  consulted `DECLARED_BOUNDARY_ENDPOINTS`. The transitive pass had the same gap
  for declared and `driver_internal` resident targets. Both passes now go through
  `runtime_readiness` / `resident_target_ready`. Fixing it moved 3B93 2→1 and
  `006C_0161` 14→13 unready, and changed no row's composability.

**Enumerate blockers with the code's own readiness sets, never a hand-written
list of states that look unready.** `ready = strict + modeled +
resident_covered + runtime_covered + declared_boundary + driver_internal`;
everything else is unready. A hand
enumeration that included `modeled` turned an 8-blocker caller into 9 and
promoted a ready row into a blocker, which produced a recommended "cheap"
declaration that was functionally inert.

**An overlay manifest `full` row does real work; check the code, don't take this sentence's word.** (This paragraph
used to say that overlay manifest entries "confer nothing"; that was false.) In `tools/overlay/strict_candidate_queue.py`
(the line numbers are at main `c9de4a43` and move with edits, so search for the quoted expressions):

- **It is a reviewed endpoint of the transitive pass.** Line 749 builds
  `endpoints = {node for node in nodes if node in state["full_models"]}`. Line 752 counts an endpoint as ready when it
  is modeled. Line 758 then skips endpoints when propagating unreadiness, so a `full` row stays ready **regardless of
  whether its own callees are ready**.
- **It also makes the body `modeled` by itself.** Lines 903-904 add the row's evidence doc to `main_files`, which is
  what line 616 tests for `modeled`.
- **`partial` is the other status**, and it marks the row *unready* (line 615).
- **Strict credit dominates both paths:** `strict` is tested first at lines 614 and 735. So once a row is strict-credited,
  its manifest row confers nothing further.
- **Measured for `OVL_03B0_06FB` (2026-10-20, par-e):** regenerating the candidate queue with and without its `full` row
  gave identical results. All 13 dependent rows see `modeled` either way, no `callee_readiness` differs, and the totals
  are 447 candidates / 58 composable in both. The effect is **latent** because 06FB's own callees are already ready. It
  would matter the moment they were not.
- **06FB is refused identity and stays evidence-only:** factor 2 fails because its 13 instructions have no memory
  operand, so its subject is a display service and the 2A0D precedent applies (VP i20054).
- **06FB's manifest `full` row remains a live readiness declaration and is deliberately kept.** Its endpoint shield is
  latent, as measured above.
- **The deletion path is the rule for a row that IS credited:** strict credit makes its manifest row redundant, so the
  row is deleted inside that row's acceptance commit, with a measured zero readiness delta. Nothing is pending for 06FB.

The manifest gate that makes `RES_00BA_1168`/`13AB` work is a **different, resident-only** mechanism (`resident_full`,
`resident_covered`). The loader accepts it only for strict-credited residents, as measured for `RES_0464_1951`.

## The loader-expanded offset trap (authors and reviewers both hit it)

Resident bodies live at `physical(segment, offset)` in the **loader-expanded
START.EXE image**, not at a raw file offset. `0x464:0x2595` is image offset
`0x464<<4 + 0x2595 = 0x6BD5`; hashing `START.EXE[0x6BD5:0x6C72]` directly
returns a plausible-looking wrong digest and looks like a body mismatch rather
than a method error. Unpack with the loader first (`resident_loader_map.unpack`)
and hash from that image. **A decoy makes the wrong answer look right for
twelve bytes:** `GAME.OVR[0x2BEBC]` reproduces the `RES_00BA_13AB` prologue
(`[scrubbed: span=REVIEW_LANE_HANDOFF-1 sha256=a9682f5ba1b0db0f3bad1642747848046de9cedf3bfda049abd844f66019ef03 bytes=12 offset=GAME.OVR:0x2bebc]`) and diverges only at byte 12
(`b0 19` vs `0e e8`); the real body is START.EXE `[0x2027,0x2107)`, 224 B,
SHA-256 `22732f3c…`, and does not occur in GAME.OVR at all. Two independent agents — the reviewer and a headless
free-model screen — made this exact mistake on the same body before finding the
bytes, so treat a first-attempt hash mismatch as a method suspect before
reporting a defect.

**Example of a rule earning its keep.** The point-in-time digest rule was
written into the queue document and, on the very next turn, a case it covers
arrived: the digest moved (`9b04734f` -> `4dba801e`) with counts identical at
455 candidates / 63 composable / 9 caveats, purely from branch-ref advance.
Following the rule meant leaving the pin alone and saying so. The alternative —
"the digest no longer reproduces, re-pin it" — is a change made for zero
semantic reason, and it is exactly the churn the rule exists to stop.

## Cost estimates are floors, and the error has a direction

**Estimates have only moved one way.** Every measured replacement so far has
been larger than the estimate it replaced: `OVL_0025_0C77` went from a 405-byte
queue row to >=1,672 B measured, and my own 1,011 B estimate was itself 65%
under. Seven rows have carried trailing data that a first-`RETF` measurement
removes or reclassifies. So treat an unconfirmed figure as a **floor**, not a
measurement, and pre-correct in that direction rather than being surprised:
when quoting a chain's cost before it is measured, say "at least" and expect to
raise it. The measured table lives in `STRICT_CANDIDATE_QUEUE.md`; select on
cost per unlocked row, never on the queue's byte fields.

**Repair budget for a single tool: five.** The pricer needed five consecutive
fixes in one evening — a gate that could not fire, an input nobody could
produce, a happy-path `NameError`, a doc that argued with the print, and a stale
row — each correct on the path it was written for and wrong on an adjacent one.
The acceptance above is what stopped the sequence. **If a sixth fix is needed in
the same area, stop patching and rewrite the entry point instead**; five repairs
to one seam is evidence about the seam, not about the five patches.

## A linear sweep counts data as code

Two numbers for the same resident differed by 11x tonight: `RES_0899_0059` had
**98** raw decoded `lcall 0899:0059` hits in my sweep and **9** fixup-backed call
sites in the ledger and in the other lane's receipt. The 89 extras were **data
bytes decoded as instructions** — a gap-skipping sweep walks through data. The
verified count matches the ledger exactly (`overlay_far_sites 9`).

Rule: a call site is real only if the **segment word carries a relocation** (or
the ledger lists it). Never quote a call count from a raw sweep; a sweep tells
you what the bytes *could* decode as, which is why it is useful for extents and
useless for centrality.

## Primary acceptance for the pricer (run this, not the unit tests)

In a checkout **with the owner's media and the accounting products**, with the
queue input deliberately absent, run exactly what the docs say:

    rm -f resources/strict-candidate-queue/queue.json
    python3 tools/overlay/price_candidates.py

Require: exit 0, a `generating the queue input (missing)` line, then the priced
table (`OVL_0077_275B … 132`). `tools/overlay/test_price_candidates_integration.py`
automates this and skips where media is absent — which is why CI cannot catch a
happy-path defect here, and why three fixes in this area were each correct on the
path they were written for. The unit tests mock the dependency check; this does
not.

## Local setup (read this before reporting a gate failure)

The checker gate needs Capstone 5.0.7 and Unicorn 2.1.4, which are installed in
CI but not in this host's system Python. If they are missing the gate now stops
with a distinct line — `ENVIRONMENT MISSING: ... not importable - this is NOT a
tree failure` — and exit code **3**, so an environment error cannot be mistaken
for a broken tree. To run it locally:

    python3 -m venv .venv
    .venv/bin/pip install capstone==5.0.7 unicorn==2.1.4
    .venv/bin/python tools/overlay/check_checker_builds.py --all

## Two CI gates, and how to prove they fail closed

1. **SHA-provenance guard** — `tools/overlay/check_record_shas.py`. Every
   40-hex id in the five accounting records must resolve via
   `git cat-file -e` or be marked external/unknown. *Proof:* inject a fabricated
   id → exit 1 with exactly one failure.
2. **Checker handoff gate** — `tools/overlay/check_checker_builds.py`.
   `--all` blocks on the 3 registered rows; `--sweep` is a non-blocking report.
   *Proof:* delete a source from a registered checker's `cc` list → exit 1
   with the real undefined symbol; restore → green. The stale-tree precondition
   is mode-aware: strict locally, relaxed to a merge-base relation in
   `pull_request`/`merge_group` CI (where main moves under the PR head), and
   `--require-current-main` forces the strict form anywhere. The compilation
   itself is never relaxed.
   Sweep baseline: 143 of 264 checkers compile — a positive baseline, **not** a
   drift signal while 121 remain unextractable. Register a row's checker when
   that row is next touched.

## Queue workflow: regenerate, filter, counterfactual, author

1. **Regenerate** the pinned products, then the queue export (recipe in
   `STRICT_CANDIDATE_QUEUE.md`). On 235/281 it is 459 candidates / 57
   composable / 9 interface caveats. Re-derive, then diff prose against the
   export — do not trust the doc.
2. **Filter** with the false-prior classes. A rank move is expected; a count
   change is not.
3. **Counterfactual** before proposing anything: what does this row's absence
   gate? Blocker fan-out is not cost — deep dependency trees are not cheap wins.
4. **Author** — the author lane writes the body and evidence; the review lane
   accepts independent of the author and records the credit.

## Three things we decided NOT to do

* **Hold `OVL_0086_08C3`** — it alone gates 32 rows (28 of them
  `game_evidenced`), but its transitive cost is 6,257 bytes / 2,541
  instructions across 11 leaves at depth 7, so it is not the cheap unlock its
  gated-row count suggests.
* **Hold `RES_00BA_16EC`** — no leaf of 16EC can become strict until 16EC itself
  is; the port harness stays spec-only.
* **Defer `RES_00BA_0BB8`** — 5–13 days, and it is what `+115C` is blocked on.

## Where the truth lives

`docs/BATTLE_CARD.md` (measured format) · `docs/re/FUNCTION_ACCOUNTING_REFRESH.md`
· `docs/re/STRICT_CANDIDATE_QUEUE.md` (pins, point-in-time by design) ·
`docs/re/QUEUE_METADATA_REPAIR.md` (extent repairs) ·
`docs/re/partial_callee_models.csv` (readiness only) · the measured
selection-cost table in `docs/re/STRICT_CANDIDATE_QUEUE.md` (cost per
unlocked row — never select on the queue's byte fields) ·
`docs/re/OVL_0068_0294_DISPOSITION.md` (the last uncredited row, and why).

## `reviewed_commit`: definition (VP i20093, 2026-10-20)

`reviewed_commit` in `reviewed_function_identities.csv` is **the commit at which the reviewer verified the row**. It takes one of two forms:

1. **The author PR's merged head**, when the evidence and the identity argument landed together in that PR. This is the usual case, and the case the credit audit below checks: its eight rows all take this form.
2. **The commit that contains the identity argument**, when that argument landed separately after the evidence. The author PR stays in `review_pr` and is named in the row's evidence doc.

**Instances of form 2:** `OVL_03B0_0C33`, `OVL_03B0_0A08`, `OVL_03B0_07A4` and `OVL_03B0_01DA` (credited 253 to 256) carry `5b225a62637afc5ea3036bea5958ef48cae6bcdc`, from #1262.
- Their author PRs (#1209, #1204, #1202, #1216) contained the evidence but no identity argument. The argument was written later by par-a, in each doc's "Identity under the two-factor standard" section.
- These four were deliberately **not** normalised to the author heads. That would cite commits that do not contain the argument, so the column would be consistently wrong rather than ambiguous.

**To audit a row:** check `reviewed_commit` against the merged PR head; if they differ, check that the commit contains the row's identity argument and that `review_pr` names the author PR.

## Credit audit, 2026-10-20: today's eight (par-b; corroborated by par-c)

Checked on main `37ab96e4`, with checkpoint commit `8e54be4b` (244/290) as the
base. The check covers the accounting as a whole, not the bodies, which were
reviewed per row. Each strict row and identity row was checked for: one per
credit; the doc carrying that body's claim; the body SHA and byte count
hash-matching GAME.OVR at the ledger offset; and `reviewed_commit` equal to
the merged PR head.

| Credit | PR | Merged head = `reviewed_commit` | Body |
| --- | ---: | --- | ---: |
| OVL_0077_275B | #1188 | `f50a9c10b022c08140475ab065cbfe504b7c92b7` | 132 B |
| OVL_0038_3134 | #1193 | `959a5b849dc188024fe7440975cfc0a9ff0e689b` | 70 B |
| OVL_0038_101D | #1198 | `0c0b9571c91e142142b356ca271f613519515837` | 105 B |
| OVL_0038_2E08 | #1200 | `5d69fc3996ff68c7112403acff934d715b00e8e8` | 47 B |
| OVL_006C_3291 | #1199 | `be2c742de68f83d1395bf986396fb84f38beb7bc` | 326 B |
| OVL_03B0_11F3 | #1206 | `da1d6f066f2619184d579c79ddc8cdcbc45fbd49` | 82 B |
| OVL_0025_1CC3 | #1227 | `0ec816a384e2b327a89deadc919f1fc8fb9e5127` | 301 B |
| OVL_0038_12B2 | #1224 | `ce8e785cb1bffcb4d7910e0c609d03c6f4b864a1` | 73 B |

Result:

- The delta is exactly +8 strict and +8 identities, these ids, with nothing
  removed.
- `overlay_classification.csv` moves exactly these eight to `game_verified`.
  Its other 27 changed rows change only the derived `ver_calls`.
- `function_queue_audit` regenerates the classification and the ledger
  byte-identically, and the canonical queue reproduces pin `aa3bad91`.
  Counts re-derive as 252 (overlay 226, resident 26) and 298 (271 / 27).
- One anomaly: OVL_006C_3291's ledger bound (342) includes 16 data bytes after
  its `RETF 4` (`00 0E "Head Body Keep"`), while the credited body is 326 B.
  The repair is par-c's one-row extent fix.

**Looks like a duplicate, is not.** `OVL_0038_0647`, `OVL_0038_1355`,
`OVL_0038_135C` and `OVL_0038_319B` share one `body_sha256` (`3d5d3102…`) in
`reviewed_function_identities.csv`. They are byte-identical bodies at four
distinct addresses, each a separate function, so they are correctly counted
four times. A duplicate-SHA check will flag them; it should not treat them as
a double count.


## Required owner-media receipt (t119)

Every PR and independent reviewer verdict must include the literal line
`ran run_media_checks.sh: result`, filled with the reviewed head, outcome,
test count and skips, or the exact NOT RUN reason. A missing/NOT RUN receipt
is not media validation. Before accepting a change affecting media-dependent
behavior, run `tools/run_media_checks.sh` from an owner-media checkout.

The command checks `resources/DOS/POOLRAD/{START.EXE,GAME.OVR}`, uses the shared
`../poolrad-heavy.sh`, runs every `tools/overlay/test_*.py` test plus
`tests/recovered/test_u6a_0824_fullc_gate.py`, and exits nonzero on missing media,
missing wrapper, test failure, or skipped coverage. Install the checker Python
dependencies and provide current accounting products under ignored `resources/`.
The command regenerates a missing or stale queue export from those products;
missing products or skipped coverage cannot be counted as a pass.
New media-dependent unittest files outside these discovered locations must be
added to `tools/run_media_checks.py`. This is the repository's media-dependent
unittest gate, not execution of every standalone row audit or native emulator.

Public CI does not possess the owner's game media. It prints a named
`media checks NOT RUN: no owner media` warning and summary, and runs the emitter
set, template-helper and missing-media refusal tests only. A green CI result
does not certify emitter reproduction or the full owner-media suite. The
local command runs all eight emitters with `--check`; it never repairs generated
files as part of verification. Media and derived captures stay ignored.

## Media-host differential gate (t79)

Before merging recovered-C changes, run the affected registered checker
contracts with `check_checker_builds.py --execute ROWS`, or select shared-source
dependents with `--all-touching FILE`. CI's compile-only result is insufficient.
Follow [the media-host execution rule](CHECKER_EXECUTION_GATE.md), including its
conservative source-selection scope and 0/1/4 result contract. This runs on a
media host, not in CI, and the gate itself serializes jobs through the shared
heavy-job wrapper. Retain its receipt under ignored `resources/`.

## Verdict states: ACCEPT, WITHDRAWN, SUSPENDED (VP i25889, CoS i24714)

A verdict is one of these three, and the word used matters because a reader six weeks later acts on it:

- **ACCEPT** - the claim was re-measured by the reviewer on the named head and holds. Naming the head, what was re-run and what was NOT verified is still mandatory.
- **WITHDRAWN** - do not use this head or its receipt at all. It is superseded; nothing in it may be cited, batched or merged, and any earlier acceptance of it is void.
- **SUSPENDED** - this named part is intact, and that named part waits for a NAMED pending verdict. Suspension is not a soft acceptance, and it carries three rules:
  1. The pending verdict must be named in the same sentence: the reviewer and the PR/head it will be issued on. "Waiting on review" is not a suspension.
  2. A suspended part NEVER counts toward credit, readiness, membership or any denominator, no matter how green the intact part is.
  3. A suspended part must be re-verified on the new head. It is never carried across heads: the same evidence measured on an older sha is historical, not evidence for the current one.

   **Blob-identical restacks are the exception, and they are not a carry.** When a reviewer confirms by explicit blob comparison that every task file is byte-identical to the head the evidence was measured on, that is the *same* evidence on the *same* bytes, and it may be transferred to the restacked head without a re-run. What rule 3 forbids is carrying a measurement made on *different* bytes. Say which comparison you made; "it looked like the same diff" is not a blob check.

**Merge-then-verify, and what the merge is not (VP i26020).** Standing condition 1 says never merge an unverified PR. One named exception exists: a blob-identical restack of an **accepted, media-gated** head MAY merge, when the reviewer names the single gate that will run on exact `main` and that gate runs immediately after the merge. Red means a REVERT report against the main commit, not a follow-up: the merge is not the verdict and carries no credit. Everything else still needs its own gate before merge.

Precedent, 2026-10-25: the byte-dump rule (PR #1575) was "frozen and double-gated" - author gate plus independent gate on the same frozen head - before anyone merged it. The wording is the protection: one green run on a rule-enforcement tool is not a verdict.

## Parity claims carry their case set, not a count (VP i25889)

This section exists because a review accepted a CPU-parity claim on the strength of a count. Any CPU-parity or "C-verified" statement is only as strong as the cases behind it, so:

1. **Every claim carries its case set and how each case was chosen** - author-chosen, adversarial-by-construction, or reviewer-chosen. A bare "N/N equal" is not evidence; say what the N was and why those cases exist.
2. **A reviewer verdict on a parity claim includes at least one REVIEWER-CHOSEN case per branch and input dimension of the body** - for example entry value, flag combinations, buffer offset, aliasing - and reports the reviewer's cases pass/fail SEPARATELY from the author's. Re-running the author's set proves the harness works, not that the body is right.
3. **"The cases were authored by the same lane" is a reason to demand adversarial cases**, not a footnote to excuse the gap. Name the inputs, not the count.

The reviewer may add mutants the author did not write, and should when the body's risk is a wrong operation rather than a missing one: an independently authored mutant that survives a differential is a finding about the differential, not about the mutant.

**Prospective.** This standard applies to verdicts issued after this section merges. Verdicts already issued stand as issued, with the limits they stated: retro-applying it would convert recorded evidence into a rule violation after the fact, and that is worse for the record than a stated gap.

## A verdict must come from a row the profile executed (VP i26338, t171)

A producer must never attach a verdict to a row its profile did not execute: it emits **NOT-COVERED**.

- **NOT-COVERED is a valid answer.** A producer must report every uncovered row explicitly; silence must not be treated as coverage - and a run whose profile covers its whole scope legitimately emits none. Requiring the outcome when a scoped row was not executed is the rule; requiring it always would condemn complete coverage.
- **AGREE on an unexecuted row ends a question that should have been asked.** It is the most expensive kind of green, because nothing downstream can tell it apart from a real pass: the row looks verified, the receipt looks complete, and the gap only surfaces when someone re-derives the run.
- The check is cheap: for every row in a profile's scope, either the run executed it (AGREE/DIFF as measured) or it names NOT-COVERED. Silence is neither.
- Reciprocally, a reviewer reading a producer's report must treat a missing row as NOT-COVERED rather than as a pass, and must say which rows the report covered.

Found on #1566 at `400f3a4e`; the reporter-side fix is t171. This belongs here rather than in a PR comment because the rule outlives the incident.
