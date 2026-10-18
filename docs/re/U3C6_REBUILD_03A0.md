# U3C6 +03A0: rebuild occupancy and relative-coordinate arrays

## Original identity and selection

- Target: OVL_03C6_03A0, public entry 5, stub 03E0:0039.
- GAME.OVR U3C6 [03A0,04BE), file [30ADA,30BF8): 286 bytes,
  122 instruction starts, no arguments, RETF at +04BD.
- Body SHA-256: 50c688892105995b01a842c1e26412f1b4c221e58a1be29ea1fa41bb7abfe400.
- START.EXE SHA-256: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- GAME.OVR SHA-256: 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- Two aligned PUSH CS / near CALL sites: U3C6 +103E and +1286.
  Four fixup-backed far callers: U5E +083F, U9E +0FAE,
  U33 +1A27 and +1D74. All are asserted by the source audit.
- Outgoing calls: SYSTEM FillChar +16A8 at +03B2, relocation +03B5,
  and same-CS U3C6 +0000 at +040C, preceded by PUSH CS.

The schema-4 workflow's current schema-8 queue ranked this body 186 and
marked it composable. U31 +0255 was left to Claude. The higher-ranked
U3C6 +10A7 remains dependent on +03A0. Selection/claim was reported to the
VP in i17761. The queue still classifies +03A0 as
game_or_other_library_unresolved and labels +0000 modeled. This PR does
not change either classification or claim game-function credit.

Before implementation, recall query deja "U3C6 03A0 10A7" found the old
read-only +03A0 lead in session a3ad1536-abb (286 bytes, 122 instructions,
no arguments) and current lane coordination in session 281c8700-5ac.
That provenance/ownership context was reused; no unpublished source was
imported. The new implementation uses existing main's raw +0000 API and
SYSTEM FillChar API, with CPU/arithmetic primitives adapted from accepted
U3C2 +0909.

## Behavior and raw CPU contract

The complete function first calls FillChar to clear DS:[60DB,65BD),
1,250 bytes. It then caches DS:5F2A's byte count in its stack frame and
visits slots 1 through that count. A zero count skips all slots. A zero
shape byte at DS:5F2A+4*slot skips that slot.

For each active slot, four calls to U3C6 +0000 read the shape's byte pairs
from DS:2844/2845. If the helper returns a nonzero AL, the function sign
extends each coordinate byte and writes the slot index at
DS:60DB + 50*(slotY+shapeY) + slotX+shapeX, with original 16-bit offset
wrapping. It does not clip negative coordinates or positions outside the
50-by-25 cleared region. Later slots overwrite earlier occupancy marks.

After those four cells, it reloads the far pointer at DS:6716/6718 twice
and writes slotX-originX to DS:604A+slot and slotY-originY to DS:6092+slot.
Origins are bytes at map offsets +2/+3; both subtraction results are stored
as bytes. All slot fields, output addresses, map pointers, local counters,
and return frames remain live. FillChar can erase an internal return frame
or the caller's final frame when SS aliases DS. Occupancy writes and
relative-array writes can also alter later inputs. No host-side snapshots
replace those reads.

The public API pr_u3c6_rebuild_03a0 takes CPU14, exactly 1 MiB A20-off RAM,
a loaded SYSTEM segment, and two positive budgets. It composes the raw
pr_u3c6_table_pair_0000 with explicit WRAPPED_SS return policy and the
raw pr_system_fillchar_16a8. There are no callback substitutes.

- Initial IP is 03A0 and SP is [001C,FFFC]. The deepest normal helper
  frame is 28 bytes below entry SP; the caller has no stack arguments.
- CPU must be a separate host object. Original loaded code and bindings
  remain immutable; writes must avoid modeled code. Data/stack aliases
  are supported within those preconditions and the dependency contracts.
- Effective offsets wrap at 16 bits; physical addresses wrap at 20 bits.
  Bytes within a word are physically contiguous. Both caller LES operands
  begin at fixed DS:6716, so the pointer operand itself cannot straddle
  offset FFFF. No wrapping far-return-frame behavior is claimed.
- Slot budget counts visits before +03CF; cell budget counts visits before
  +03ED and is shared across all slots. Exhaustion exposes the exact partial
  RAM/CPU before the declined instruction. Neither limit invents a return.
- TRANSFER preserves a changed direct-callee return immediately. CALLEE
  preserves a rejected dependency state; no CALLEE case was measured.
  If only the final caller frame changes, the completed return exposes
  that modified CS:IP and saved BP.
- MUL's undefined arithmetic flags do not escape: +044F ADD defines all
  six arithmetic flags before a store, call, return, or budget boundary.
  No FLAGS masks or emulator substitutions are used in comparisons.

## Evidence and reproduction

The measured closure has three complete bodies: caller 286B/122 starts,
U3C6 +0000 104B/41 starts, and SYSTEM FillChar 20B/7 starts; total
410 bytes and 170 instruction starts. Helper SHA-256 pins:

| Body | SHA-256 |
| --- | --- |
| U3C6 [0000,0068) | 05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207 |
| SYSTEM [16A8,16BC) | f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7 |

- **440 original-code cases** compare CPU14, all 1,048,576 RAM bytes,
  and exact stop status: 427 full returns, four slot-limit stops, six
  cell-limit stops, and three changed FillChar returns. Caller coverage is
  **122/122 sites and 10/10 conditional outcomes**; the complete closure is
  **170/170 sites and 14/14 outcomes**, with no uncovered sites or edges.
- Cases include every shape byte, counts 0/1/72/73/108/109/254/255,
  coordinate and signed-byte boundaries, negative shape pairs, colliding
  occupancy writes, map-pointer offset wrap, map/relative-array overlap,
  stack/data aliases, relocated/A20 code/data/stacks, and budget stops.
- **35 native DOSBox-X full returns** compare CPU14, all 65,536 DS bytes,
  and 256 selected SS bytes (the SS=DS case overlaps those physical bytes).
  Original code executes with normal core / 386 and interrupts disabled.
  Native cases include all four allowed SP endpoints, signed coordinates,
  counts through 255, overlapping map origins, and SS=DS. Capture bookkeeping
  uses the harness's own stack so it does not contaminate compared DS.
- Strict C11 with -Wall/-Wextra/-Wpedantic/-Werror and ASan+UBSan:
  **52 semantic checks passed**. Focused CMake/CTest: **1/1 passed**.

With owner-supplied media under ignored resources/DOS/POOLRAD, and Capstone
and Unicorn on the Python path:

    python3 tools/overlay/u3c6_rebuild_03a0_audit.py --output resources/u3c6-03a0/audit.json
    python3 tools/overlay/u3c6_rebuild_03a0_native.py
    cmake -S . -B resources/u3c6-03a0/build -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/u3c6-03a0/build --target poolrad_overlay_u3c6_rebuild_03a0_tests
    ctest --test-dir resources/u3c6-03a0/build -R '^poolrad_overlay_u3c6_rebuild_03a0_tests$' --output-on-failure

The differential harness loads all original body bytes, changes only the
SYSTEM call's segment binding, and executes the original callees. It fails
on unexpected PCs, modeled code writes, an instruction ceiling, or any
CPU/RAM/status mismatch. Native captures are synthetic direct calls, not
measurements of a saved gameplay route. Original media and captures remain
ignored. Full static site/edge coverage is not proof over every possible
alias combination or execution environment.

## Receipt and tool pins

- Integration base: 8db70016fde526a5510580ad226cb6bfacbb42e4.
- Private receipt: resources/u3c6-03a0/evidence.json, SHA-256
  **fb7ebadeb26e3b87fbafa07707d8331ee8247eae8e1b7e488478722fb000d1cc**.
- Aggregate native capture SHA-256:
  05d76f648a959e88cca0ea24b41f6341ca25fa5781fdf745c4b2f6ca417165cb.
- Ordered original closure SHA-256 (kind/start order in receipt):
  b8a8af51ee2cab28d9810b1bb5cf9f8335a23bff0d2f1ecfb4bf57b3ddf52426.
- Python 3.14.4, Capstone 5.0.7,
  Unicorn 2.1.4; cc (Ubuntu 15.2.0-16ubuntu1) 15.2.0.
- DOSBox-X executable SHA-256:
  99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.

The receipt pins caller/header/test/tool sources, dependency source/header
files, each original body, media, individual native captures, report files,
and queue classification/readiness. It records evidence, not acceptance.

## Author parity assessment

- target_function: OVL_03C6_03A0, complete entry5 [03A0,04BE).
- source_candidates: overlay_u3c6_rebuild_03a0.c, accepted raw table-pair
  source, and SYSTEM runtime leaves source.
- signals: hash-pinned body bounds, public stub, six aligned incoming
  callers, both outgoing callees, complete static site/branch coverage,
  CPU14/whole-RAM comparison, negative budget/return tests, and separate
  native original-chain captures without masks.
- score: high identity and within-contract parity evidence; original
  game/library ownership classification and live-route reachability remain
  unresolved. No native wrapping-frame or native changed-return claim.
- verdict: likely_match, pending independent review.
- review_notes: no credit, acceptance, classification, or accounting edits.
  Queue readiness alone did not establish the raw helper contract; this
  composition used and measured the existing complete CPU interface.
