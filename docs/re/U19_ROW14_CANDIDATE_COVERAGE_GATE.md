# U19 row-14 candidate-stream capacity gate

This is a **source-only** capacity and offline stream-shape check for the
fixed U19 `+02D8` printer route. It supplies no new native stop, writer,
four-plane delta, page, RGB, C parity, or full-frame result. The host/native
slot remains held. The bounded `deja "Poolrad U19 row14 candidate_hits
store_ordinal 128 897 target filter"` query timed out after eight seconds
without a result. This gate reuses pN's private conditional hit-budget
audit and the merged [36-cell CPU-source model](U19_ROW14_CPU_STORE_SOURCES.md),
[target filter](U19_ROW14_TARGET_FILTER_PREFLIGHT.md), and
[capture source preflight](U19_ROW14_CAPTURE_SOURCE_PREFLIGHT.md).

## Counter and route bound

The public `filter_frames` increments `candidate_hits` on **every supplied**
printer `+0412/+0438/+0445/+0451` PC frame after the U19 call, including a
wrong `ES:DI`. That count is neither a matching-target ordinal nor a count
of all guest memory writes. Its 128-hit cap and the same limit in the local
selected-host witness cannot cover an exhaustive accepted EGA transcript to
the `A000:118F` witness.

The merged C source model emits eight rows, four plane loops, and two page
stores for each of the 36 fixed cells: **64 candidate-PC executions per
cell, 2,304 across the entire conditional route**. Fourteen earlier cells
give 896 preceding candidates; the earliest possible target is ordinal
**897** on the first page, or **898** when only the second page maps to
`A000`. These are source bounds under the accepted EGA path, **not**
observed native counts or a guarantee that the retained route takes that
path. A changed path, relocation, page segment, repeated print, or missing
candidate is a typed stop requiring fresh evidence.

`tools/replay/cold_start_codeword_row14_candidate_coverage_gate.py` checks
one claimed full fixed-row stream with a bounded iterator. It expects a U19
`+02D8` caller frame, all 2,304 ordered candidate frames, the first target's
immediate pinned successor, and U19 `+02DD` return. It checks each candidate
PC and effective page segment/byte offset against the conditional source
model, counts wrong-address frames, and rejects legacy-cap exhaustion,
sparse or reordered frames, wrong successor, absent return, and overflow.
The compact JSONL stream has at most **2,307** records of at most 256 bytes
each. Even a structurally complete stream returns
`SOURCE_ONLY_SHAPE_UNAUTHENTICATED` and `native_coverage_proved=false`:
supplied records cannot authenticate omitted debugger stops or source-run
identity by themselves.

## Pinned debugger record boundary

Ignored DOSBox-X source revision `5a9c7c53481c2e933948039c33256776653f4faf`
has `src/debug/debug.cpp` SHA-256
`2762251108e54aeec3474b9f495222ce09bec9f246dd0af53a4d10fa7bb2a38d`.
The checker pins six exact windows: `664–676` breakpoint construction,
`729–793` breakpoint checks, `2505–2522` command parsing, `2763–2799`
bounded log commands, `5430–5502` instruction record fields, and
`6009–6030` logging/stop progression. The `BP` command takes only a
segment:offset and stops execution; `BPM` compares one guest-readable byte.
Neither command implements an `ES:DI` predicate or produces an automatic
complete candidate-frame transcript.

The heavy debugger's `LOGL` command accepts a hexadecimal **instruction**
count and emits CS:IP, ES, and EDI in `LOGCPU.TXT`. `LOGC` omits ES/EDI.
`LOGL` can therefore be a source candidate for post-filtering a bounded
instruction interval, but its count is not a candidate-hit count; `LOGL`
itself does not bind the count to U19 `+02DD` or capture a plane pair.
No route-sized `LOGL` count, uninterrupted file-completion recipe, or
same-run extraction provenance has been validated. A future driver must
retain and hash the raw log, prove start/end boundaries, all breakpoint or
instruction-record coverage, and every extracted candidate before this
offline shape can support a native completeness claim. A private VGA
write-handler hook is another conditional design, but generic and fast
unchained handlers differ and actual handler/paging identity is unmeasured.

The current 128-hit target filter, event-plan and host witness contracts
remain unchanged. pC and pR must review any shared cap/ordinal schema before
use; a sparse target-only stream cannot silently redefine
`store_ordinal`. Source-only reproduction without media or native execution:

```sh
python3 -m tools.replay.cold_start_codeword_row14_candidate_coverage_gate \
  --source resources/dosbox-x-16a1-private-src
python3 -m unittest \
  tests.replay.test_cold_start_codeword_row14_candidate_coverage_gate
```

Original media, debugger logs, CPU/RAM records, EGA planes, and pixels stay
in ignored `resources/`. Only addresses, hashes, bounds, and synthetic
events are public.
