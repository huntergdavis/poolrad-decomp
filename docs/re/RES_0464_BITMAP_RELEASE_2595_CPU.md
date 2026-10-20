# RES_0464 +2595 raw bitmap release

This adds a CPU14/1MiB implementation of the complete resident caller and
composes the existing raw SYSTEM FreeMem model. The previous
[callback model](RESIDENT_464_BITMAP_RELEASE.md) remains available. This raw
API has no release callback and does not skip an allocation. VP i19241/i19245
assign this work after the U3B0 +06FB readiness declaration and the accepted
U9E +0026 dynamic-chain blocker receipt. No independent credit is claimed.

## Original extent and caller evidence

| Artifact | Extent | SHA-256 |
| --- | --- | --- |
| START.EXE | 65,231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206,105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Resident caller | 0464:[2595,2632), expanded image [6BD5,6C72), 157 bytes / 52 starts | d3cd5846fe2109fc1eb33b500561830c73601d21bea64575f7a08488301eb700 |

The extent comes from loader-expanded START.EXE bytes and the RETF4 at
+262F, not a queue row. The three far call sites +25F0/+2605/+261B all
resolve to SYSTEM 0AF8:0254; all three segment operands are checked against
the loader's relocation stream. At most two execute in one invocation:
+25F0 and +2605 are alternative auxiliary-release paths, followed by +261B.

The original-byte checker revalidates eleven incoming GAME.OVR far calls
and their overlay segment fixups: U17:+06F8, U2F:+005F,
U33:+1E46/+1E50, U68:+108A/+1094, U3B9:+04CA, and
U3CE:+00E1/+017A/+01A5/+0635. The earlier callback evidence establishes
bitmap lifetime use: U33 releases DS slots 6B01/6B06, and U17 releases a
local slot after the image routine. These support the caller/role argument;
they do not prove stock-library attribution or a natural gameplay capture.
The earlier game/library identity question remains for independent review.

The complete static execution closure is 390 bytes / 158 starts: the caller
plus FreeMem [0254,026E), [03D3,0432), [0432,0491), [0491,04A2).
The checker pins the four existing FreeMem hashes and runs their original
instructions; it does not substitute C results into the oracle.

## Contract and effects

`pr_r0464_bitmap_release_2595_cpu` enters at IP2595 after a far call.
SS:SP contains return IP, return CS, pointer-slot offset, pointer-slot segment.
Success executes RETF4. The slot stores a far pointer to the bitmap header.
A completely nil bitmap pointer returns without freeing; a partially nil
pointer is followed as the original follows it.

The low word of header byte+8 times word+11 is saved in the stack frame.
If auxiliary pointer+13/+15 is non-nil, it is freed first. Mode byte DS:4A1E
is read live; mode1 divides the truncated product by four using two SHR
instructions, while every other mode uses the full truncated product.
The caller then rereads its pointer slot, frees the primary allocation with
product+23 modulo65536, and clears offset then segment in the slot. A
failed primary free retains the prior auxiliary free; there is no rollback.
A zero-sized free follows SYSTEM's original no-op semantics.

The API accepts exactly1MiB A20-off ordinary RAM, entry SP[0040,FFF8], and
separate CPU/diagnostic host objects outside RAM. Effective offsets wrap at
16 bits; word/far-pointer operands are physically consecutive with 20-bit
wrap. Stack, data, bitmap and heap aliases remain live in original order.
All reached FreeMem entry frames must satisfy its SP range, and all reached
runtime return frames remain nonwrapping. Code and bindings are immutable;
interrupts, faults, devices and self-modifying code are outside the contract.

All three original CALL opcodes/offsets and their selected `system_cs`
segment bindings are validated before mutation, including unreachable sites.
This is an API precondition, not an assertion that the original validates
code. Invalid arguments leave RAM, CPU and diagnostics untouched. Valid entry
initializes the diagnostic to no call; it then records the exact most recent
FreeMem status/site and number of calls. A null diagnostic is supported.

`max_heap_steps` resets for each call and counts FreeMem's taken list-walk
back edges. There are no caller loops. RunError204, allocator walk limits
and changed internal returns stop with their exact partial state. An
unexpected successful far return produces TRANSFER. No arbitrary-PC resume,
error-handler continuation or invented cleanup is provided. The differential
measures the internal-return TRANSFER; the outer unexpected-far-return guard
is defensive and is not a claimed measured stop.

FLAGS use the existing DOSBox-X normal/386 convention: logical AF0 and SHR
AF1. MUL's undefined arithmetic bits are overwritten by OR at+25D0 before
any branch/call/API stop; its product and CF/OF are implemented. No output
register, FLAGS bit or RAM byte is masked or normalized in differential
comparison. No native checkpoint or intermediate-state claim is made inside
the overwritten MUL-FLAGS interval.

## Validation and limits

The original-byte checker runs every case from the resident entry, loading
that original body at a synthetic CS and the original FreeMem closure at
its binding. It compares all14 CPU words and every1,048,576 RAM bytes, plus
status, allocator diagnostic, call site and call count. The original decides
the stop; writes are mirrored for A20 wrap and any code write fails the
checker. No selected instruction effects or function bodies are replaced.
Coverage gates require every caller and FreeMem start and both outcomes of
every conditional branch; uncovered execution is a hard failure.

The native runner uses DOSBox-X normal/386 and synthetic entry frames. It
loads the original bodies and captures completed returns or RunError entry.
The sole synthetic code interception is a flags-neutral far JMP at SYSTEM
+00E2, before the first unexecuted RunError instruction; the handler itself
is not executed. Completed captures contain13 directly captured words plus
the known executed return-label IP; RunError captures contain12 directly
captured words plus declared original CS:IP. All comparison words are
unmasked. Captures cover SS1KiB, DS64KiB and heap64KiB (132,096 bytes), not
full native1MiB. No native budget, alias-transfer, A20 or natural saved-game
route claim is made. Native cases explicitly exclude slots outside captured
DS/heap memory. An initial out-of-window stack-slot fixture was rejected;
the corrected final corpus is rerun from scratch.

Synthetic C tests independently check rounded allocation sizes, free-list
links, mode-dependent auxiliary size, pointer clearing, failure after an
auxiliary free, walk limits, atomic rejection of each bad binding and host
argument, null diagnostics, and the Pascal return frame. The focused CTest
and ASan/UBSan execute the same semantic suite.

## Measured result

On base `940ffc70`, the final original-byte run passed367 cases:
340 returns,22 RunError204 stops,4 allocator walk limits and1 changed
internal return. All52/52 caller starts and6/6 caller branch outcomes,
and all158/158 closure starts and40/40 closure outcomes, were reached.
There are no coverage exclusions or missing starts/outcomes. All609
FreeMem calls originate in the resident caller. CPU14 and full1MiB RAM
match unmasked in every case, with exact status and diagnostics.

The final native run passed128 cases:113 completed calls and15 RunError
interceptions, with no CPU or captured-memory mismatch. RESULT.BIN SHA-256:
cd0e8baa51f317243b56f4d68ac6709376fba1aaf43937a00b0d20fe6f32a4c0.
The ASan/UBSan semantic test passed3,115 checks; focused CTest passed1/1.
The fetched-current-main compile/load/bind gate passed10 checkers/10 rows.
Main `738ba834` adds only the independent U3B0 +06FB declaration; all
resident execution dependencies remain unchanged. The private evidence receipt binds source/harness hashes, reports, tool
versions, generated native inputs/COM/output and build logs to the PR head.

## Reproduction and reuse

Use a Python environment with Capstone and Unicorn; keep owner media under
ignored `resources/DOS/POOLRAD`.

    python tools/overlay/res0464_bitmap_release_2595_audit.py --unicorn --native --output resources/validation-final.json
    python tools/overlay/check_checker_builds.py RES_0464_2595 --require-current-main --output resources/checker-builds.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_resident_0464_bitmap_release_2595_cpu_tests
    ctest --test-dir resources/build -R '^poolrad_resident_0464_bitmap_release_2595_cpu_tests$' --output-on-failure

The registered compile-only checker uses the same compilation, load and
symbol binding as the actual audit, without reading game media. The
manifest's `full` describes this scoped raw composition readiness; it does
not resolve identity or award strict credit.

Dunking Sheep context query `poolrad resident 0464 2595 bitmap release
FreeMem ownership` returned historical callback/constructor work in sessions
cdf47f7c-802d-44bb-beed-91a4e149fb8e and
12a7f8ad-6b6c-4d8e-b123-327925860532. That context led back to the existing
callback evidence; it is not reused as execution proof. The raw memory and
FLAGS idioms, original hook mechanics and native capture layout reuse the
merged U6C record-release and SYSTEM FreeMem harnesses. All measurements
here execute this new caller; no earlier pass is presented as a new run.

## Parity review

- target_function: RES_0464 [2595,2632), with raw SYSTEM FreeMem.
- source_candidates: resident_0464_bitmap_release_2595_cpu.c and unchanged system_freemem_0254_cpu.c.
- signals: loader-proved body/hash/end, three relocated calls, eleven fixup-backed callers, custom header arithmetic, live pointer rereads, exact Pascal frames, full-state original differential, native corroboration, independent heap-effect tests.
- score: nine evidence categories; a scoped behavioral match, not a source-identity probability.
- verdict: match within the stated contract, subject to independent acceptance.
- review_notes: distinguish readiness from game/library identity; native memory is a subset; all runtime stops retain state; no callback or target restriction is smuggled into a full declaration. Reviewer/VP owns merge and credit.
