# Model-side loaded overlay guards

Role: AUTHOR, implementation and evidence only. PM assignment i21442.
No credit, identity, ledger, manifest, or accounting change is requested.

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
U38 and U7F include added successful A20-wrapped stub-segment fixtures.
Existing deeper-chain coverage gaps remain in the receipts; no coverage
credit is inferred from executing synthetic stubs. U6A +0000 and +019C each
retain one nonreturning original fixture outside the C comparison; +0214
also checks two nonreturning originals against its existing C cap.

The two uncredited sidecar audits, changed native harnesses, and final
registered-checker handoff are pending. An initial sidecar host check stopped
before C invocation because the shared fixture supplied a distinct ctypes
CPU class; the host checks now use the same CPU conversion as the existing
full comparison. No recovered C change was needed for that checker failure.

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
- score: pending execution of the updated checkers.
- verdict: unclear until the updated validation packet completes.
- review_notes: entry rejection can conservatively refuse a branch that would
  not actually call the absent unit. This matches the existing all-listed-units
  loaded contract. A valid EA does not prove arbitrary body bytes match the
  recovered function; loaded body identity and immutable code remain required.
  No overlay-loader recovery, naturally reached route, or new credit is claimed.
