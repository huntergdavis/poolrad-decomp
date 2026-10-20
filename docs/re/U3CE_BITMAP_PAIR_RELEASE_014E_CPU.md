# U3CE +014E raw bitmap-pair release

The complete decoded U3CE caller now composes accepted raw resident
0464:2595 and its SYSTEM FreeMem chain. The resident was independently
accepted in #1157. No indirect dispatch, host callback, allocator substitute,
or hidden dynamic chain was found after checking the body and incoming
fixups. This author change makes no identity, strict-count, acceptance-map,
or credit update; independent review owns those decisions.

## Media-derived extent and ownership

Pinned original media:

- START.EXE SHA-256: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
- GAME.OVR SHA-256: 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614

GAME.OVR file [203706,203804), U3CE [014E,01B0), is **98 bytes / 40 starts**.
RETF2 at 01AD ends the body. All branch targets remain within that extent.
Body SHA-256:
244d2bbd214ceb86d9d6daa80252da6af77fa0a92e4a287ac555aaca26b3c047.
The former queue bound [014E,01C8) included 24 trailing data bytes and
reported 122B/59 instructions. The hash-pinned ledger correction is carried
by #1158; this implementation and audit always use the decoded 98-byte end.

Public entry3 maps to 03E9:002F. Incoming far calls with segment fixups are
U6C:3321, U6C:337B, U6C:3EBF and U6F:065F. Both outgoing calls, at 017A and
01A5, have segment fixups targeting resident0464:2595. The audit pins these
six edges and the public entry. This indexed game bitmap-table orchestration
is distinct from the SYSTEM allocator it eventually invokes; no independent
historical source-library identity is inferred.

Composition basis: resident [2595,2632), 157B/52 starts, measured from the
loader-expanded START image [6BD5,6C72), SHA-256
d3cd5846fe2109fc1eb33b500561830c73601d21bea64575f7a08488301eb700;
SYSTEM FreeMem 233B/106 starts across the four accepted spans recorded by
the resident audit. The complete footprint is **488 bytes / 198 starts**.

## Raw behavior and contract

The byte at SS:BP+6 is zero-extended and multiplied by eight. If the far
pointer at DS:5E5A + 8*index is nonzero, its address is passed to resident
release. The argument byte is reloaded, and the same check/release is
performed for DS:5E5E + 8*index. The pointer slots, argument, registers,
locals and return frames remain live RAM. The root returns with RETF2.
All 256 byte indices are supported; the upper argument byte is ignored.

The API requires exactly 1MiB A20-off ordinary RAM, immutable code, entry
IP014E, SP[0080,FFFA], valid nonwrapping reached dependency frames, and
CPU/diagnostic objects outside RAM and mutually distinct. Interrupts,
faults and code writes are outside scope. Both caller bindings are checked
atomically before mutation; the resident's three SYSTEM bindings must meet
its published contract. There are at most two resident calls and no caller
loop. The explicit heap budget resets per FreeMem call.

Callee failure and unexpected return preserve exact partial CPU14/RAM.
Diagnostics record the last resident invocation and caller site/count.
ARGUMENT changes no object; a null diagnostic is supported. Intermediate
XOR/SHL flags are overwritten by OR or ADD before a branch/call/stop. The
accepted DOSBox-X normal/386 flag convention applies to runtime operations.
No stop-state register, flag, stack byte, or other memory byte is masked.

## Original-byte coverage and alias evidence

The differential executes **978 cases**, comparing unmasked **CPU14 plus
all 1,048,576 bytes**. Caller coverage is **40/40 starts and 4/4 outcomes**;
closure coverage is **198/198 starts and 44/44 outcomes**, with **zero
exclusions**. It executes 1,086 resident invocations. Stops: 932 returns,
36 RunError entries, 8 heap-budget limits, 1 resident internal near-return
transfer, and 1 changed resident far return reported by the caller as TRANSFER.

Cases cover both pointer positions, every byte index and upper argument-byte
noise, all resident/FreeMem branches, A20/stack bounds, random RAM and live
aliases. These are instruction-start/conditional-outcome measurements, not
an exhaustive enumeration of arbitrary memory states.

Two aliases prevent overclaiming a simplified model:

- DS=SS=0, SP=5E60, index0: the first pointer is initially nonzero. PUSH DS
  writes zero into that pointer after the caller's test. The raw resident
  observes nil and executes its nil-entry branch. Thus that outcome is
  feasible even though the caller ordinarily filters null pointers.
- A freed allocation header overlaps the root argument. FreeMem changes
  index0 to index8; the second check releases index8's pointer. The audit
  explicitly asserts the changed byte, both calls, and the second call site.
  Caching the argument at entry would fail this case.

Original execution hooks only observe coverage, mirror A20 writes, stop
at return/RunError entry, and enforce the same explicit heap budget. No
original helper is replaced by C or by a synthetic successful return.

## Native corroboration and checks

DOSBox-X normal/386 executes **128 synthetic COM cases**: 112 returns and
16 RunError204 entry interceptions, zero mismatches. CPU14 and **132,096
memory bytes** (SS1KiB, DS64KiB, heap64KiB) are compared without masks.
This native sample is not full1MiB evidence and does not claim native
coverage of the stack-alias/limit cases tested by the original-byte oracle.

The COM supplies synthetic Pascal frames and heaps, relocates the original
caller/resident to fixed test segments, and executes original FreeMem.
The only error interception is a flags-neutral far JMP at SYSTEM:00E2 to
the recorder. It measures entry to RunError rather than error termination.
No semantic stub replaces a release. The heap budget for native comparison
is 64. Stack residue in the stated window is compared.
DOSBox executable SHA-256:
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
Native RESULT.BIN SHA-256:
9418cb776a34a924761f1b72cd302d3e489a7dd9a70a4835f5b4233ec8f2d843.

Synthetic C semantics pass **6,434 checks with ASan/UBSan**, including all
256 indices and all four pointer-presence combinations, allocator effects,
error/limit partial states, atomic binding rejection and null diagnostics.
The registered CMake/CTest target passes. CHECKER_ROWS registers
OVL_03CE_014E, and compile-only builds, loads and binds the actual caller
plus its two dependency source files before media access. The handoff
receipt pins author HEAD and fetched main; this build gate confers no credit.

Reproduce with owner-supplied media under ignored resources/DOS:

    python tools/overlay/u3ce_bitmap_pair_release_014e_audit.py --unicorn --native --output resources/u3ce-evidence.json
    python tools/overlay/u3ce_bitmap_pair_release_014e_audit.py --compile-only
    python tools/overlay/check_checker_builds.py --all --require-current-main --output resources/checker-builds.json

Python needs Capstone/Unicorn; native needs a C compiler, GNU as/ld and
DOSBox-X. Media, generated assembly/COM, and captures remain ignored.

## Reused context

Dunking Sheep recall for “poolrad U3B9 048F U3CE 014E bitmap release ownership”
located historical U3CE boundary work in session
a3ad1536-abb2-4fb8-af2c-1362b8656678. That boundary is coordination context,
not full execution evidence. VP i19255 assigns this caller after U3B9;
i19257 confirms the decoded extent. The accepted resident API, memory/FLAGS
idioms and oracle mechanics are reused; all results above are fresh runs
through U3CE. See RES_0464_BITMAP_RELEASE_2595_CPU.md for its contract.
