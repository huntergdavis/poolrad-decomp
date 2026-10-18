# U38 dispatch no-op handlers: +0647, +1355, +135C, +319B

Four distinct original identities share the same seven-byte, four-instruction
body: PUSH BP; MOV BP,SP; POP BP; RETF10. The raw C preserves the saved-BP write
and consumes the five argument words through the live far-return frame.
It does not replace the routine with an empty host function.

Assignment: VP i17989, tracking [#1019](https://github.com/huntergdavis/poolrad-decomp/issues/1019).
The four bodies cover 12 primary dispatcher slots. This PR supplies separate
identity evidence for reviewer verification; it does not edit strict credit,
classification, or tracker checkboxes. The interface-only inventory update
locates these four raw APIs: 20 raw / 37 scalar-or-callback / 61 missing models.
Canonical membership stays 43 rows, with 75 identities still unchecked.

## Original identities

| Unit offset | Public entry | Original stub | File interval | Dispatcher types |
| --- | ---: | --- | --- | --- |
| +0647 |15|0041:006B|[62114,62121)|16|
| +1355 |44|0041:00FC|[65456,65463)|54|
| +135C |45|0041:0101|[65463,65470)|55|
| +319B |116|0041:0264|[73206,73213)|5,13,19,24,27,60,63,92,127|

Every interval is exactly 7 bytes / 4 instructions, with no trailing data,
calls, branches, or unmodeled effects. Their shared body SHA-256 is
3d5d31028d8927cd81dffdf3a86f216fa4eb84d0874dc3daada06bd31188514d.
The audit resolves all four public entries independently from the overlay
descriptor, and matches each stub to U38 +31A2's original installer stores.
Byte equality does not collapse the four source identities into one credited
address, nor inflate nine uses of +319B into nine identities.

Pinned owner media:

- START.EXE, 65,231 bytes, SHA-256
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- GAME.OVR, 206,105 bytes, SHA-256
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- Installer [31A2,38A9), SHA-256
  4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519.

## Raw API

pr_u38_dispatch_nop_cpu accepts exactly 1 MiB A20-off RAM and the standard
14-word AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS state. IP selects one of
the four original entries. Entry SP is restricted to [0002,FFFC], ensuring
nonwrapping saved-BP and far-return operands; final SP wraps modulo 65536.
CPU storage is separate from RAM. Code is immutable; interrupts and CPU faults
are excluded. No original instructions or code-binding bytes are required.

The sole data write is the incoming BP word at entry SS:SP-2. The actual POP
restores BP, and RETF reads the return IP/CS from RAM then advances SP by14.
All other registers and every FLAGS bit remain unchanged. Physical accesses
wrap at 1 MiB; multi-byte accesses are physically contiguous. Null inputs,
wrong RAM size, unsupported entry IP, or out-of-profile SP return ARGUMENT
without changing CPU/RAM. No callback, heap, budget, or external callee exists.

## Measured checks

| Original identity | Original-byte cases | Instruction starts | Native cases |
| --- | ---: | ---: | ---: |
| U38 +0647 |96|4/4|6|
| U38 +1355 |96|4/4|6|
| U38 +135C |96|4/4|6|
| U38 +319B |96|4/4|6|

All 384 original-byte differential cases compare all 14 CPU words and every
byte of 1 MiB RAM, with no flag masks. Cases vary initial registers/flags,
return addresses, ordinary/shared/high stack segments, odd SP, minimum SP,
FFF0/FFFB/FFFC, and seeded full-RAM backgrounds. A20-wrapped Unicorn writes
are mirrored to the corresponding low-memory alias.

The 24 DOSBox-X normal/386 cases execute the pinned original body at its
original offset in segment8800, entered through a real far CALL with five
synthetic argument words. They compare all 14 CPU words and 64 bytes around
the stack frame. All flags, including AF and DF, are unmasked. The frame is
captured before PUSHF can reuse it. Other data windows are unnecessary for
these bodies, whose only write is the captured saved BP. Full-RAM comparison
in the original-byte oracle separately verifies the absence of other writes.

The native oracle uses synthetic frames; it does not execute the dispatcher
or prove natural gameplay reachability. Those remain separate from body
parity and installer-backed identity. Original bodies, assembly, COM and
captures stay under ignored resources/.

Native RESULT.BIN SHA-256:
c20b87f593078989fed56cd6dff8870e99288a37638b3ffdb1972cabd3dcfdb8.
Audit receipt SHA-256: fc634f6fcf16e5bf352b02faa79783391124431d72c889e58783a9fb9689543f.
Synthetic C: 586 checks under ASan/UBSan. Focused CTest results are recorded in the PR.

## Reproduction and reuse

    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u38_dispatch_nop_audit.py --unicorn --native --output resources/u38-nop-audit.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_overlay_u38_dispatch_nop_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_overlay_u38_dispatch_nop_cpu_tests$' --output-on-failure

Recall queries for the U38 cluster timed out after index-update output;
deja-health reported an incomplete staging directory and a timed-out probe.
The prescribed deja-health --repair started background rebuild pid551029.
No prior-session result was reused. The target/slot mapping comes from #1020's
[dispatch inventory](https://github.com/huntergdavis/poolrad-decomp/pull/1020)
and the original installer, rechecked here. The existing U38 installer native
harness supplies the synthetic far-call/capture method; the shared Cpu and
media-descriptor helpers come from u86_procvars_3413_audit.py.

## Parity review record

- target_function: four independently mapped U38 handlers +0647/+1355/+135C/+319B.
- source_candidates: overlay_u38_dispatch_nop_cpu.c, shared raw body selected by IP.
- signals: exact hashes, public descriptor entries, installer slot mapping,
  RETF10 ABI, observable BP stack write, full-state original and native checks.
- score: high for each complete leaf body; no dispatcher/gameplay closure claim.
- verdict: likely_match, pending independent reviewer verification.
- review_notes: distinguish four identities from 12 slots; inspect SP profile,
  A20 behavior, unchanged flags, and capture-before-PUSHF ordering. The 118-handler
  closure still requires the separately tracked type-range and live-binding proof.
