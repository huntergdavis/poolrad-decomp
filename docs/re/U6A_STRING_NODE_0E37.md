# U6A +0E37: append a Pascal-string node with raw GetMem

The complete executable body snapshots a Pascal string with capacity
40 characters, allocates a 46-byte node, appends it through a parent
frame's tail pointer, copies the snapshot into the new node, and writes
the low byte of its attribute argument at node +29h. The raw CPU14/RAM
implementation composes the existing SYSTEM GetMem and StrStore APIs
unchanged. It retains partial effects on allocator stops and altered
callee returns.

## Executable boundary and identity

The queue's 389-byte / 159-instruction upper bound extends into inline
string data. The executable body is **[+0E37,+0EBE), 135 bytes and 48
instructions**, ending at RETF 8 at +0EBB. It has no conditional branch;
no executable path falls through into the following 254 data bytes.
The next code body starts at +0FBC. Those data bytes are not translated,
counted as instructions, or included in the new body hash.

- GAME.OVR file span [0x18668,0x186EF); U6A starts at file 0x17831.
- Body SHA-256:
  5f63f1823287894ede57fcef122307f3e296293a633e530a1836f2f36d389234.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- Three aligned far calls: +0E4B and +0EA5 to SYSTEM +0B25
  (StrStore, not Move), +0E54 to SYSTEM +023F (GetMem). Their segment
  words have loader fixups at +0E4E, +0E57, +0EA8.
- Four aligned incoming PUSH CS / near CALL sites in U6A +0FBC:
  +102C, +12DB, +130C, +1323. This is a private helper, not an overlay
  entry-table slot. The audit asserts all four references.

## Raw contract and ordered effects

Entry stack words after the far-call frame are parent-frame offset,
source-string offset, source-string segment, attribute. With BP set
after PUSH BP, these are BP+6, BP+8/BP+0Ah, BP+0Ch. RETF 8 consumes them.

1. StrStore copies up to 40 characters plus the length byte to SS:BP-29h.
2. GetMem requests 46 bytes, rounding the allocation to 48.
3. Through the live tail pointer at SS:parent-0Ch, the returned DX:AX
   is stored into old-node +2Ah/+2Ch.
4. The pointer is reread through that link and replaces the parent tail.
   The new node's link words are cleared.
5. StrStore copies the snapshot to the new node. The attribute's low byte
   is written at +29h. Byte +28h belongs to the 40-character string;
   no unused string bytes are initialized.

Every pointer and source operand is reread in instruction order, so
data/stack/heap aliases preserve the original effects. LES and RETF
frames are physically contiguous, with A20 wrapping at 1 MiB.
The entry requires IP=0E37 and initial SP in [0060,FFF4], original
immutable loaded bodies, and no writes into modeled code. CPU and
diagnostic objects must not alias RAM. All reached dependency far-return
frames must fit within SS:0000..FFFF, even after alias-induced BP changes.
Far-call segment words are read live; native tests relocate SYSTEM to
7000. Interrupts, faults and self-modifying code are outside this profile.

max_heap_steps is GetMem's taken free-list walk-edge cap. Zero allows
paths that need no such edge. GETMEM_STOP preserves the raw allocator
status and state: RunError 203 at +00E2, a custom HeapError at its call,
walk LIMIT, or an internal changed return. RunError itself and custom
HeapError bodies are not executed. TRANSFER preserves an unexpected
direct callee return; the two tested allocation/stack aliases stop after
the final StrStore changes its own return frame. ARGUMENT leaves RAM
and CPU unchanged; callee_status is -1 until a dependency has run.

## Evidence and reproduction

- **282 original-chain comparisons**, each matching all CPU14 words and
  the entire 1 MiB RAM with no mask or callee substitution: 247 returns,
  29 RunError stops, two custom HeapError stops, two walk-limit stops,
  and two changed StrStore returns.
- Caller coverage **48/48 instruction starts**, with no caller branches.
  Combined caller/StrStore/GetMem coverage is **163/171 starts and
  18/26 conditional outcomes**. Fixed allocation size 46 and the default
  HeapError do not exercise every runtime path.
- Inputs include every string length 0..255, truncation boundaries,
  exact/split/top allocation, full/cyclic heaps, cap equality, odd and
  boundary SP, SS=DS, A20 wrap, string-offset wrap, forward/backward
  source overlap, allocation equal to the old node, allocation over the
  final return frame, and a randomized RAM background.
- **64 DOSBox-X normal-core/386 synthetic cases**: 56 returns and eight
  RunError 203 stops. Each matches CPU14, 256 stack bytes, all 65,536 DS
  bytes, and a 16,384-byte heap arena. Original GetMem/StrStore/caller
  code runs with IRQs masked. A far jump at RunError's entry records
  the pre-RunError state; no callee result is substituted. No FLAGS
  masks or memory normalization.
- Native result SHA-256:
  ab11f0d3578c8668df0fb8aced36c3bff007034e9d8b38f63079f09bd67cc517.
- **280 synthetic C checks** pass under ASan/UBSan. Focused CTest: 1/1.
- Private receipt resources/u6a-0e37/evidence.json SHA-256:
  8cd625b5a0db8e6606a4e843b8e06d6d474f582b05fed0b7d3c9310e6160ef02.

With owner-supplied media under ignored resources/DOS/POOLRAD and
Capstone/Unicorn installed:

    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u6a_string_node_0e37_audit.py --unicorn --native --output resources/u6a-0e37/evidence.json
    cmake -S . -B resources/u6a-0e37/build -DBUILD_TESTING=ON
    cmake --build resources/u6a-0e37/build --target poolrad_overlay_u6a_string_node_0e37_tests
    ctest --test-dir resources/u6a-0e37/build -R '^poolrad_overlay_u6a_string_node_0e37_tests$' --output-on-failure

Original media, decoded data, generated assembly/COM and captures remain
under ignored resources/.

## Reuse and parity assessment

Local recall before implementation returned Claude session
281c8700-5ac (2026-10-18), identifying GetMem as U6A +0E37's blocker.
That dependency was verified against the actual call at +0E54.
The implementation links [GetMem from PR #983](https://github.com/huntergdavis/poolrad-decomp/pull/983)
and [the existing StrStore model](SYSTEM_STRSTORE_0B25.md) unchanged.
Arithmetic/memory primitives follow accepted U3C2 +0909. The audit heap
builder and native capture scaffolding adapt the GetMem audit.

- target_function: OVL_006A_0E37, executable [+0E37,+0EBE).
- source_candidates: overlay_u6a_string_node_0e37.c with unchanged
  system_getmem_023f_cpu.c and system_strstore_fullstate_0b25.c.
- signals: exact executable hash/bounds, three fixups, four aligned
  callers, frame/field offsets, live link ordering, full-state
  differential, native heap/error replay, focused C tests.
- score: five evidence categories (identity, call structure, differential,
  native replay, contract checks); no probabilistic match score.
- verdict: likely_match, pending independent verification.
- review_notes: native frames are synthetic, not a naturally captured
  gameplay route. Queue extent correction and helper classification
  remain for the independent lane. The allocator's non-default
  HeapError and RunError implementations remain explicit boundaries.
  No classifier, accounting, acceptance, or credit is changed.
