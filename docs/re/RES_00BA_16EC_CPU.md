# RES_00BA_16EC: bounded CPU composition, no strict unlock

This candidate models `00BA:[16EC,17E3)` and composes its existing callees
**within the declared modelled domain**. On the 255-byte message below,
the bounded model stops with LIMIT where the unbounded original does not
terminate. This intentional modelling difference excludes a claim of equal
termination over all inputs. This PR records modelled composition; it
grants no identity, no strict credit and unlocks no closure. These receipts
support modelling only, not identity or credit admission.

The branch starts from fresh main `a611ec51b86dfdce65033dc553b2a42ad0f8f22a`
and is refreshed through `4b0ce9fd6bb57fc5c40691411eb750b012f39ba3`. It resumes
par-d2's parked `b17239fdefa85df13c7f9208aa5c1d1e20e0d878`. Reused evidence:
[16EC's separate identity argument](RES_00BA_16EC_CLOSURE_IDENTITY.md),
[t100's NO result](RES_03F1_02E8_IDENTITY.md), and the existing raw callee
models. The task's historical 1,193-byte one-hop price is not this body's
size or newly verified C bytes.

## Target, behavior and contract

The original body is **247 bytes / 116 instruction starts**, with seven
far calls, three near calls and its sole `RETF 0Ch` at 17E0. Body SHA-256:
`c8507a56b6044087a23b724d368ac78558bae1ce5fb42f6eace1d29985feb929`.
The checker reads and validates the extent, call sites and relocation words
from the local original image; media are not committed.

SYSTEM `0B25` copies the caller's Pascal string into `[BP-100h]`.
`DS:49F3 == 5` chooses one display branch. Otherwise `DS:6DDA` selects
colour 11h/12h; colour 11h also draws a box. The branches clear, display a
title and print the local message. Nonzero `[BP+6]` adds delay and clearing
before return. Call-site opcode/offset guards retain CODE stops, and each
far segment is read live from the relocated instruction.

The candidate uses `03F1:0137`, near 1816, `0709:0768`, `03F1:0000`,
`0709:0DBE` and near 17E3. The printer's STOP_BF5 invokes 0709:0BF5 and resumes
at 0999; the box follows its loaded stub intoOVL_040C_015D. Those callees'
limits and transfers remain observable. The printer register typedef is
renamed to avoid a collision with the other display-printer header; its
layout and function ABI remain unchanged.

Entry requires 1 MiB RAM, IP16EC, SP0800h..FFF0h, valid CPU/bus, budget and
environment. Each own-body instruction consumes one budget unit; separate
environment limits bound callees. Code is immutable. Own stack/DS accesses
and flat runtime string operands exclude A0000h..AFFFFh aliases by contract,
without a dynamic guard. EGA observability covers bus-aware display
accesses, not those excluded aliases. This is CPU-level modelling, not a
native hardware or gameplay comparison.

## The 254/255 boundary and unequal termination

The initial failing case, `m5-f0-w0-max`, is exact and synthetic:
Pascal `FFh` followed by 255 copies of `79h` (`y`), mode 5, colour-flag 0,
wait 0, row 3 and title `Ti`. **The original ran past the six-million-instruction
watchdog; the C model had not run on that case.** The initial failure log
is retained under `resources/t24-16ec/initial-runaway.log`. A second redundant
diagnostic was interrupted and retained separately; it is not a completed
run or a passing comparison.

The existing [printer evidence](RES_0709_PRINTER_0768_CPU.md) documents the
06A7 byte-index boundary. The new original-byte checks pin CMP at 06B5,
unsigned JA at 06BA, the byte increment at 0705 and JMP070E back to06AA.
For unchanged end-byte 255, an unsigned byte index cannot exceed the end;
increment wraps modulo256. The06A7 body hash is
`c9674dcc914fe3e3bb0ed3d54e58776e351f7d3888ccfd15d5b4320d97241bc5`.
This is original nontermination, not evidence that raising a watchdog would
make the call return.

The focused pair differs only in message length. Both sides get256
permitted06A7 loop-backs: the C API returns LIMIT, and the original oracle
is instrumented to stop before the same 257th JMP070E arrival. **2/2 full
state comparisons passed**:

| Synthetic Pascal message | C result | Instrumented original result |
| --- | --- | --- |
| length 254 +254 copies 79h | RETURNED | RETURNED |
| length 255 +255 copies 79h | LIMIT at 0709:070E | external LIMIT at 0709:070E |

The model's LIMIT versus the unbounded original's nontermination is an
**intentional modelling difference**. The six-million-instruction watchdog
and256-loop-back limit count different events; they are not interchangeable.
The pair establishes matching state at a finite prefix, not equal
unbounded termination. All six existing `*-max` inputs retain their255-byte
strings and use that declared finite-prefix limit in the final audit.
A passing audit must not be read as the original runaway disappearing.

Exact synthetic Pascal-message hashes:

- 254:`4c7468562e9c216692a7b7e8aec6cf66f19b087d4e8b45027c70c9e1c6de5696`
- 255:`24096c8197152e6ad03a2eb074c2a36c0dfe2422564991e12b47f5aebf6a9943`

`--reproduce-runaway` separately reruns the original-only watchdog
experiment and reports RUNAWAY,`c_model_run:false`,`parity_claim:false`.
It cannot supply a C parity result. The standalone reproduction confirmed RUNAWAY after 6,000,001 hook
arrivals, at loaded CS:IP1709:0437; no C model ran in that experiment.
Both the initial failure and this separate JSON receipt remain preserved.

## Validation receipts

Comparisons cover CPU14, all 1,048,576 RAM bytes, EGA state, ordered EGA
OUT events, VRAM write operands, environment events, stop and remaining
budget. Callee shift/MUL/DIV flags use explicitly counted pinned DOSBox-X
splits; no blanket FLAGS mask is used. Independent rules check the local
string, branch colour and RETF0Ch frame. Case worlds have name-based seeds.

The full **388/388 comparisons passed**, with zero failures: 247 RETURNED,
140 LIMIT and one OVERLAY; 116/116 starts, 6/6 branch outcomes, 243 independent
statement checks and zero shift/MUL/DIV flag splits observed. No earlier
case outcome changed during the final run. The cases cover both display
branches, colour/wait options, message lengths, graphics modes, flags,
stack edges, frame aliases, own-instruction budgets and seeded worlds.
The separate boundary pair has2 comparisons, one RETURNED/one LIMIT,
zero failures and zero flag splits. No earlier cases were removed.

Other completed checks, all heavy work through `poolrad-heavy.sh`:

- Six compiled mutations caught after an8/8 clean baseline:116/116 starts,
  6/6 branch outcomes, four RETURNED/four LIMIT, four independent statement
  checks and zero flag splits. Mode-selector, colour, copy-cap, wait-branch,
  return-cleanup and budget mutations fail on state/budget differences;
  no compiler failure counts as a kill.
- Baseline saved before mutants at `2026-10-25 06:05:31 PDT` (`date -r`),
  SHA `598988648aa47d208734c581f31d69feee7e5b13c60bf0090cc9a214a7f68382`.
- CMake build and two C tests:24 new boundary checks plus27 existing printer
  checks passed. Synthetic C fixtures need no original media.
- Fifteen flag/aperture static tests passed; full overlay suite 329 tests,
  two skips, passed. New flag rows classify the used rules against pinned
  DOSBox-X source `5a9c7c53481c2e933948039c33256776653f4faf`.
- `--native` refuses with exit2: **no native comparison run**. The parked
  checker had silently accepted `--native` and `--mutate` without running
  either; native now refuses and mutations run a required baseline first.
- Canonical accounting regenerated twice with identical pin
  `ce7a5462da1f8f8b85c187c1d476a3b8fc8c4ddd0a6877ab5b5c638520ca3049`.
  Strict 290/evidence-kind290, verified identities 335, REPLACED 125 and
  runtime-membership 3 remain unchanged, as do all six accounting files.

## Exact strict blocker and review record

`RES_03F1_0000` remains represented by `RES_IMAGE_03F10`: classification
unresolved,`strict_full_c=0`,`game_or_library=unknown`. t100 found no passing
argument-borne identity for its near caller02E8: all 27 overlay and seven
resident direct sites use literal text; game cells gate the branch rather
than supply the text object. Thus03F1_0000 is still ineligible for the
ready-helper class. 16EC's separate identity argument does not make its
unready dependency disappear. No 16EC membership or downstream unlock is
claimed; no strict, identity, runtime, REPLACED or checkpoint row is edited.

- target_function:`RES_00BA_16EC`, complete247-byte body.
- source_candidates:`resident_00ba_window_16ec_cpu.c`, resumed parked model.
- signals: exact extent/return, seven far and three near calls, mode/colour
  constants, stack string object, original-byte state comparisons and
  negative boundary evidence.
- score: scoped modelling evidence; no identity or strict score.
- verdict: bounded CPU composition supported within the declared domain; unbounded
  termination differs intentionally; strict closure BLOCKED.
- review_notes: no native oracle; aperture exclusions; explicit callee
  flag splits; original255-byte nontermination and artificial LIMIT;
  ready-helper membership unresolved. Names alone establish no parity.

Reproduce with the repository's Capstone/Unicorn Python environment:

```sh
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res00ba_window_16ec_cpu_audit.py --unicorn --keep-going --output resources/t24-16ec/complete.json
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res00ba_window_16ec_cpu_audit.py --unicorn --only boundary-254 boundary-255 --output resources/t24-16ec/boundary.json
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res00ba_window_16ec_cpu_audit.py --reproduce-runaway --output resources/t24-16ec/original-watchdog.json
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res00ba_window_16ec_cpu_audit.py --mutate --output resources/t24-16ec/mutations.json
/home/hunter/workspace/poolrad-heavy.sh ctest --test-dir resources/t24-16ec/build -R '^poolrad_resident_(00ba_window_16ec|0709_printer_0768)_cpu_tests$' --output-on-failure
```

All media, derived artifacts and full reports stay under ignored
`resources/`; the resources symlink is not tracked.
