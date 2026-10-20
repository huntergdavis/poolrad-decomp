# U3B9 +048F raw bitmap-set release

This adds the complete decoded caller composed with accepted raw resident
0464:2595 and SYSTEM FillChar 0AF8:16A8. Resident +2595 was independently
accepted in #1157; its raw FreeMem chain is included here. No hidden dynamic
chain was found after rechecking incoming fixups and the two outgoing calls.
The older Claude callback model is absent from current main; this is a raw
composition addition, not a claim to credit that historical model again.
Readiness, implementation, and source identity remain separate. This author
PR makes no strict-count or source-identity update; independent review owns credit.

## Original and ownership

Pinned media:

- START.EXE: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
- GAME.OVR: 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614

The actual GAME.OVR span is [189789,189894), unit [048F,04F8):
**105 bytes / 41 starts**, ending in RETF4 at 04F5. SHA-256:
08c8d4e1d889b5373c9d85613dd0530f4485892ee29681f5a14efcbfb203dcde.
The queue's 106-byte bound included one trailing byte. The ledger generator
now uses this hash-pinned end before decoding. All conditional targets stay
inside the decoded body; an early-return heuristic is not being applied.

Public entry3 maps to stub 03D0:002F. Three incoming far calls have segment
fixups: U33:1E3C, U68:1080, U75:0109. Local U3B9:00DF calls 048F after PUSH CS
at 00DE, supplying its far-return frame. The audit checks all four edges.
The caller reads DS globals and ES bitmap-set memory and invokes game-owned
resident bitmap release; it is game orchestration, not a SYSTEM leaf.
This does not resolve independent historical source-library ownership.

Both outgoing segments are fixup-backed: 04CA -> 0464:2595 and
04E3 -> 0AF8:16A8. The resident is 157B/52 starts from the loader-expanded
START image, SHA-256
d3cd5846fe2109fc1eb33b500561830c73601d21bea64575f7a08488301eb700.
FreeMem is 233B/106 starts across its four accepted spans. FillChar is
20B/7 starts, [16A8,16BC), SHA-256
f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7.
The exact composed footprint is **515B / 206 starts**.

## Semantics and API boundary

The far Pascal argument points to a bitmap set. Its first byte is a signed
count copied into a stack local. Counts 1..127 enter the release loop;
0 and 128..255 skip it. DS:70C7 is the live byte counter. Each iteration
passes set + 8*signextend(counter) - 2 to raw resident release. The bound,
base pointer, counter, and callee effects are read from live emulated RAM.
There is no restriction to the usual eight entries. On completion, raw
FillChar zeroes exactly 66 bytes, DS:6B0A becomes 0, DS:6B1C becomes FF,
and RETF4 restores the frame from live RAM.

The header specifies exactly 1MiB A20-off ordinary RAM, immutable code,
entry IP048F, SP[0080,FFF8], valid nonwrapping reached dependency frames,
and distinct host CPU/diagnostic objects. Both caller bindings are checked
atomically before mutation. The accepted resident's SYSTEM binding contract
also applies. Interrupts, faults and code writes are outside scope.

The explicit release budget stops at 04CA before CALL, after pushing its
argument. The separate heap budget resets per FreeMem invocation. A callee
stop or unexpected return preserves partial CPU14 and all RAM, including
stack writes. Diagnostics retain the latest resident status and invocation,
plus total resident and FillChar calls. The null-diagnostic path is supported.
No host callback, copied record, allocator substitution, or rollback exists.

FLAGS use the accepted DOSBox-X normal/386 convention. SHL and INC intermediate
flags are overwritten by subsequent ADDs before any exposed stop; resident
MUL flags are similarly overwritten by OR. We make no intermediate-instruction
flag claim. **All compared stop-state flags and memory are unmasked.**

## Measured evidence

Original-byte Unicorn differential: **626 cases**, CPU14 plus **all 1,048,576
bytes**, zero masks or exclusions, caller **41/41 starts, 4/4 outcomes**,
closure **206/206 starts, 44/44 outcomes**. The corpus executes 8,623 resident
calls and 599 FillChar calls. Stops: 598 normal returns, 20 RunError entries,
4 heap limits, 2 caller limits, 1 changed FillChar return, 1 resident internal
near-return transfer. It includes every signed count, populated count127,
all resident/FreeMem conditional outcomes, A20/offset wrap, stack bounds,
random RAM, and selected live aliases. This is exhaustive start/outcome
coverage of this decoded closure, not exhaustive enumeration of all RAM states.

The oracle runs original instructions only. Host hooks observe coverage,
mirror A20 writes, enforce the same explicit budgets, and stop at return or
RunError entry. They do not implement any callee behavior.

DOSBox-X normal/386 native corroboration: **128 synthetic COM cases**,
113 returns and 15 RunError204 entries, CPU14 plus SS1KiB, DS64KiB and
heap64KiB (**132,096 memory bytes**, not full 1MiB), zero masks/mismatches.
The native set includes counts 0,2,7,8,9,127,128,255. Budgets are 256 releases
and 512 heap steps. Native uses real caller/resident/FreeMem/FillChar bytes,
relocated resident/overlay segments and synthetic Pascal frames/heaps.
The sole error interception is a flags-neutral far JMP at SYSTEM:00E2 to
the recorder; it observes entry to RunError, not DOS error termination.
Return/frame aliases and limit stops have original-byte differential evidence,
not native coverage claims. The native snapshots retain all stack residue
inside the stated window. DOSBox executable SHA-256:
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
RESULT.BIN SHA-256:
697b76bfe1fdcc5aa8dfeecfd9f0b5af1d294d9aa37344af4bc20eb23e6b66b0.

Synthetic C semantics: **18,471 checks**, ASan/UBSan. CMake registers the
same test. CHECKER_ROWS registers OVL_03B9_048F; compile-only really builds,
loads, and binds this caller with all three dependency source files before
media access. The pre-handoff receipt records fetched main and author HEAD;
it is a build gate, not parity evidence or credit.

Reproduce with owner-supplied media under ignored resources/DOS:

    python tools/overlay/u3b9_bitmap_set_release_048f_audit.py --unicorn --native --output resources/u3b9-evidence.json
    python tools/overlay/u3b9_bitmap_set_release_048f_audit.py --compile-only
    python tools/overlay/check_checker_builds.py --all --require-current-main --output resources/checker-builds.json

Use Python with Capstone and Unicorn, a C compiler, GNU as/ld, and DOSBox-X.
Original media, generated assembly/COM and snapshots remain ignored.

## Reuse

Dunking Sheep context for “poolrad U3B9 048F U3CE 014E bitmap release ownership”
located historical sessions 9742b6e1-d504-40a0-92b9-7d2536d3f756 and
4d9851e1-f261-48b9-9cfa-5876d1500eb7. Their old callback model/ownership
messages were coordination context only. Current assignment is VP i19255,
with extent/composition clarification i19257. Accepted resident and SYSTEM
APIs and harness mechanics are reused; all results above were freshly run
through this caller. See RES_0464_BITMAP_RELEASE_2595_CPU.md for the resident
contract. No original game bytes are embedded in tracked code or tests.
