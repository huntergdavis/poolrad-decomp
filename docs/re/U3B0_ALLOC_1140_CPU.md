# U3B0 +1140: raw GetMem composition upgrade

This upgrades the existing [entry-6 callback model](U3B0_ENTRY6_FULLSTATE_1140.md)
with `pr_u3b0_entry6_1140_cpu`. It preserves that API and the original caller
implementation, and directly composes the merged
[`pr_system_getmem_023f`](SYSTEM_GETMEM_023F.md). It is not a new identity.
The VP/reviewer owns independent acceptance and credit.

## Identity and selection

VP approval i18885 requires the upgrade framing, executable-span proof,
callback compatibility, and explicit allocator failure/budget evidence.
The regenerated schema-8 successor to the schema-4 queue on
`836742eb` selects `OVL_03B0_1140` at rank **444**, score **1**,
`main_model`, `game_evidenced`, composable with runtime GetMem covered.
The saved older row was rank 443/score 6. The target did not change.
Higher-ranked composable callers already expose raw entries or are inlined
in strict implementations; leaf/data rows are not new caller-composition work.
Queue readiness is not an executable-span or raw-interface proof.

The original GAME.OVR span is **[0x2DA0F,0x2DAC2)**, unit
**[0x1140,0x11F3)**: **179 executable bytes / 58 instructions**.
SHA-256:
`fdced3a4a7f0c069d55a9a4fef8a048eca70e649ec66559b84240259d2280c5c`.
The final instruction is the three-byte RETF 6 at +11F0; its final byte is
+11F2. The next +11F3 starts PUSH BP; MOV BP,SP. **There is no trailing
DATA or padding.** Both +114A/+11A8 far-call segment operands have original
fixup entries.

The media are the owner-supplied DOS build, kept under ignored resources:
GAME.OVR (206105 bytes), SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
START.EXE (65231 bytes), SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

The measured static closure is **425 bytes / 165 instructions**:
179/58 caller plus these unchanged SYSTEM bodies in the expanded image:

| SYSTEM half-open span | Bytes / instructions | SHA-256 |
| --- | --- | --- |
| [023F,0254) | 21 / 9 | `d4ceab56ec05612e8ba6c90dab11d9da62f1f38235503781f80d636daa63d346` |
| [0308,03D3) | 203 / 88 | `03e08d678db922d962aafbd79b301ca90556775acef11b832fecbeb29fe0dff0` |
| [0491,04A2) | 17 / 8 | `440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d` |
| [00A9,00AE) | 5 / 2 | `702a44410b4b3c0c9df438b32a641b3203e9046ef66052fef66163bebdfa12b7` |

## Contract

The caller always requests a 46-byte head, then additional nodes for a signed
count of at least two. Counts at most one, including negative counts, still
request the head. Each node clears byte 0, byte 29h and its next far pointer
at 2Ah/2Ch. Other payload bytes remain untouched. The result is stored through
the caller's live far output pointer; subsequent reads preserve alias effects.

The raw entry uses exactly 1 MiB of ordinary RAM, A20 off, immutable code,
and entry SP in [0040,FFF6] so the active stack-offset frames do not wrap.
The live call segment is read after validating opcode 9A and target 023F.
The adapter pushes the real far-return frame, copies all 14 CPU fields into
the unchanged allocator, and retains its CPU/RAM result. On success only,
it compensates the legacy callback's pending two-byte argument cleanup;
the caller immediately restores those two bytes without a memory access.
The legacy body and public callback declarations remain unchanged.

**Stack-top RETF wrap: unreachable inside this window (reviewer-added 2026-10-20, VP i19693).** With entry SP E in [0040,FFF6], SYSTEM GetMem `0AF8:023F` (called at `+114A` and `+11A8`) is entered at E−10h (`PUSH BP`, `SUB SP,8`, the size word and the far return), i.e. in [0030,FFE6]. Its own far return executes at the entry SP, because the frame is balanced, so at most FFF6. None of these ranges reaches FFFE/FFFF. That is the only band where DOSBox-X wraps a RETF's CS word within SS while the merged SYSTEM models and Unicorn read it contiguous ([REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#retf-stack-top-wrap-a-recorded-class-deliberately-not-swept-2026-10-20)). The window, not a measurement, keeps the band out of reach, and a caller outside the window is outside the contract. This row is a model, not a strict credit; the window is recorded for completeness.

Stops are explicit:

- CODE (2): invalid call opcode/target, before pushing the far frame.
- CALLEE (3): GetMem's non-returning state, including RunError at +00E2,
  custom HeapError at +03B7/+03C3, and free-list walk LIMIT at +0319.
  The diagnostic retains the allocator status; no cleanup or return is invented.
- TRANSFER (4): GetMem returned to an unexpected CS:IP; defensive, unmeasured.
- LIMIT (5): the allocation-call budget is exhausted at the next LCALL,
  with size already pushed and the far frame not pushed. Guard checks precede
  the budget. The free-list budget resets for every allocator invocation.

The diagnostic starts at -1 and otherwise reports the last allocator
invocation, including an earlier success when a later guard/budget stops.
ARGUMENT (1) leaves CPU/RAM unchanged; a NULL diagnostic is supported.
The final RETF 6 consumes the live caller frame, even if the output aliases it.
General stack/heap/code aliases and asynchronous modification are not claimed.

## Evidence

Run the new audit with the owner media available:

```sh
python tools/overlay/u3b0_alloc_1140_cpu_audit.py --unicorn --native --output resources/oracles.json
```

The harness reuses the hash-checked SYSTEM extraction/heap fixtures and the
U25 +1951 native capture pattern. Original execution determines the stop,
independently of C. Native fixture selection is predetermined, without
filtering cases through the candidate C implementation.

- **280 original-byte cases:** 216 returns, 50 allocator stops (47 RunError,
  two custom HeapError sites, one cyclic-list limit), seven allocation limits,
  seven call guards. All **14 CPU words and the entire 1 MiB RAM** compare
  without masking. This includes failure at calls one/two/three, per-call
  walk-budget reset, signed counts, exact/split/bump allocation, randomized
  heaps/backgrounds, low/high SP, A20 stack wrapping, DS=SS, output at FFFF,
  output aliasing the caller return frame, and output aliasing FreeList.
- **58/58 caller starts and 4/4 conditional outcomes** observed; 2013
  allocator calls executed. Closure: **159/165 starts, 23/28 outcomes**.
- **64 DOSBox-X normal-core/386 cases:** 48 returns and 16 RunError stops,
  including failures after zero/one/two successful allocations. All CPU14,
  the 256-byte active-stack window, the full 64 KiB DS and 16 KiB heap arena
  compare without masking. SYSTEM is relocated to 7000, caller to 8800,
  COM segment is 0814. RunError is trapped at its entry by an instrumentation
  jump; its subsequent runtime-error behavior is not executed.
- Native RESULT.BIN SHA-256:
  `7486920ca5c069c0d3985a27a41eadc288e2c07228bd0af367f9d73148e23f44`.
- **69020 ASan/UBSan semantic/diagnostic checks**, including 84 count/flag
  combinations, node links/clears, untouched payload/granule bytes, argument
  errors, guards, allocation budgets and allocator failure prefixes.
- **2/2 focused CTest targets** pass (caller, GetMem); the preserved callback
  original-byte oracle also passes its **21 cases**. This is not a full
  repository CTest claim.

Closure gaps are explicit. SYSTEM +0383/+0385/+0386 are the nil-size path,
unreached by a fixed 2Eh request. +03CB/+03CD/+03D0 require a non-default
HeapError return, where the raw callee deliberately stops instead.
Missing edges are +030A→0383, +0334→0319, +03C9→03CB,
+03CB→0383 and +03CB→03CD. The +0334 unsigned granule-short edge cannot
occur for this request's rounded granule zero in the separated fixtures.
These are not claims of full closure path coverage.

## Reuse and author checker

Dunking Sheep query `Poolrad U3B0 1140 SYSTEM GetMem raw CPU allocator`
returned Claude session `281c8700-5acc-438b-b93d-c061e28437eb` and
reviewer session `01a14c56-d196-777f-aaeb-0eac03c2083d`.
Reuse consists of the preserved caller, merged raw GetMem API/implementation,
SYSTEM extraction and heap setup, and U25 +1951 original/native harness
structure. This audit measures the new composition independently.

- **target_function:** U3B0 [1140,11F3), raw upgrade of the callback model.
- **source_candidates:** raw companion in the existing source/header pair.
- **signals:** original extent/hash/fixups, complete caller start/edge coverage,
  live far frames, CPU/RAM comparisons, native captures, failure prefixes,
  semantic checks and legacy regression.
- **score:** 9/10 author confidence within this profile, not acceptance.
- **verdict:** likely_match within the stated profile, pending independent review.
- **review_notes:** no native HeapError/host-budget/guard capture, full-1MiB
  native dump, non-default HeapError execution, RunError body, MMIO,
  asynchronous interrupts, self-modifying code, arbitrary aliases, wrapping
  active stack-offset frames, or end-to-end gameplay claim. Unexpected
  near/far return TRANSFER paths are defensive and unmeasured. Native
  interrupts are disabled during the measured invocation. No elapsed-time
  claim and no new identity claim.

Private receipts pin public files, transitive dependencies, selection inputs,
test logs and native artifacts. The VP/reviewer decides acceptance and credit.
