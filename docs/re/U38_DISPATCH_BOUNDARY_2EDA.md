# U38 +2EDA: eight incoming slots, unresolved dynamic closure

**Verdict at main ba0cd949: a dynamic dispatch boundary, not currently a
strict composable target.** The reviewer is correct that one U38 handler
serves eight installed slots. The earlier readiness warning is also correct:
this handler calls partial removal and the unrestricted U9E dispatcher.
Recursion can be implemented, but recursion support alone cannot close the
other possible targets. This report resolves the inventory/readiness
distinction; it does not add a handler model or assign any credit.

## Reproducible identity and calls

Owner START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Owner GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

U38[2EDA,2F4B), file[72501,72614), is113 bytes/47 instructions.
The schema-8 queue reports125 bytes/51 decoded instructions using the next
function's upper bound. This audit ends after RETF10 at+2F48 and excludes the
12 following bytes[2F4B,2F57). Its readiness flag is already false: +0026 is
partial and +0000 untranslated. The queue extent is not a function-end proof.

Body SHA-256:
e75c9f7ebfcd4235a1a5a11b6d77ae544c0bd64a3c3f8ade8c5fac454757ecd3.
It is public entry111, stub0041:024B, installed at types
128,129,130,133,134,136,138,139. These are incoming selector values,
not a restriction on the record byte used for a subsequent dispatch.

The audit checks each segment operand's loader fixup, resolves each public
stub through the overlay entries, and obtains:

| Caller site | Original stub | Actual body |
| --- | --- | --- |
| U38:2F06 |00B0:002A|U9E:0026 removal|
| U38:2F24 |00B0:0052|U9E:0E52 append|
| U38:2F40 |00B0:0025|U9E:0000 dispatcher|

For nonzero mode, +2EDA passes a nil item and the record's byte+3D to
+0026. The existing raw removal API is explicitly partial: a matching node
with nonzero byte+4 stops at U9E:001E before dispatch. Its complete unlink/
FreeMem path cannot establish closure for the nonzero-byte+4 path. See
[U9E_LIST_RELEASE_0026_CPU.md](U9E_LIST_RELEASE_0026_CPU.md).

For zero mode, +2EDA calls +0E52, reloads the local record pointer and
record byte+3D, then calls +0000. The reloaded byte is the dispatch type;
there is no range test or restriction to the eight incoming values.
The mode passed to the selected handler is zero, and its item pointer is nil.

U9E[0000,0026) has38 bytes/16 instructions, SHA-256:
1572342d9be429f7a7b7f6b015b2a609726b4d5580e34235fda31eb2a43b9217.
At+001E it calls the live pointer at DS:6828+4*unsigned_type.
The type is zero-extended and multiplied by4; all256 byte values are possible
at the instruction level. No route-specific data invariant has been supplied.

## Original continuation probe

The audit runs257 original-code continuation probes: all256 byte values and
one changed live target. Each begins at U38:2F29 with an explicitly synthetic
post-append state, follows the original call into U9E+0000, and stops before
the indirect call at+001E. The one reached stub binding is resolved to the
original dispatcher body for this local fixture.

All cases reach the boundary with the expected DI=4*type, exact prepared
five-word handler argument frame, and actual installed target pointer.
The union covers25 instruction starts before the boundary. Changing type54's
live pointer after initialization changes the observed target to the supplied
synthetic CAFE:BEEF pointer; initialization is not a permanent binding proof.

| Record byte+3D | Selected target under the checked installers |
| --- | --- |
|0|not initialized by these installers|
|4|U38+017F, outside this eight-slot handler|
|54|U38+1355, an existing raw no-op handler|
|128 or139|U38+2EDA itself|
|140 or214|not initialized by these installers|
|215|U86+0A91, adjacent procedure-table alias|
|255|U86+234C, adjacent procedure-table alias|

Uninitialized here means only that the two checked installers do not
initialize it. It does not imply zero in a real game: the zero RAM in those
synthetic slots is a fixture choice. Slot214 overlaps DS:6B80 used elsewhere.
See the [full table inventory](u9e_dispatch_handlers.json) for the distinctions
between primary slots1..139, uninitialized0/140..214 and adjacent215..255.

The probe does **not** execute +0E52, +0026, or any selected handler. It does
not prove the synthetic post-append states occur in natural gameplay, and it
does not claim full-function CPU/RAM parity, native DOS validation, or strict
completion. The existing append may change the record before the reload;
the probe deliberately does not assume otherwise.

Receipt SHA-256:
2df300a343d5954eb0f5de067fb2a0a0f9589961aac68bf69bfe51c1b07d2811.

    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u38_dispatch_boundary_2eda_audit.py --unicorn --output resources/u38-2eda-boundary.json

All original bytes, decoded bodies and synthetic execution state remain
under ignored resources/. The public report contains addresses, hashes,
source-derived behavior and probe counts.

## What would close the chain

A future raw +2EDA implementation can preserve state up to both unresolved
dispatch paths, expose a precise pending boundary, and compose the existing
append/removal APIs. That would be useful partial work, but not a full strict
chain. A callback returning success or a blanket skip would lose behavior.

A strict claim requires complete reachable handlers under an evidence-backed
input contract, live pointer/stub validation, the exact indirect call/return
ABI, and recursive execution with truthful resource-limit stops. If a reached
game route proves a narrower type range, only that proven closure needs to be
implemented. Without such a precondition, the missing/callback handlers,
uninitialized slots and adjacent procedure aliases remain dependencies.
A recursion depth limit addresses nontermination; it does not implement
unresolved target behavior.

Next authoring should drain the assigned single-slot handlers in public-stub
order, coordinating existing owners, while the dispatcher closure remains
explicit. U38+0679/+0947 are assigned to the Claude lane. This report neither
marks any issue checkbox complete nor edits canonical strict accounting.

## Recall and review record

Recall query “poolrad U38 2EDA dispatch recursive” returned Claude
session281c8700-5ac and the prior handler inventory. Reused findings are
[U9E_DISPATCH_HANDLER_BACKLOG.md](U9E_DISPATCH_HANDLER_BACKLOG.md), the
hash-checked installer audit, and the partial removal contract. This audit
independently checks +2EDA's own extent, public entry, all three fixups/callees,
mode branch, type provenance, and the original dispatch continuation.

- target_function: OVL_0038_2EDA and its U9E dispatch dependency.
- source_candidates: no new C model; existing raw +0026 and +0E52 interfaces.
- signals: exact hashes/extent, resolved stub/fixups, eight installer bindings,
  byte provenance and257 original continuation probes.
- score: strong boundary evidence; insufficient full-function parity evidence.
- verdict: dynamic boundary; not currently strict-composable.
- review_notes: eight incoming aliases and outgoing closure are different
  properties. Recursive support is feasible but does not discharge dynamic
  targets. Independent review decides acceptance; no self-credit.
