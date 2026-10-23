# Folded flags: u31 +0255 and +0675 (task t44, slice 2)

This doc covers the twelve rows of [FLAG_FOLDED_SITES.tsv](FLAG_FOLDED_SITES.tsv) for `OVL_0031_0255`
(`overlay_u31_candidate_check_0255_cpu.c`) and `OVL_0031_0675` (`overlay_u31_step_gate_0675.c`). Each was checked against
the ORIGINAL bytes: GAME.OVR unit `31h`, and START.EXE `00BA:23B7` for the resident routine that `+0255` composes.

## Method

`tools/overlay/flag_fold_liveness.py --preset u31` (#1451) regenerates the table from the bytes. For every flag a folded instruction writes,
it walks every successor path of the routine until one of three things happens:

- **Overwritten.** A later instruction writes the flag. The pinned DOSBox-X rules from
  [FLAG_SEMANTICS_CENSUS.md](FLAG_SEMANTICS_CENSUS.md) apply:
  - `XOR`/`OR` write all six flags with AF = 0;
  - a shift with a non-zero count writes all six with AF = 1;
  - `MUL` writes CF/OF/ZF/PF and keeps SF/AF;
  - `IDIV` writes all six;
  - `INC` keeps CF.
- **Observed.** An instruction reads it: Jcc/SETcc/LOOPcc, ADC/SBB/RCL/RCR/CMC, PUSHF/LAHF, INT, or DAA-family.
- **Exposed.** It reaches a far call or a return.
  - A far call is both the callee's entry and a point where these models can stop: CODE before the call, or the
    callee's own stop.
  - A return hands FLAGS to the caller.

A Jcc read is listed separately, because a model that takes that branch from the same condition consumes it without
exposing a FLAGS image. Several TSV rows name a line that folds more than one instruction; each instruction is walked.

## Per-site table

| Body | TSV row | Site | Instruction(s) | Next writer | Observer or boundary | Verdict |
| --- | --- | --- | --- | --- | --- | --- |
| +0255 | `:196` | `00BA:23BA` | `SUB SP,2` | `CMP` at `23C0` | none | dead |
| +0255 | `:230` | `0258` | `SUB SP,4` | `XOR AH,AH` at `026A` | none | dead |
| +0255 | `:239` | `026A`, `0270` | `XOR AH,AH`; `SHL DI,CL` (CL = 4 at `026E`) | `SHL` at `0270`; `XOR` at `0276` | none | dead |
| +0255 | `:266` | `02B3`, `02B9`, `02BB` | `XOR`; `SHL DI,1`; `ADD DI,SI` | `SHL` at `02B9`; `ADD` at `02BB`; `XOR` at `02C1` | none | dead |
| +0255 | `:269` | `02C1`, `02C5`, `02C7` | `XOR`; `SHL DI,1` ×2 | `SHL`; `SHL`; `CMP` at `02D2` | none | dead |
| +0255 | `:273` | `02D2` | `CMP AL,DL` | `CMP` at `02DD` | `JE` at `02D4` reads ZF, modelled as the equality branch | dead (ZF consumed by the branch) |
| +0675 | `:82` | `0689`, `0699`, `06B1`, `06B8`, `06FB`, `072A`, `0736`, `073D`, `07B2` | `XOR AH,AH` | `XOR`/`SHL`/`ADD`; `MUL` then `CMP` at `0744`; `INC` then `CMP` at `07C4` | none | dead |
| +0675 | `:185` | `0678` | `SUB SP,0Ah` | `XOR AH,AH` at `0689` | none | dead |
| +0675 | `:193` | `069F`, `06A1`, `06A3`, `06A5` | `SHL DI,1` ×2; `ADD DI,SI`; `ADD DI,DX` | `SHL`; `ADD`; `ADD`; `XOR` at `06B1` | none | dead |
| +0675 | `:211` | `06C0` | `IDIV CX` | none before the call | `LCALL` at `06DA` | **exposed: already computed** (pinned DIVW/IDIVW); only the stale comment changes |
| +0675 | `:302` | `075D`, `0774`, `078B` | `OR AL,AL` | `CMP` at `07C4` when `JNE` is taken | fall-through reaches the `LCALL` at `076F`/`0786`/`079D` with all six flags live | **exposed; the folded value was equal by construction; now computed** (see below) |
| +0675 | `:302` | `07A2` | `OR AL,AL` | `XOR` at `07B2`, `CMP` at `07C4` | `JNE` reads ZF (the branch) | dead |
| +0675 | `:308` | `07B4` | `INC AX` | `CMP` at `07C4` | none | dead |
| +0675 | header | `071F`, `073F` | `SHR AL,1`; `MUL DX` | `XOR` at `072A`; `CMP` at `0744` | `JAE` reads CF (the branch) | dead |

The callees behind the two exposed boundaries both open with `PUSH BP; MOV BP,SP; SUB SP,n`, and that `SUB` overwrites
all six flags before any read. Those callees are U3C6 `+0CB9` (through stub `03E0:0070`) and `00BA:2199`. The exposure
is therefore through the stop images, not through a read in the callee: CODE before the call, or the callee model's
own stop at its entry.

## Why the old `OR AL,AL` fold was not observably wrong

`OR AL,AL` falls through to the next lookup only when AL = 0. Its flags there are therefore always ZF = PF = 1 and
SF = CF = OF = AF = 0. The old model carried the previous `00BA:2199` call's returned flags instead.

`2199` (`2199..2207`) leaves only through `21F9`, which reaches its `RETF 0Ah` with no flag writer in between. It arrives
there in one of two ways:

- **Hit:** `JNE` after `CMP BYTE [BP-2],0` with `[BP-2]` = 1, so AL = 1 and the `JNE` at the caller is taken.
- **Miss:** `JE` after `OR AX,ES:[DI+2]` gives zero (the end of the list), so AL = 0 and the flags are exactly the
  six values above.

So whenever the previous lookup returned, the carried flags already equalled `OR`'s. The new negative control confirms
this: the old model, with only the IP fix below applied, passes all 16 Unicorn CODE-stop cases. The fold is now
computed anyway, so the stop image no longer depends on that coincidence in a different callee. The defect these stops
did expose was the CODE-stop IP.

## Model changes

`overlay_u31_step_gate_0675.c`:

- **CODE-stop IP (the defect found).** A CODE stop now leaves CS:IP at the call it refused. It used to leave the entry
  IP, `0675`, so every CODE stop image was wrong in IP. The unmodified model fails the new stop cases on IP at the first
  one (`code-06da-0`: C 0675, original 06DA).
- **`OR AL,AL` computed.** After each lookup, `OR AL,AL` now writes its pinned flags: CF = OF = AF = 0; SF, ZF and PF from
  AL. The values are unchanged, as shown above, but they no longer rely on `2199`'s return path.
- **Comments and header.** The stale "IDIV ... folded" comment and the folded-flags paragraph in the header are
  corrected. Each remaining fold names the instruction that overwrites it.

`+0255` needs no change.

## Evidence

- **Focused test.** `test_overlay_u31_step_gate_0675.c` gains the hand-predicted check described below and passes **62**
  checks.
  - The check: the `0758` lookup misses, then `076F` is non-original. The model must stop with CODE at `076F`, with
    AL = 1Eh and OR's flags (ZF = PF = 1, the other four 0) over entry flags that had CF, AF and SF set.
  - The new test fails against the old model.
- **Audit.** `u31_step_gate_0675_audit.py --unicorn` gains 20 CODE-stop cases: the five far-call sites under four
  entry-flag seeds. The original stops on reaching the site, and all 14 CPU words and the full 1 MiB must be equal,
  unmasked. It also gains CODE-stop cases. All runs went through `poolrad-heavy.sh`, one job at a time, with load below 12.

- **Unicorn:** 16 CODE-stop cases at the four lookup sites, under four entry-flag seeds. The original stops on reaching
  the site, and all 14 CPU words and the full 1 MiB must be equal. The 407 returning cases still pass: 137/137 starts,
  20/20 outcomes.
- **`06DA` is DOSBox-X only.** Its stop flags are `IDIV`'s, written to the pinned DIVW/IDIVW values, and Unicorn's
  `IDIV` flags differ (FLAGS 7202 vs 7206 on `code-06da-0`). The census already notes that only a DOSBox re-run settles
  DIV/IDIV.
- **DOSBox-X:** `u31_step_gate_0675_native.py` adds CODE-stop cases. Each patches a `JMP FAR` over the chosen call site
  (the `LCALL`'s five bytes, no flag or stack effect) to a capture label, and restores the site after the case. **65**
  cases pass (50 earlier ones plus 15 stops at all five far-call sites), comparing all 14 CPU words, 256 stack bytes and
  every data region, with no mask. The result SHA-256 is
  `a5a46c4872bd17fdc53e28b796846a96455ee33c56bf6f207f69bfa2d0a72db3`. This confirms the pinned `IDIV` flags at the
  `06DA` stop.
- **Negative controls:**
  - The unmodified model fails the new stop cases on IP.
  - The model with only the IP fix passes them, which is the equal-by-construction result above.

## Credited bodies

- **`OVL_0031_0255`:** the evidence did not depend on any folded flags. All six sites are dead in the original bytes,
  and no model change was made.
- **`OVL_0031_0675`:** the evidence did not depend on any folded flags. The credited differential compares only
  returning cases, and on every returning path a `CMP` (`06DF`, `06EF`, `070F` or `07C4`) overwrites every folded flag
  before the `RETF`. The CODE-stop IP defect sits outside that evidence too: no credited case stopped with CODE. The
  re-run reproduces all 407 returning cases unchanged.

`FLAG_FOLDED_SITES.tsv` is left as listed: its line numbers for `overlay_u31_step_gate_0675.c` predate this change.
