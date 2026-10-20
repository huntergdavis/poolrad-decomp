# Identity-sweep design note (proposal, not a decision)

**Why this is the biggest gate.** In the modelled-but-uncredited set, identity
blocks more bodies than paperwork and translation combined: the queue-scope audit
finds 52 of 73, the doc-based map finds 23 of 25. Translation cannot start until
those rows have an ownership answer, and no identity rule exists beyond "a
reviewer decided".

**No identity is decided here and no classification is touched.** This note
proposes the tests, the classes, and the counts that a sweep would produce.

## The three classes and the test that defines each

### (a) Decidable from the media, now
A row lands here if **any one** of these holds and can be shown from files in the
repo:

* **A1 — fixup-backed caller in a credited function.** The call site carries a
  relocation on its segment word and the enclosing function is in
  `strict_full_c_addresses.csv`. Test: enumerate callers from the ledger's
  fixup sites, resolve each enclosing row, require at least one credited.
  (This is the same test that cleared U33 `+12E1` via its callers.)
* **A2 — game-owned dispatch table.** The row is an entry of a table whose
  *owner* is game code: the unit descriptor's `memory_segment`/`entries` plus a
  credited caller that addresses the stub slot. Test: entry index derived from
  the stub slot (`0x20 + 5*i`), target equal to the row, and the addressing
  caller credited.
* **A3 — documented game behaviour.** The body reads or writes state that a
  merged document defines as game state (e.g. `DS:0x5372` mode byte), *and* no
  contradiction exists. Test: the doc names the address and the row's decode
  touches it.

### (b) Decidable only with new capture evidence
A row lands here when it has **no credited caller (A1)**, **no game-owned table
entry (A2)** and **no documented state (A3)**, but a natural reach exists: a
fixup-backed caller in an *uncredited* function, or a reachable entry through a
table whose owner is unresolved. The test requires a capture of that path — a
native trace showing the body running with game state — before the questions
above can even be asked.

### (c) Stays unresolved
No credited caller, no table entry, no documented state, no reachable path, and
a closure that is itself identity-unresolved (`RES_0899_0059`'s four ordinary
helpers are the worked example). Record it as unresolved; do not launder it
through a declared boundary.

## Counts from a screening pass (not a sweep)

Screened set: the **20 overlay rows** that the doc-based map finds modelled,
uncredited and identity-blocked. Signals: `callers_fixup_backed`, credited
callees, and `indirect_unresolved` from the queue.

| class | count | rows |
| --- | ---: | --- |
| **(a) media-decidable** | **7** | `OVL_006C_3291` (7 callers), `OVL_0077_004A` (3), `OVL_009E_0000` (11, dispatch), `OVL_009E_0026` (31, dispatch), `OVL_009E_02FA` (21), `OVL_03B0_03BD` (35), `OVL_03B0_06FB` (19) |
| **(b) needs capture** | **13** | `07BC`, `101D`, `2E08`, `2F57`, `0294`, `056C`, `2033`, `007D_0000`, `05F0`, `0160`, `03B0_01DA`, `07A4`, `0A08` — all with **zero** fixup-backed callers in the ledger |
| **(c) unresolved** | 0 in this set | the class is populated by the `0899:0059` closure, which is not in this set |

If the queue's larger 52-row set behaves the same way, the sweep would convert
**roughly a third** of the blocked rows to "creditable without translating
anything" and leave two thirds needing a capture.

## Worked examples that define the tests

* **(a) by A1 — `OVL_03B0_03BD`:** 35 fixup-backed callers, one credited callee,
  no dispatch. The test is caller resolution alone; if any enclosing function is
  credited, this is decidable today.
* **(a) by A2 — `OVL_009E_0000` / `OVL_009E_0026`:** both carry
  `indirect_unresolved` and large fixup-backed caller counts (11 and 31). The
  question is not "who calls it" but "who *owns* the table that dispatches into
  it" — decided by the unit descriptor plus a credited adaptive caller.
* **(b) — `OVL_0068_0294`:** three credited callees, a complete differential, a
  documented AF exception — and **zero** fixup-backed callers. Its identity
  cannot move on caller evidence; it needs a capture of a real path, or an
  explicit decision to accept behaviour-plus-fields as ownership.
* **(c) — the `RES_0899_0059` closure:** four ordinary helpers with zero overlay
  CALL fixups, parents unresolved in the ledger, two documents already recording
  identity as unresolved. Unreachability of *ownership*, not of code.

## What a sweep would need from the reviewer

1. Run A1 (caller resolution) over every identity-blocked row — mechanical, no
   judgement.
2. Run A2 (table ownership) where A1 fails and `indirect_unresolved` is set —
   mechanical once the descriptor parse is written.
3. Refer only the residue to human judgement, with the capture requirement
   stated per row.

That is the shape of the question for the owner: **a sweep converts caller-
evidenced rows mechanically; it does not convert behaviour-only rows.**
