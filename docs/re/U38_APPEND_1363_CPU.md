# U38 +1363 fixed sidecar append

This raw handler always appends a9-byte sidecar through the existing raw
U9E+0E52/GetMem chain, with type25, word value1, byte value12 and mode byte0.
The passed mode and the other two non-record arguments are ignored.
Public entry46, stub0041:0106, serves installed dispatch type56.
This author change requests independent review and assigns no strict credit.

## Identity and queue correction

| Artifact | Extent | SHA-256 |
| --- | --- | --- |
| START.EXE |65,231 bytes|d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd|
| GAME.OVR |206,105 bytes|872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614|
| U38+1363 |unit[1363,1382), file[65470,65501),31 bytes/15 instructions|4850fb98f51666178d3b6e8fc612f3a2afa80e355a7fa13cafb56073481a58c2|
| U9E+0E52 |180 bytes/57 instructions|cfde5096f098f1f0c4aa15f805eb6a02c3540f56a2f0df8632b99501a6366b20|

Selected from schema-8 rank26 after VP assignment. The queue's54-byte,
26-instruction upper bound includes23 trailing bytes[1382,1399) after RETF10.
They are excluded from function coverage. The caller has no conditional
branches. The source audit checks the hash, complete15-instruction extent,
entry46, original00B0:0052 call at+1379 and segment fixup+137C, next entry at
+1399, and the independently decoded installer binding for type56.

The pre-implementation recall query “poolrad U38 1363” timed out.
deja-health found a stale index staging directory; --repair launched a
background rebuild. No recalled result was reused. Reused repository
findings and harnesses are [U38_LOOKUP_APPEND_2814_CPU.md](U38_LOOKUP_APPEND_2814_CPU.md)
and the raw U9E+0E52 implementation/header/audit. The latter composes merged
SYSTEM GetMem. The new caller is compared against its own original bytes.

## CPU contract and observable effects

pr_u38_append_1363_cpu accepts exact1MiB A20-off RAM and14 CPU words, entered
after far CALL at IP1363. SP is conservatively0040..FFF2. The record offset
and segment are at SS:[SP+0Ah]/[SP+0Ch]; RETF10 consumes14 bytes total.
Code and relocated bindings remain immutable, reached return frames remain
nonwrapping, and callee profiles apply. Host structures are distinct from RAM
and each other. IRQs, faults and self-modifying code are excluded.

PUSH BP and the record-argument reads remain ordered and live. MOV AL,19h
preserves incoming AH in the pushed type word; MOV AX,1 then clears AH before
the other three values are pushed. No caller instruction changes flags, so
pre-call errors retain the input flags and successful returns retain the
callee's flags. The normal unaliased appended node has byte+0=19h, word+1=1,
byte+3=0Ch, byte+4=0, and nil next pointer at+5/+7.

The reached far-call opcode/offset and public stub's JMP offset are guarded;
the relocated stub segment and target body segment are read from RAM.
CODE stops at+1379 after all six callee arguments have been pushed, before
a far return frame is created. CALLEE returns the exact partial CPU/RAM and
append/GetMem statuses. TRANSFER stops if the callee returns to another
CS:IP. On a normal inner return, the caller POPs BP using the actual SP;
there is no invented MOV SP,BP or cached-frame restore.

Heap and list budgets may be zero. They retain the existing GetMem and
append semantics; allocation can already have happened when a list budget
stops traversal. ARGUMENT leaves CPU/RAM unchanged and initializes the
optional diagnostic to-1/-1. All other stops retain partial state without
rollback. A null diagnostic is allowed.

## Measured verification

| Gate | Result |
| --- | --- |
| Original differential |288 cases:267 returns,2 CODE,15 CALLEE,4 TRANSFER|
| Comparison |all14 CPU words and all1,048,576 RAM bytes; no masks|
| Caller coverage |15/15 instruction starts; zero conditional branches|
| Composed coverage |172 instruction starts,20 conditional outcomes|
| Native DOSBox-X normal/386 |120 cases:107 returns,13 RunError203; no masks|
| Native footprint |CPU14,SS1KiB,DS64KiB,heap64KiB,nodes16KiB|
| Focused C checks |2,064 ASan/UBSan checks; CTest1/1|

Original cases include varied heap/free lists, empty/nonempty/cyclic sidecar
lists, heap/list budgets, custom HeapError, binding corruption, SS=DS, A20
wrap, record-offset wrap, record/node/frame aliases, heap writes into saved
frames, seeded memory/flags, and odd/boundary SP. Four stack aliases change
the direct return and are checked at the exact transfer stop.
Native cases use real far CALLs with synthetic lists/heaps and fresh original
code; only SYSTEM RunError entry is redirected to capture. They do not
include the alias-induced transfer or wrapping fixtures.

Focused synthetic C checks cover all256 incoming AH values, varied ignored
arguments, exact node/list effects, caller/stub/callee errors, a zero-link
budget with a two-node chain, guards, and zero-budget empty-list success.
ASan/UBSan passes2,064 checks. A one-node list has no followed link; the
zero-link limit fixture therefore uses two nodes.

Combined receipt SHA-256:
6f159a44f644716d80d94353cff6d16bd5baa095165e040f58d5e705ec8a09a3.
Native RESULT.BIN SHA-256:
81c2aca8981362d99c8552c543c384ddfe58e2b1d1b979ca7517b3747a22b763.

Native frames are synthetic; no natural-game reachability is claimed.
Callee branches not reached by this corpus retain their existing evidence,
not an implied new coverage claim. Original media, extracted bytes,
assembly, COM, fixtures and captures remain under ignored resources/.

## Reproduction and inventory

    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u38_append_1363_audit.py --unicorn --native --output resources/u38-1363-evidence.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_overlay_u38_append_1363_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_overlay_u38_append_1363_cpu_tests$' --output-on-failure

The original audit compiles its isolated dependency chain with strict warnings.
Sanitizers use the same SOURCES list plus the focused test with
-fsanitize=address,undefined -fno-omit-frame-pointer.

The handler inventory maps type56 to this raw interface. It reads canonical
membership from the accepted strict CSV, including independent #1026 for
+133F. The refresh also maps the merged +0679 API from #1031.
Counts become23 raw/37 scalar-or-callback/58 missing interfaces;
48 accepted canonical handlers remain48. This PR does not credit +1363,
change canonical accounting, or mark issue #1019 complete.

## Parity review record

- target_function: OVL_0038_1363.
- source_candidates: overlay_u38_append_1363_cpu.c and the existing raw
  overlay_u9e_list_append_0e52_cpu.c/system_getmem_023f_cpu.c.
- signals: exact function hash/extent, public entry/installer binding,
  loader fixup, complete local coverage, full-state original and native runs.
- score: high behavioral evidence under the declared CPU/memory profile.
- verdict: likely_match, pending independent review.
- review_notes: inspect trailing-data exclusion, AH in the pre-call frame,
  live return aliases, zero budgets and native capture footprint.
  Independent reviewer decides verification/credit; no self-credit.

## Checker registration (2026-10-29)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0038_1363 --require-current-main
```
