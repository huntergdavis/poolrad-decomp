# U5E +32E6: compose slot fields, candidate builder, and parent records

## Loaded-state precondition

Added 2026-10-22 by par-e (credit authority), VP i21329, ruling i21377. This states a condition the credit always
assumed; the credit is unchanged.

This credit holds **only while every overlay unit the body far-calls is resident**.
The C now reads each listed live `LCALL` binding and requires its stub to hold a
five-byte far `JMP` (`EA`) to the expected entry. Explicit callee-segment
arguments must agree with the jump target; U6A's predicate uses the live jump
segment. If a required unit is absent or a binding is malformed, **the C
refuses with `ARGUMENT` before changing CPU14 or RAM**. The caller no longer
has to enforce this listed precondition on the C's behalf.

Bindings and code must remain immutable throughout the synchronous call.
The overlay manager's DOS loading path is still outside this C model.
Loaded-state differentials execute real far-JMP stubs; separate host checks
exercise cold stubs and malformed bindings and verify atomic refusal. Host
checks are not original executions and add no parity-case or credit count.
Updated validation results are pending; see
[the guard work report](../LOADED_STUB_GUARDS.md).

| Credit | Stub | Target | Call sites |
| --- | --- | --- | --- |
| `OVL_005E_32E6` | `03E0:005C` | unit `0x3C6` entry 12 `+0BE7` | `+32F2` |
| `OVL_005E_32E6` | `03E0:0061` | unit `0x3C6` entry 13 `+0C0F` | `+32FE` |
| `OVL_005E_32E6` | `03E0:0066` | unit `0x3C6` entry 14 `+0C37` | `+3311` |
| `OVL_005E_32E6` | `03DB:003E` | unit `0x3C2` entry 6 `+0909` | `+331F` |

Stubs were resolved from the original bytes: each `LCALL` segment is matched to its unit's `memory_segment`, and
the slot `0x20 + 5 * entry` gives the entry. SYSTEM (`0AF8`) calls are resident and need no stub.

## Identity and provenance

- Target: OVL_005E_32E6, GAME.OVR U5E [32E6,3389), file
  [15AE1,15B84): 163 bytes, 62 instruction starts, RETF 6.
- Body SHA-256: 456ced130d112bf573dafb528d31b76604c455891fa25622da2d4cfba1568772.
- START.EXE SHA-256: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- GAME.OVR SHA-256: 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- No public entry stub. Aligned PUSH CS / near CALL sites in U5E +3558
  are +35E6, +36EF, and +3736. Five outgoing relocation words are
  +32F5/+3301/+3314/+3322/+3379. The audit asserts these identities.
- This is the rank-2 composable hard target in the schema-4 workflow's
  current schema-8 queue. The queue marks the three slot getters strict,
  but their public slot API is semantic only. This change folds their raw
  CPU behavior locally; it does not infer a raw API from a readiness label.

Before implementation, local recall query deja "32E6" surfaced the old
#639 U5E reservations in session a3ad1536-abb and Codex histories
01a147c2-329/01a13101-709. Only the ownership warning was reused. VP message
i17741 confirmed Claude had no active/unpublished branch, stash, scratch,
or PR for this body. No old unpublished implementation was imported.

Current interface update: the raw slot engine described below is now shared
through [the U3C6 getter CPU API](U3C6_SLOT_GETTERS_CPU.md). The +32E6 caller
uses that API with the same per-call budget and status behavior. This
extraction adds no identity credit.

## Implementation and observable contract

The public function pr_u5e_caller_32e6 takes CPU14, exact 1 MiB A20-off
RAM, explicit loaded code segments, and five positive budgets. Its three
word arguments after the far-return pair are a parent-frame offset and a
slot pointer (offset, segment). The complete fixed caller body runs in
original instruction order; all stack, table, record, and return reads
remain live.

The caller obtains the slot's three bytes through U3C6 +0BE7/+0C0F/+0C37.
It calls the complete U3C2 +0909 with those origin/shape fields, radius 127,
direction 255, and the far map pointer at DS:6716/6718. It stores DS:671A's
result count at SS:parent-DA. For each one-based index i it copies three
bytes from DS:6718+3*i to SS:parent-DB+3*i using original forward SYSTEM
Move semantics. The byte between count and first record is left alone.
The parent argument is reloaded for every record, so output can alter
subsequent destinations, saved BP, and the final return address.

Composition reuses the accepted U3C2 +0909 CPU API (PR #964) and SYSTEM
Move +0A3F. The raw slot composition follows U5E +1BF8 / +263D's accepted
lookup pattern, with all four original bodies represented explicitly,
including the third getter. Arithmetic/memory primitives are adapted from
U3C2 +0909. No callback supplies a callee's effects. The original-chain
closure contains 14 bodies, 3,534 bytes, and 1,428 instruction starts.

The three getters are 40 bytes/17 instructions each, and lookup +0C5F is
90 bytes/32 instructions. SHA-256 pins:

| Body | SHA-256 |
| --- | --- |
| U3C6 +0BE7 | eb48355d64c585ee95a31739074ede53929cc991ba519157009c7c8e1c9b7ed5 |
| U3C6 +0C0F | a1000b2f6a29a0d74f37a9b631b26844044f2c1a163b2de793672cbc0664395d |
| U3C6 +0C37 | 71c5655d5b32a4688be0ab98761c4b5d2cfb4cc117056648ab92a552ffc06e48 |
| U3C6 +0C5F | c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41 |

Initial IP must be 32E6 and SP must be [00A4,FFF6]. The lower bound leaves
+0909's minimum 008E frame after the caller's 22-byte descent. Caller CPU,
layout, and limits objects must be separate from RAM. Code and supplied
bindings must be immutable and consistent; all writes must avoid modeled
code. Arbitrary data/stack aliases are allowed, subject to dependency
contracts and nonwrapping return-frame preconditions. Nested sorter Move
bindings are read from loaded code exactly as the accepted sorter does.

Each getter has a fresh slot budget, counted at +0C71; the lookup increments
its byte index before testing count, so zero count can traverse all 256
slots. The chain/line/sort budgets retain +0909's exact meanings. The copy
budget counts +3349 visits and exposes the live partial frame. Limits do
not invent a return. Direct unexpected callee returns produce TRANSFER;
nested dependency rejection/changed-return states produce CALLEE. A final
caller RETF whose frame was overwritten remains an ordinary completed
return with the modified CS:IP. No FLAGS are masked; intermediate SHL AF
uses the existing normal-core 386 convention and exposed stop points are
compared without substitutions.

## Reproduction

Owner-supplied media stays under ignored resources/DOS/POOLRAD. Install
Capstone and Unicorn in the Python search path. Run:

    python3 tools/overlay/u5e_caller_32e6_audit.py --output resources/u5e-32e6/audit.json
    python3 tools/overlay/u5e_caller_32e6_native.py
    cmake -S . -B resources/u5e-32e6/build -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/u5e-32e6/build --target poolrad_overlay_u5e_caller_32e6_tests
    ctest --test-dir resources/u5e-32e6/build -R '^poolrad_overlay_u5e_caller_32e6_tests$' --output-on-failure

The original-code audit loads every original body and applies only the
explicit far-call bindings. It compares all CPU14 words, all 1,048,576 RAM
bytes, and the stop reason. An unexpected PC, code write, or instruction
ceiling fails the audit. Native captures execute original bytes under
DOSBox-X normal core / 386, with interrupts disabled during the call,
and compare CPU14, all 65,536 DS bytes, and 1,888 SS bytes spanning the
caller frame, parent output, and low/high offset boundaries. Native tests
are synthetic direct calls, not measurements of a saved gameplay route.

## Measured results

- **440 original-code cases**: CPU14 plus all 1,048,576 RAM bytes,
  including 415 completed returns, one slot-budget stop, 15 chain-budget
  stops, three line-budget stops, three sort-budget stops, two copy-budget
  stops, and one changed direct-callee return. No FLAGS masks or emulator
  state substitutions. The 436-case full run and four subsequently added
  lookup boundaries are retained separately and combined in the receipt.
- Caller coverage: **62/62 instruction sites, 4/4 conditional outcomes**.
  Full closure: **1387/1428 sites, 177/206 conditional outcomes**.
  The receipt lists every uncovered transitive site and edge; these are
  not promoted to full-chain coverage. No CALLEE status case was measured.
- Cases cover every shape byte, zero/missing/last slot and byte-count wrap,
  zero/nonzero output, coordinate boundaries, blocked maps, frame endpoints,
  relocated/A20 code and stacks, data/stack aliases, modified final return,
  and all five positive-budget stops. A parent-output alias overwrites the
  parent argument itself: the second record follows the newly loaded value.
- **27 native full returns**: CPU14 plus 67,424 compared memory bytes per
  case (65,536 DS + 1,888 SS), without masks. Captures include all four
  permitted SP endpoints, output-offset wrap, zero count with missing and
  wrapping lookup matches, and varied initial arithmetic/DF flags.
- Strict C11 with -Wall/-Wextra/-Wpedantic/-Werror and ASan+UBSan:
  **56 semantic checks passed**. Focused CMake/CTest: **1/1 passed**.
- Tool versions: Python 3.14.4, Capstone 5.0.7, Unicorn 2.1.4,
  Ubuntu cc 15.2.0. DOSBox-X executable SHA-256:
  99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
- Aggregate native capture SHA-256:
  97a2f92b192f4f2d2955b51f99569f32e821e6c1203b83aebe1e94067716efbf.
- Ordered original closure SHA-256 (kind/start order recorded in receipt):
  6dcbd52dc416a6c5b8f3baa4a99125d27923261dddb3d417a7a0e004efcf7853.
- Private evidence receipt: resources/u5e-32e6/evidence.json, SHA-256
  **fad1c810838a8e117cbba7168e8e12487fa74efc58ddb83c620d2418fb5039f2**. It includes body/source/dependency/report hashes, individual
  native capture hashes, status counts, uncovered sites/edges, and limitations.
  Integration base: b13c6a79757d50447d274aadf28df37ee3adf990.

## Author review

- target_function: OVL_005E_32E6, complete [32E6,3389).
- source_candidates: overlay_u5e_caller_32e6.c plus the existing +0909
  chain and SYSTEM runtime source dependencies listed by the audit.
- signals: exact body hash/bounds, three aligned incoming callers, all five
  outgoing fixups, full caller instruction/branch coverage, CPU14 plus
  whole-RAM original-code comparisons, and separate native original-chain
  captures. Negative limits and output/frame aliases are tested.
- score: high identity and within-contract parity evidence; incomplete
  transitive branch coverage and no saved-route evidence.
- verdict: likely_match, pending independent review.
- review_notes: no self-credit or acceptance/accounting edits. The queue's
  semantic slot interfaces required explicit raw composition. Native
  execution does not establish live gameplay reachability, arbitrary
  self-modifying code, or wrapping far-return frames. Dependency bodies
  retain their own measured contracts and unresolved coverage edges.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven in a worktree WITHOUT `resources/` (what CI has) with

```sh
python3 tools/overlay/check_checker_builds.py OVL_005E_32E6 --require-current-main
```
