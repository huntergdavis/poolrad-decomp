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

Validation is pending. All compilation and audit runs use poolrad-heavy.sh,
one at a time, with load1 at most 12 before starting work. Receipts and logs
remain under ignored resources/stub-guards. The registered checker handoff
will use actual compile/load/bind operations against current main.
