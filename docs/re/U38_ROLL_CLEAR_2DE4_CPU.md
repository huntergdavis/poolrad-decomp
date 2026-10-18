# U38 entry 108 (+2DE4): raw dice and helper composition

The complete 36-byte, 19-instruction caller now composes the merged raw
U9E +0DE3 dice/SYSTEM chain and U38 +0000 helper. It rolls 1d100 and, when
the unsigned result is at most 30, calls helper(0Bh), then helper(35h).
It preserves the incoming high byte in the argument pushes, applies each
callee's live CPU/RAM effects, and returns with RETF 0Ah. An unexpected
callee return stops at that actual CS:IP, retaining earlier writes.

## Identity and reused evidence

- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- U38 entry108, complete [+2DE4,+2E08), file [0x11A3F,0x11A63).
  Body SHA-256:
  7c0e900e0e89d330ef9f35ad0dcfa3550859a8eb46f4e1e311b40b585f591943.
- The far call at +2DED names loaded stub :0048, backed by the +2DF0
  segment fixup. The stub jumps to U9E +0DE3. Calls +2DFA and +2E01
  are PUSH CS plus near CALL +0000. The audit checks instruction-aligned
  calls, slot108, next-function boundary, byte hash, and fixup membership.
- U38 +0000 helper SHA-256:
  f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240.
- Provenance reused from the old unmerged branch
  codex/fullc-u38-2de4, commit
  60630524b33cc4c442de6a986c11f959e23f3af2:
  its U38_ROLL_CLEAR_2DE4.md identifies the indirect effect-table route
  (item124, DS:6A18, installed by +31A2). Its 192 original/stub and four
  native/stub cases are **not** full RNG-chain evidence and are not
  included in the new measurements.
- The raw caller helpers and replay scaffolding are adapted from merged
  overlay_u38_roll_gate_2915_cpu.c (+2992) and its audit/native tools.
  The U9E and U38 dependency implementations are linked unchanged.
- Local recall was attempted with deja before implementation; it timed
  out. deja-health reported a healthy 248 MiB index; hook-context returned
  no usable context. No recall result is represented as evidence.

## API profile

pr_u38_roll_clear_2de4_cpu takes exactly 1 MiB A20-off RAM, CPU14,
max_system_calls, and optional callee_status. Entry IP is +2DE4 and
initial SP is in [0040,FFFC]. The caller and dependency code must remain
original and immutable under their loaded mappings; SYSTEM is at 0AF8.
All reached U9E/SYSTEM far-return frames must fit in SS:0000..FFFF,
including if aliases change BP. Wrapped dependency RETF frames and
self-modifying code are outside this profile.

The budget is checked by the existing dice API immediately before a
SYSTEM slice. Zero budget therefore retains the caller pushes and dice
prefix, stopping at U9E +0E0E. CODE stops at the unsupported caller far
call, ARGUMENT leaves RAM/CPU unchanged, CALLEE keeps the dependency's
stop, and TRANSFER keeps a changed return. callee_status is -1 before
any dependency ran, otherwise the last dependency status.

DS/SS aliases remain live, including seed writes into return frames.
Both helper calls reread the key and latch in source order; clearing the
latch may alter a saved BP or return word. The caller checks both helper
returns before proceeding. Multi-byte caller accesses are physically
contiguous, wrapping at 1 MiB. Original flag outcomes are compared
without masks.

## Reproduction

From the repository root with owner-supplied media under ignored
resources/DOS/POOLRAD and Capstone/Unicorn installed:

    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u38_roll_clear_2de4_audit.py --unicorn --native --output resources/u38-2de4/evidence.json
    cmake -S . -B resources/u38-2de4/build -DBUILD_TESTING=ON
    cmake --build resources/u38-2de4/build --target poolrad_overlay_u38_roll_clear_2de4_cpu_tests
    ctest --test-dir resources/u38-2de4/build -R '^poolrad_overlay_u38_roll_clear_2de4_cpu_tests$' --output-on-failure

## Measured evidence

- **420 original-chain comparisons:** 413 returns, three zero-budget stops
  at U9E +0E0E, and four alias-induced transfer stops. Each compares all
  14 CPU words and all 1 MiB RAM, with no FLAGS mask or callee substitution.
  Cases include odd and boundary SP, SS=DS, A20 wrap, random background,
  preserved DF/IF, latch keys 0/0Bh/35h/34h/FFh, and roll results exactly
  30 and 31.
- Caller coverage: **19/19 instruction starts, 2/2 conditional outcomes**.
  The five-body chain (caller, helper, U9E, SYSTEM random, SYSTEM seed)
  measured **90/91 starts and 9/14 conditional outcomes**. This caller's
  one-die path does not establish every dependency branch.
- The four changed-return cases preserve a SYSTEM near-return change
  (0AF8:E4D0), a dice far-return change (1801:2DF2), and helper return
  changes to caller :2D00 and :2E00. The second helper stop is recognized
  only after its RETF, because :2E00 is also visited on normal execution.
- **120 DOSBox-X normal-core/386 synthetic far-frame cases** matched CPU14,
  all 64 bytes of the exercised stack window, and 27 watched data bytes
  including every DS write made by this chain. IRQs were masked; no FLAGS
  or memory masks. The harness runs original dice and SYSTEM code,
  including the original +1245 multiplier. Native capture SHA-256:
  132d8dcbb4e92daa9739bdc9f437872e7bbcb0e34961f13f36a1b01f88dd80fe.
- **782 synthetic C checks** passed with AddressSanitizer and
  UndefinedBehaviorSanitizer, and the registered CTest passed 1/1.
- Private audit receipt resources/u38-2de4/evidence.json SHA-256:
  72318dba59511cafda97274e737c3d13d19b49f442d62de2f07cd8bde95c61ee.

The native cases are synthetic machine-state replays. They do not claim
natural gameplay reach or all-memory native capture; full-RAM comparison
is supplied by the independent original-byte differential.

## Parity assessment and limits

- target_function: OVL_0038_2DE4, complete entry108.
- source_candidates: overlay_u38_roll_clear_2de4_cpu.c with the unchanged
  U38 helper cluster and U9E full-state composition.
- signals: exact bounds/hash/slot/fixup, aligned three-call graph, low-byte
  constants 1/100/30/0Bh/35h, call order, live stack/data writes, CPU14,
  original-chain and DOSBox-X differential results.
- score: five independent evidence categories (identity, structure,
  CPU/RAM differential, native replay, API-stop tests); no probabilistic
  match percentage assigned.
- verdict: likely_match, pending independent reviewer verification.
- review_notes: synthetic far-call native frames establish executable
  chain parity, not a naturally reached gameplay route. The indirect
  effect-table provenance has no newly captured natural caller frame.
  Queue readiness must distinguish the obsolete callback-only branch
  from this raw composition and retain the dependency stack/code profile.
  No classification, acceptance ledger, denominator, or credit is edited.
