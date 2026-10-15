# SYSTEM +0308/+0491 selection and explicit flags-profile gate

This addressed adapter executes the allocator selection/normalization prefix
from loaded SYSTEM 131C:0308 to the actual split-tail handoff at +0363.
It also covers separately entered zero-size, exact-fit, growth and
exhaustion prefixes. It stops before +0363/+0376's existing tails,
before wrapper return +0248, and before unknown indirect far calls
+03B7/+03C3. No hook outcome or later snapshot resumes a blocked state.
The caller +19BA stores remain a disjoint task.

The branch starts from merged PR #465 (`446bbdc`). Bounded
`deja "Poolrad SYSTEM 0308 0491 allocator free-list normalization undefined
AF 449A 44AC"` expired without reusable output before implementation.
Reused leads are the pinned wrapper/frame/writer evidence and original
code spans in [the #465 audit](SYSTEM_GETMEM_23F_NATIVE_ABI.md). New native
normalizer checkpoints and independently controlled heap cases recheck
the previously unresolved flags and branch boundaries.

The review branch is restacked onto current main
`e48ebe1208815a205d1e938a84b2b6d534d3e373` after #471. Owned source,
checks and directly reused headers/oracle helpers retain their tested
hashes; the focused audit is rerun on the final restacked head.

## Explicit CPU flags profiles

The adapter requires one of two named profiles; no implicit default exists.
For the original count-one SHR instructions at +0498/+049A/+049C,
DOSBox-X commit 59915c1 sets undefined AF, while Unicorn 2.1.4 clears it.
This is **set AF**, not preservation of incoming AF. A controlled input
with FLAGS=7206 (AF clear) produces 7212 after the first SHR. Subsequent
native FLAGS are 7217 and 7216; AND at +049E produces 7246 and reconverges.
The untouched natural input has AF set and gives the same output sequence.

The pinned native emulator's `src/cpu/flags.cpp`, `get_AF`, confirms its
SHR case derives AF from shift count `& 1F`; count one is nonzero. This
explains the controlled result and rejects the plausible preserve-AF model.
These profiles describe the audited emulators, not a universal x86 AF rule.
Other implemented instruction flags follow the measured original semantics.

The audit executes both C profiles alongside an unmodified raw Unicorn.
Every raw-profile CPU field and RAM byte must match original-byte execution.
Every transient native-profile difference must be the exact expected AF
addition at +049A/+049C/+049E, with all other registers and RAM equal.
Flags are never masked for comparison or rewritten in the oracle. Native
checkpoints compare the native-profile full flags word and all other state
exactly. Cross-profile differences are reported separately from full matches.

## Pinned inputs and separate native provenance

| Input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SAVGAMA.DAT | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original/loaded +0308..03D2 | 203 | `03e08d678db922d962aafbd79b301ca90556775acef11b832fecbeb29fe0dff0` |
| Original/loaded +0491..04A1 | 17 | `440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d` |
| Complete 25-row raw native transcript | — | `b53f000fc0099b6e3bed113b7de83a14d83bb0f9d7cf755d510dd6a5c2e427e0` |

Original SYSTEM bytes come from START file base AC76 plus the listed IP;
both spans match every loaded capture. The ignored capture root is
`resources/oracle-runs/slot-a-getmem-core-abi-20261014`.
All 25 native rows pin sixteen CPU fields and all 655360 RAM bytes.
The first twelve rows form an untouched saved-A run; their stops are
+0308, every +0491..04A1 instruction boundary, +0312, +0363 and caller
+19BA. This run returns 449A:0008, as #465 measured, and stops before
the caller instruction. The new C fragment stops earlier at +0363.

The remaining thirteen rows are explicitly controlled, independent native
cases. Five rows re-enter +0498 with CS/IP/AX/DX/FLAGS edited to test incoming
AF=0, stopping before +04A1's RET. Eight rows are four input/output pairs
with synthetic DS=6000 heap metadata, explicit SS:SP=1D16:37AE and near
return word 0248. They are not natural saved-game reachability evidence.

| Controlled case | Constructed input | Measured stop and effects |
| --- | --- | --- |
| Exact fit | Head 6100:0000, size 0012:0000; request DX:AX=0012:0000 | +0376; four real stack pushes, before predecessor tail |
| Growth prefix | Head/end 6200:0000, limit 6300:0000 | +03B7; end/head advance to 6212:0000, zero argument pushed |
| Exhaustion prefix | Same head/end, limit 6200:0000 | +03C3; requested size 011D pushed, before hook |
| Zero size | +0308 with AX=0 and DX=ABCD | +0248; AX=DX=0, actual near return consumed |

The controlled far hook pointer is 6500:0100 in DS:[4414]. Neither far
call executes. Reports expose this live loaded target, call site and
return PC (+03BB or +03C7), while preserving CPU/RAM and leaving the far
frame unpushed. The growth/exhaustion labels describe prefixes; they do
not establish the hook's semantics, eventual allocation or failure.

## Verification and limits

The natural C fragment executes 61 instructions and matches ten post-input
native full-state checkpoints. Controlled AF-clear execution adds four
native checkpoints; exact-fit/growth/exhaustion/zero prefixes add one each:
**18 native-profile checkpoints** total. None of the synthetic original-byte
cases below adds native reachability evidence.

The focused audit covers twenty helper size boundaries, eight independent
selection/limit cases, 276 boundary/random arithmetic instructions, and
three alias-order cases. The latter check LES operand ordering, a requested
size word aliasing its PUSH destination, and predecessor writes aliasing
heap globals. A cyclic free-list case must fail at its forty-instruction
bound; its comparisons are excluded from the positive total.

The complete audit passes **817 raw-profile full CPU/RAM instruction
comparisons**, 18 native-profile full-state checkpoints and **102 explicitly
retained cross-profile AF differences**. The latter are negative equality
evidence, not additional full matches. Strict C11/Wconversion and standalone
checks pass. AddressSanitizer/UndefinedBehaviorSanitizer pass the strict
checks and the complete 817-comparison audit after oracle reuse; the
120-second process guard remains unchanged.

The runner restricts inputs/builds to this clone's ignored `resources/`.
Each case has a 500-instruction/ten-second bound; the process has a
120-second deadline and five-second kill grace. One Unicorn instance is
fully initialized with all input RAM and registers before each independent
case. It is never corrected or reinitialized within a case. Reusing the
instance avoids repeated emulator allocations under sanitizers. The first
sanitizer attempt exceeded the unchanged process deadline during arithmetic
cases and failed rather than reporting success.

Pinned emulator `flags.cpp` SHA-256 is
`3ea36c4c56278ef14ec0f89ac93966197ed389ce4daaee7afa3d9de39e02d893`.
Its implementation is a supporting behavior reference; original game
bytes, controlled counterexample and native state are independent signals.
No emulator source or copyrighted media is copied into public files.

Run the media-free check with
`bash tests/replay/check_system_getmem_308_gate.sh`.
With pinned captures and Unicorn 2.1.4, run
`POOLRAD_UNICORN_PYTHON=/path/to/python bash tests/replay/check_system_getmem_308_gate.sh resources/oracle-runs/slot-a-getmem-core-abi-20261014`.
Assertions must remain enabled. All prior gates and the immutable #410
document remain unchanged; six new uniquely named files, no CMake edit,
and no key/printer lane overlap.

## Adversarial review and next scopes

- target_function: SYSTEM original AF8:0308 selection and AF8:0491
  normalization, through explicit tail/return/hook boundaries.
- source_candidates: `system_getmem_308_gate.c` addressed adapter with two
  required named CPU profiles.
- signals: original/loaded byte identity; actual near frame and native
  checkpoints; controlled heap-prefix effects; AF-clear counterexample and
  pinned emulator implementation; full raw-profile differential checks.
- score: 5/5 checked categories above, an ordinal evidence score and not a
  probability or whole-core completion score.
- verdict: match for the measured bounded prefixes and explicit profiles;
  full allocator semantics remain unclear.
- review_notes: the rejected preserve-AF assumption is negative evidence;
  synthetic or controlled entry does not prove saved-game reachability.
  Unknown hooks and post-hook +03C7 behavior remain blocked. No full-core,
  continuous saved-A C, symbol identity, playable progress or count credit.

Two disjoint follow-ons remain:

1. Measure the real DS:[4414] hook ABI, actual far frame and native
   post-hook AL/retry/error effects independently, retaining unknown system
   dependencies. Runtime-specific files; no fabricated hook outcomes.
2. Caller +19BA pointer stores and loader setup through +19D7's near CALL
   +0AEC (return +19DA), stopping before the loader. Caller-specific files;
   no heap/core translation or snapshot bridge.
