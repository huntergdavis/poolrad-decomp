# U9E +0026 raw removal and explicit dispatch boundary

This is a **partial chain**, with no new strict credit. It implements U9E+0026
list search/removal and composes merged raw SYSTEM FreeMem. A node whose
byte+4 is nonzero enters the exact U9E+0000 prefix, then stops before the
indirect LCALL at+001E. The handler is neither replaced by a callback nor
silently skipped. U7D+0000 (#1015) remains partial pending handler composition
and fresh timer-chain evidence.

## Identity and reused evidence

| Artifact | Extent / identity | SHA-256 |
| --- | --- | --- |
| START.EXE |65,231 bytes|d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd|
| GAME.OVR |206,105 bytes|872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614|
| U9E+0026 |file[175888,176170), unit[0026,0140),282 bytes/94 instructions|12578dfdfb77080908623cf336552b20354b56128580f377b3d26ad2caf1df12|
| U9E+0000 |unit[0000,0026),38 bytes/16 instructions|1572342d9be429f7a7b7f6b015b2a609726b4d5580e34235fda31eb2a43b9217|

Public slot2 is+0026. The queue's314-byte/109-instruction span includes32
trailing data bytes[0140,0160), excluded here. +0000 is not a leaf:
+001E calls [DS:6828+4*type]. The apparent strict U38-inline association is
U38's own+0000, not U9E's dispatcher.

Recall: deja query "poolrad U9E 0026" returned Claude session281c8700-5ac,
which led to bounded U9E+0160 and inline annotations. Source inspection and
the original decode contradicted supposed raw-leaf readiness. Reused semantic
reference [U9E_ENTRY2.md](U9E_ENTRY2.md) supplies search/unlink behavior and the
trailing-data boundary; its host callbacks do not establish raw composition.
Raw FreeMem and its original/native harness come from #1011
([SYSTEM_FREEMEM_0254_CPU.md](SYSTEM_FREEMEM_0254_CPU.md)); full-state comparison
follows #1013. The installer inventory re-decodes the hash-pinned bodies in
[U38_EFFECTS_INSTALL_31A2.md](U38_EFFECTS_INSTALL_31A2.md) and
[U86_PROCVARS_3413.md](U86_PROCVARS_3413.md).

## Raw interface and stops

pr_u9e_list_release_0026_cpu accepts exactly1MiB A20-off RAM and14 CPU words:
AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS. Enter at IP0026 after far CALL;
SS:SP holds return IP/CS, item offset/segment, type word, member offset/segment.
Success ends with RETF10. SP must be0040..FFF2. All reached return frames
must remain nonwrapping, including aliases; code/bindings are immutable.
CPU, limits and diagnostic storage are distinct from RAM and each other.
Interrupts and faults are excluded. Operands remain live in instruction order.

A nil item searches member+7F/+81 by byte key; a supplied item is used directly.
If node byte+4 is zero, the head or predecessor is unlinked before FreeMem(9).
A missing predecessor still writes through0000:0000 exactly as the original.
An invalid free can therefore stop after unlinking; there is no rollback.

For nonzero byte+4, six dispatcher words and its PUSH-CS/near-CALL frame are
constructed. The prefix pushes BP and five handler arguments, zero-extends
type, scales DI by4 and stops at CS:001E. PENDING leaves SP38 bytes below entry
and reports the live table pointer. Exactly13 dispatcher prefix instructions
execute; the indirect call, POP BP and RETF12 are not completed paths.

Other statuses distinguish invalid host arguments, changed reached call operands,
FreeMem stops, changed FreeMem far returns and list limits. The shared list
budget counts taken0076->0052 and0113->00E5 edges; LIMIT stops at their target
before another iteration. Zero is valid. FreeMem retains its own walk budget
and RunError204 semantics. All non-argument stops retain partial CPU/RAM state.

FLAGS use measured DOSBox-X normal/386 behavior: logical AF=0, SHL-by-one AF=1.
SHL AF is architecturally undefined; Unicorn returns0 at the pending boundary.
Those70 cases assert this difference and compare the other15 flag bits.
Other original comparisons and all native comparisons are unmasked. Different
x86 cores are not claimed to agree on undefined flags.

## Measured evidence

| Gate | Result |
| --- | --- |
| Original differential |292 cases:211 returns,70 handler stops,3 RunError204,5 list limits,1 heap limit,2 binding stops|
| Comparison |14 CPU words and all1,048,576 RAM bytes; only70 declared Unicorn SHL-AF exceptions|
| Removal coverage |94/94 instruction starts,20/20 conditional outcomes|
| Composed coverage |13 dispatcher prefix starts;207 chain starts,39 conditional outcomes including FreeMem|
| DOSBox-X normal/386 |128 cases:96 returns,30 handler stops,2 RunError204; CPU14,SS1KiB,DS64KiB,heap64KiB; no flag masks|
| Synthetic C |2,323 ASan/UBSan checks; focused CTest1/1|

Cases cover head/interior/tail removal, nil search/no match, same-segment
inequalities, missing predecessor, cycles/budgets, invalid/duplicate frees,
merges/top shrink, SS=DS, A20, record-offset wrapping, stack/record aliases,
odd/boundary SP, all256 key values and reached/unused invalid call bindings.
Native places the member in captured DS and nodes in captured heap RAM;
only PENDING and RunError entries are redirected to flag-preserving capture
traps. Generated COM/assembly, original media, inputs and captures stay ignored.

Native frames are synthetic. Natural gameplay reachability, dynamic-handler
execution or return, and U7D chain closure are not proved. Changed-return
handling exists, but this corpus does not claim coverage of that stop status.

Receipt SHA-256:
7fdcefebfa48d8e8672dd7a88403caf170dd252b2b7ae145b0db74d87632b3b5.
Native RESULT.BIN SHA-256:
e75ee1726f3e3a8511afe6c7dc5f918846cf9ca8eb9e416cd457c6b6e206ef02.

## Enumerable handlers and unresolved closure

The [JSON inventory](u9e_dispatch_handlers.json) and
[work checklist](U9E_DISPATCH_HANDLER_BACKLOG.md) record139 primary slots and
118 distinct targets:115 U38,2 U5E,1 U86. U38+31A2 writes138 pointers; U86+3413
supplies slot88. Primary table:DS:682C..6A57. There are43 canonical strict rows;
interfaces are16 raw CPU,37 scalar/callback,65 missing located models. The75
identities without canonical rows are separately listed work items.

The dispatcher addresses256 slots across DS:6828..6C27. Slots0 and140..214
are not initialized by these installers. Slots215..255 alias41 initialized
U86 procedure pointers outside the primary table; the JSON also records them.
Closing only118 handlers requires a proven type1..139 invariant and live-target
validation. Enumeration proves neither. U38+2EDA calls removal and dispatch
recursively, adding a closure dependency rather than an independent leaf.

## Reproduction

Owner media must be under ignored resources/DOS/POOLRAD.

    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u9e_list_release_0026_audit.py --unicorn --native --output resources/u9e-0026-audit.json
    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u9e_dispatch_table_audit.py --output resources/u9e-dispatch-handlers.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_overlay_u9e_list_release_0026_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_overlay_u9e_list_release_0026_cpu_tests$' --output-on-failure

Interface classes are an explicit reviewed snapshot at2225bc54, not a filename
heuristic. Canonical membership comes independently from the strict CSV, whose
hash is recorded. Review mappings and regenerate after interface/accounting
changes. The script never edits canonical accounting.

## Parity review record

- target_function: U9E+0026 with raw FreeMem and U9E+0000 prefix.
- source_candidates: overlay_u9e_list_release_0026_cpu.c and merged
  system_freemem_0254_cpu.c; overlay_u9e_entry2.c is a bounded semantic reference.
- signals: exact hashes, public entry/fixup, Pascal frames, list/heap effects,
  full local branch coverage, independent original execution/native captures,
  negative evidence from the unresolved handler.
- score: high for the declared partial boundary; insufficient for full-chain parity.
- verdict: likely_match for the partial contract, pending independent review.
- review_notes: inspect interface classes, AF profile, live aliases, capture
  windows and synthetic reachability limits. No classifier, identity, strict-map,
  credit or gameplay-completion claims are changed.
