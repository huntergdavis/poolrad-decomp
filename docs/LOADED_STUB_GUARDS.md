# Model-side loaded overlay guards

Role: AUTHOR, implementation and evidence only. PM assignment i21442.
No credit or identity change is requested. The generated evidence-kind table
labels six existing entries as registered checkers; their credited membership
and the credit count remain unchanged.

A live hybrid arrival at U6A +0214 exposed the gap: its +019C model skipped
the overlay manager when U86 was absent. The model-side guards now read the
live LCALL's segment word and require the referenced stub to contain EA and
the expected entry. Explicit callee segment arguments must match the jump's
segment. U6A uses the live jump segment when entering the predicate.

The guards run before CPU14 or RAM mutation, including required inherited
bindings. Cold or malformed required bindings return ARGUMENT. This is a
host refusal, not an emulation of the overlay loader. Existing immutable
code/binding and synchronous-access contracts still apply. Existing initial
argument/code-alias checks retain their priority. U31's own malformed call
or stub retains its original CODE stop; a valid own binding preflights its
composed U3C2 dependencies before mutation.

| Entry | Checked calls or inherited dependency |
| --- | --- |
| OVL_001C_0B52 | Six calls through U25 entry 22, stub +008E -> +1EA6 |
| OVL_006A_019C | +01C3 through U86 entry 6, +003E -> +323D |
| OVL_006A_0000 | +008A through the same U86 stub |
| OVL_005E_263D | Five calls through U3C6 +005C/+0061 and U3C2 +0034 |
| OVL_005E_1BF8 | Three calls through U3C6 +005C/+0061/+003E |
| OVL_007F_1010 | Thirteen calls through U9E +0048 -> +0DE3 |
| OVL_0038_04BA | Three calls through U9E +0075/+0070 |
| OVL_03C2_0909 | Two calls through U3C6 +0025 -> +0000 |
| OVL_005E_32E6 | Four direct calls plus the U3C2 +0909 dependency |
| OVL_006A_0214 | Inherited +019C binding |
| OVL_005E_17F6 and OVL_005E_2574 | Inherited +263D bindings |
| OVL_0031_0255 | Inherited +0909 bindings |
| Uncredited U9E +0160 and +02FA | Inherited +0909 bindings only |

The nine direct callers cover the 17 rows / 38 sites identified in PR #1426.
U9E's existing synthetic direct-call rebinding profile remains unchanged;
this change protects its shared U3C2 dependency and makes no loader claim
for its own rebound calls.

## Closure of the live missing-overlay defect class

> the class that produced 006A_0214's first differing byte at PSP:002E is now guarded, and the guard refuses rather than passes.

The [stub-precondition census](HYBRID_STUB_PRECONDITION_CENSUS.md#result-main-435c6f65)
found 42 crossing models at main 435c6f65, with 10 missing-runtime-guard defects:
six declared the loaded-unit precondition without checking it, and four did
neither. These guards fix that class within the direct and inherited binding
scope above: absent or malformed required stubs cause refusal before CPU/RAM
mutation. The owner's count does not move because this is a correctness guard,
not a new body. The original overlay loader, naturally reached route parity,
and the U9E sidecars' own synthetic rebound calls remain outside this claim.

## U5E +263D: all 22 atomic refusal cases

On code head 51f7edea (including main 6fac2396), the repeated +263D audit passed
307 original-chain CPU14/full-1-MiB comparisons, with the existing declared AF
split in one case; 506 standalone predicate cases; and the following 22 atomic
host refusals. Caller instruction coverage was 41/41; standalone predicate
coverage was 410/410. The AF-split case ends after a non-zero shift: pinned C
AF=1 and Unicorn AF=0; all other compared flag bits match.

These are the exact labels constructed by
[verify_refusals](../tools/overlay/loaded_stub_checks.py), using the default
[+263D fixture and install functions](../tools/overlay/u5e_direction_263d_audit.py).
Each row starts with a fresh valid fixture and applies only the named mutation.
The fixture has caller CS=3000, U3C6 slots CS=4000, and U3C2 predicate CS=5000.
Stub labels contain the five-digit physical address, not a segment.
Every row returned ARGUMENT (1), preserved all 14 CPU words, and preserved all
1 MiB of RAM relative to that row's deliberately altered input. These refusal
checks are host tests, not original-DOS executions or new native evidence.

| Exact case name | Binding and refused precondition |
| --- | --- |
| `call-3000:2649-opcode` | Call to U3C6 stub 03E0:005C -> +0BE7: opcode 9A changed to 90; required LCALL absent. |
| `call-3000:2649-offset` | Call to U3C6 stub 03E0:005C -> +0BE7: slot offset 005C changed to 005D; required stub slot mismatched. |
| `call-3000:2657-opcode` | Call to U3C6 stub 03E0:0061 -> +0C0F: opcode 9A changed to 90; required LCALL absent. |
| `call-3000:2657-offset` | Call to U3C6 stub 03E0:0061 -> +0C0F: slot offset 0061 changed to 0060; required stub slot mismatched. |
| `call-3000:2665-opcode` | Call to U3C6 stub 03E0:005C -> +0BE7: opcode 9A changed to 90; required LCALL absent. |
| `call-3000:2665-offset` | Call to U3C6 stub 03E0:005C -> +0BE7: slot offset 005C changed to 005D; required stub slot mismatched. |
| `call-3000:2673-opcode` | Call to U3C6 stub 03E0:0061 -> +0C0F: opcode 9A changed to 90; required LCALL absent. |
| `call-3000:2673-offset` | Call to U3C6 stub 03E0:0061 -> +0C0F: slot offset 0061 changed to 0060; required stub slot mismatched. |
| `call-3000:2693-opcode` | Call to U3C2 stub 03DB:0034 -> +057B: opcode 9A changed to 90; required LCALL absent. |
| `call-3000:2693-offset` | Call to U3C2 stub 03DB:0034 -> +057B: slot offset 0034 changed to 0035; required stub slot mismatched. |
| `stub-03DE4-unit-absent` | U3C2 stub 03DB:0034: first bytes changed to CD 3F (INT 3Fh); unit absent/cold trap, not a loaded EA jump. |
| `stub-03DE4-opcode` | U3C2 stub 03DB:0034: EA changed to 90; required loaded far JMP absent. |
| `stub-03DE4-entry` | U3C2 stub 03DB:0034: jump entry 057B changed to 057A; required callee entry mismatched. |
| `stub-03DE4-segment` | U3C2 stub 03DB:0034: jump segment 5000 changed to 5001; target differs from the explicit layout segment. |
| `stub-03E5C-unit-absent` | U3C6 stub 03E0:005C: first bytes changed to CD 3F (INT 3Fh); unit absent/cold trap, not a loaded EA jump. |
| `stub-03E5C-opcode` | U3C6 stub 03E0:005C: EA changed to 90; required loaded far JMP absent. |
| `stub-03E5C-entry` | U3C6 stub 03E0:005C: jump entry 0BE7 changed to 0BE6; required callee entry mismatched. |
| `stub-03E5C-segment` | U3C6 stub 03E0:005C: jump segment 4000 changed to 4001; target differs from the explicit layout segment. |
| `stub-03E61-unit-absent` | U3C6 stub 03E0:0061: first bytes changed to CD 3F (INT 3Fh); unit absent/cold trap, not a loaded EA jump. |
| `stub-03E61-opcode` | U3C6 stub 03E0:0061: EA changed to 90; required loaded far JMP absent. |
| `stub-03E61-entry` | U3C6 stub 03E0:0061: jump entry 0C0F changed to 0C0E; required callee entry mismatched. |
| `stub-03E61-segment` | U3C6 stub 03E0:0061: jump segment 4000 changed to 4001; target differs from the explicit layout segment. |

## Evidence and reuse

The source mappings come from each entry's Loaded-state precondition section
(PR #1426). The class was identified by PR #1428 at a4f635d9; its census is
lexical, so shared helper use must also be inspected. The existing
U7F +0FD2 model supplies the live LCALL -> loaded EA -> target pattern.
Dunking Sheep recall was run before implementation; retrieved historical
loaded-stub discussions were leads, not new measurement evidence. No Deja
recall was used.

The original-byte audits retain actual far-JMP stubs instead of rebinding
the affected caller directly to a body. Stub instructions execute in the
oracle but add no body-coverage credit. No CPU, FLAGS, or RAM masks are added.
Separate host checks change cold-stub bytes, opcode, entry offset, segment
(where the API specifies it), and each call site's opcode/slot offset. They
require ARGUMENT and unchanged CPU14/full 1 MiB RAM; these are not DOS cases.

The 13 credited entries passed their full original-byte audits on f0826b32:

| Entry | Full CPU14 / 1 MiB RAM cases | Atomic host refusals | Absent-unit fixtures |
| --- | ---: | ---: | ---: |
| U1C +0B52 | 919 | 16 | 1 |
| U6A +019C | 507 | 5 | 1 |
| U6A +0000 | 507 | 5 | 1 |
| U5E +263D | 307 | 22 | 3 |
| U5E +1BF8 | 587 | 18 | 3 |
| U7F +1010 | 2,338 + 8 aliases | 30 | 1 |
| U38 +04BA | 3,019 | 14 | 2 |
| U3C2 +0909 | 439 | 8 | 1 |
| U5E +32E6 | 440 | 32 | 5 |
| U6A +0214 | 125 | 5 | 1 |
| U5E +17F6 | 2,794 | 22 | 3 |
| U5E +2574 | 2,301 | 22 | 3 |
| U31 +0255 | 174 | 8 | 1 |

These are 14,465 full-state cases with no FLAGS masks, plus 207 host refusal
checks, including 26 absent-unit fixtures. Repeated host checks in the U7F
alias invocation are counted once. The +263D checker also passed 506
standalone predicate CPU/RAM cases. All 16 focused C executables passed.
U38 adds a successful A20-wrapped stub-address fixture. U7F adds an
A20-wrapped callee with a 16-bit wrap in the relocated stub segment.
Existing deeper-chain coverage gaps remain in the receipts; no coverage
credit is inferred from executing synthetic stubs. U6A +0000 and +019C each
retain one nonreturning original fixture outside the C comparison; +0214
also checks two nonreturning originals against its existing C cap.

An initial sidecar host check stopped
before C invocation because the shared fixture supplied a distinct ctypes
CPU class; the host checks now use the same CPU conversion as the existing
full comparison. No recovered C change was needed for that checker failure.

Both uncredited sidecar audits subsequently passed: +0160 has 609 cases,
140/140 caller starts and 22/22 caller outcomes; +02FA has 593 cases,
911/911 caller starts and 40/40 caller outcomes. Their existing Unicorn
profiles retain AF exceptions: +0160 has 294 pending and two binding
exceptions; +02FA has 161 pending and two binding exceptions. These are
not unmasked FLAGS parity claims. Each sidecar adds eight atomic host
refusals, including one absent-unit fixture, bringing the total to 223
unique host checks and 28 absent-unit fixtures. Their own rebound calls
remain outside the shared-dependency guard scope.

After rebasing onto main 574092a9, the affected audits are being rerun because
PR #1433 changed arithmetic flag semantics in shared dependencies. U6A's
507 + 507 full cases, U1C's 919 full cases, U3C2's 439 full cases, and the
affected focused C tests passed again. U3C2 retains zero FLAGS masks, eight
atomic host refusals, 297/297 caller sites and 34/34 caller branch outcomes;
its deeper-chain coverage remains 1,281/1,283 sites and 185/192 outcomes.
U38's unchanged source closure retains its earlier result.
U5E +32E6 also passed all 440 full-state cases, with 32 atomic refusals and
five absent-unit fixtures. U7F +1010 passed 2,338 full-state cases and eight
alias cases, with 30 unique atomic refusals and one absent-unit fixture.
Both retain zero FLAGS masks and their earlier coverage limits.
U31 +0255 passed all 174 cases and eight atomic refusals again, with
58/58 caller starts and 6/6 conditional outcomes.
The U5E +263D `predicate-budget` case stopped with C FLAGS 7212 versus
Unicorn 7202: main's `times_four` now sets AF under the pinned DOSBox-X
profile. The failed receipt is retained. PM instruction i21568 assigns the
declared profile split to par-d2; this branch adds no mask and will rerun the
checker after that correction reaches main. Other audits continue separately.

The continuation also retained these current-main failures; passing host
refusals do not turn a failed original-byte audit into a pass:

| Checker | First failing fixture | C FLAGS | Unicorn FLAGS |
| --- | --- | --- | --- |
| U5E +17F6 | `direction-0-heading-5` | 7203 | 7202 |
| U5E +2574 | `predicate-limit`, budget 6 | 7212 | 7202 |
| U5E +1BF8 | `code-alias-(24576, 7160)` | 0212 | 0202 |

The +17F6 dependency now includes pinned IDIV flag behavior; +2574 inherits
the +263D AF split. PM received these first two additional failures in
message i21582 and the +1BF8 failure in i21585. Their profile corrections
are outside this guard change.

All five changed native harnesses passed with unmasked FLAGS under DOSBox-X
normal core / 386, binary SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`:

| Native harness | Cases | Compared RAM in addition to CPU14 |
| --- | ---: | --- |
| U3C2 +0909 | 24 | 65,536 data bytes and 864 stack bytes |
| U5E +32E6 | 27 | 65,536 data bytes and 1,888 stack bytes |
| U7F +1010 | 37 | 65,536 data bytes and 864 stack bytes |
| U9E +0160 | 113 | 65,536 DS bytes, 65,536 node bytes, 1,024 stack bytes |
| U9E +02FA | 77 | 65,536 DS bytes, 65,536 node bytes, 1,024 stack bytes |

These 278 native cases do not compare all RAM or establish a naturally
reached game route. Sidecar captures use their existing far-JMP boundary
traps; they are not full executions beyond those pending/binding stops.
The +0160 result SHA-256 is
`ae56a0ca8b57e07921f65e169aaf7fa24aa458fd54456c633df75239e1c77921`;
the +02FA result is
`d75264b1686c8b44f6a74f57416ad35586fdca9cdaa7a0a14d433bc64a89dbc6`.
Per-case capture hashes for the other three harnesses remain in their
ignored native reports. After rebasing onto main 9d55a9b0, the gate passed
12 actual compile/load/bind builds for all 13 credited rows on ef83017e.
The checker-name gate and all six evidence-kind table tests passed too;
PR #1445's CI is green on that head. Acceptance remains pending the
upstream profile corrections and affected full-checker reruns.

All compilation and audit runs use poolrad-heavy.sh, one at a time, with
load1 at most 12 before starting work. Receipts and logs remain under ignored
resources/stub-guards. The registered checker handoff will use actual
compile/load/bind operations against current main.

## Adversarial parity assessment

- target_function: the nine listed direct callers, four inherited composers,
  and two uncredited sidecars' shared dependency path.
- source_candidates: their existing recovered C entries with read-only
  loaded-stub preflights; no arithmetic body replacement.
- signals: original descriptor/call-site mapping, live call and EA operands,
  existing loaded-state oracles, explicit absent-unit refusal and atomicity.
- score: not assigned while four current-main original-byte audits fail.
- verdict: current-main handoff remains blocked on the reported flag-profile
  differences; atomic guard checks, the other original-byte audits, native
  comparisons within their stated regions, and checker builds have passed.
- review_notes: entry rejection can conservatively refuse a branch that would
  not actually call the absent unit. This matches the existing all-listed-units
  loaded contract. A valid EA does not prove arbitrary body bytes match the
  recovered function; loaded body identity and immutable code remain required.
  No overlay-loader recovery, naturally reached route, or new credit is claimed.
