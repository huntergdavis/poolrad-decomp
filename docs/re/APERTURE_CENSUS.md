# The aperture census: which models can reach A0000..AFFFF through a pointer they were handed (VP i20701, 2026-10-21)

**A green differential on a flat model is NOT evidence that the model is aperture-safe, and never was.** Every flat model's audit runs a
Unicorn oracle that maps A0000..AFFFF as ordinary RAM, so the flat model and the oracle agree about a pointer into the aperture **by
construction** (not by fixture luck). 144/144 equal covers every value except the one that matters here. The census therefore emits
the absence of evidence as a column, beside coverage: `aperture_observable = no` for every flat model, in
[`aperture_observability.csv`](aperture_observability.csv), and a test (`test_aperture_census.py`) fails if a model source is
added without a row. Same discipline as naming an incomplete audit as zero coverage.

## The census (tools/overlay/aperture_census.py)

| | models |
| --- | ---: |
| recovered model sources | 548 |
| **bus-aware** (take `pr_bus`; an aperture pointer reads what the machine reads, observable against an EGA oracle) | 32 |
| **flat** (aperture **not observable in their audits**, marked in the CSV) | 516 |
| flat models whose header or opening comment states an aperture exclusion | 4 |

The subject is not the 516 in isolation but the **pair**, because 0411's shape is a bus-aware body handing a pointer to a FLAT callee:
`aperture_pairs.csv` lists every (bus-aware caller, flat callee) pair: **71 pairs**, of which **46 take a data pointer** (StrLoad, StrStore,
StrCopy, StrCat, Str, Val, Move, FillChar, set membership/expansion, GetMem/FreeMem, node and operand readers). For each of the 46 the
pointer's **source** is classified, because only a caller **argument**, a **global** (DS:5D92, the script pointer) or a pointer the callee
**returns** can be corrupt; a body-built **local** or a **code literal** (a string in the caller's own segment) cannot reach the aperture
except through stack aliasing, which the headers exclude.

| Pointer source | pairs |
| --- | ---: |
| can reach the window under a corrupt pointer (caller argument / global / callee-returned) | **27** |
| of those, the caller's header **declares** the exclusion (a precondition) | 25 |
| of those, **not declared anywhere** | **2** |
| unreachable by construction (code literal or body-built local) | 19 |

**The two undeclared, reachable pairs (the finding):** `overlay_u7f_number_0294` (0294, the numeric line editor) and
`resident_0709_printer_0768` (the text-window printer) each hand **StrStore a caller-supplied far string** (`[BP+8]` and `[SP+4]`) with no
statement that it must lie outside A0000..AFFFF. A pointer into the aperture would be read flat by StrStore while the machine reads the
planes. Both are bus-aware bodies with the EGA oracle, so the case is observable: an aperture string pointer that must DISAGREE with the oracle
(a negative control) and a header precondition are the fix; neither model is changed here (a fix ships with its case).
The other 25 reachable pairs carry the declaration ("windows exclude A0000..AFFFF because the callee reads flat RAM"), which is a
**precondition, not a proof**: 0464's blit alone turns it into a stop (`CONTRACT` when a Move meets A0000).

## What this does not yet measure

- **The classification is by reading, partly by header.** 19 of the 46 pairs were classified from the caller's call sites (`classified_by = read`),
  27 from the header's argument description (`header`); a pair misclassified as unreachable would be a defect hidden by the table, so the
  `header` ones are the next to read.
- **"Can reach" is static.** How many of the 27 actually diverge from the EGA oracle under a corrupt pointer is a fixture result, not a
  count of sites: the adversarial fixture is a caller-supplied pointer placed (a) in the aperture and (b) inside a block the body returns,
  frees or rewrites. **0239** carries both kinds (alias cases inside the GetMem block, and `hole-item-in-aperture`/`hole-character-in-aperture`
  negative controls that must disagree with the oracle); its report states the measured result. The other pairs' fixtures are pending.
- **A flat audit cannot reproduce a machine read of the aperture.** For a flat callee the only real evidence is a bus-aware CALLER audit with a
  pointer in the aperture compared against the EGA oracle, and that exists only for the 32 callers that own the oracle.

## Reading the files

`aperture_observability.csv`: `model, kind, aperture_observable, declared_exclusion`. `aperture_pairs.csv`: `caller, flat_callee,
callee_takes_pointer, pointer_source, classified_by, reach, caller_declares_exclusion`. Regenerate with `aperture_census.py --write`.
