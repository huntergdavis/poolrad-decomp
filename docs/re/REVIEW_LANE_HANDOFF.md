# Review lane handoff (Poolrad strict-C)

You verify other lanes' PRs. You do not own credit, you do not merge your own
body, and you report every outcome to the VP pane. Current checkpoint on main:
**235 strict originals / 281 verified identities**, 214 overlay credits. The
whole-game denominator is unknown — never state a completion percentage.

## Seven standing conditions

1. **Verify before merge.** Never merge an unverified PR. Breaking the claim is
   the standard: delete a source, inject a bogus id, re-run.
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

7. **No self-credit, no self-merge.** An acceptance PR records strict + identity
   + the schema-9 checkpoint together, quotes author caveats verbatim, and
   leaves merging to the VP when it touches accounting.

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

**Enumerate blockers with the code's own readiness sets, never a hand-written
list of states that look unready.** `ready = strict + modeled +
resident_covered + runtime_covered`; everything else is unready. A hand
enumeration that included `modeled` turned an 8-blocker caller into 9 and
promoted a ready row into a blocker, which produced a recommended "cheap"
declaration that was functionally inert.

**Overlay manifest entries confer nothing.** Overlay readiness comes from
strict credit, a `partial` flag, or `modeled` presence in the tree; the
manifest is consulted for overlay rows only to mark them `partial`, which
counts as *unready*. The manifest gate that makes `RES_00BA_1168`/`13AB`
work is a **resident-only** mechanism, so declaring an overlay row `full`
is accurate documentation and no readiness change at all.

## The loader-expanded offset trap (authors and reviewers both hit it)

Resident bodies live at `physical(segment, offset)` in the **loader-expanded
START.EXE image**, not at a raw file offset. `0x464:0x2595` is image offset
`0x464<<4 + 0x2595 = 0x6BD5`; hashing `START.EXE[0x6BD5:0x6C72]` directly
returns a plausible-looking wrong digest and looks like a body mismatch rather
than a method error. Unpack with the loader first (`resident_loader_map.unpack`)
and hash from that image. Two independent agents — the reviewer and a headless
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
`docs/re/partial_callee_models.csv` (readiness only) ·
`docs/re/OVL_0068_0294_DISPOSITION.md` (the last uncredited row, and why).
