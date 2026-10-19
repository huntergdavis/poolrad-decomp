# U38 +2B7D through the raw target selector

## Result and provenance

The new `pr_u38_target_reset_2b7d_cpu` entry executes the complete wrapper
through merged raw +101D and RES_00BA_2E8B. It extends the existing source
and header, leaving the callback API and its source body unchanged.
A selected target with a nonzero +32 byte blocks the reset. Otherwise,
the routine reloads the active character pointer and clears DS:6818 if
signed character byte +2E is positive or signed byte +73 is less than four.

Before implementation, shared memory was queried with
`dunkingsheep memory context "poolrad U38 2B7D raw target reset ownership"`.
The returned Codex session `01a1386e-bea7-7883-8ddd-06fd2be8db9b` identifies
the prior callback recovery. Its [legacy evidence](U38_TARGET_RESET_2B7D.md)
provided identity and behavior hypotheses rechecked against original bytes.
Historical branch `codex/fullc-u38-2b7d` at
`302044629cdf4a936efd355a474917ad6734349f` was inspected as prior work,
not as an active competing implementation.

The raw selector contracts reuse [PR #1102](https://github.com/huntergdavis/poolrad-decomp/pull/1102)
and [its evidence](U38_TARGET_SELECTOR_CPU.md). The actual-chain harness
structure reuses [PR #1105](https://github.com/huntergdavis/poolrad-decomp/pull/1105).
VP message `i18604` confirms ownership and authoring scope. Fresh schema-8
queue metadata (successor to schema 4) at main `22ff6530` ranks +2B7D
**395, score 18, composable**, with +101D `modeled`. Higher sibling +2A87
was already authored in #1106 and is not a dependency of this work. Existing
callback callers are not auto-converted. No accounting, classification,
acceptance, or credit is changed here.

## Complete original closure

| Artifact / routine | Extent | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| START.EXE | complete file | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | complete file | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U38 +2B7D | [2B7D,2BC8), file [117D8,11823) | 75 / 25 | `8cdf5fe20286cd39394d2b2e5ed4dffa01a318fb10a62b35548bb19ff9a0d323` |
| U38 +101D | [101D,1086), file [FC78,FCE1) | 105 / 36 | `632b609ad14b0fff262cd578a81e144d5dbeab6418ee8ef68303ff9e60cea33a` |
| RES_00BA +2E8B | [2E8B,2F57), expanded image [3A2B,3AF7) | 204 / 69 | `c7aa0a429219bbc7e162996a4af3e1a8e1c95bad8b847edcd0244e42ed54af67` |

The closure is **384 bytes / 130 instructions**. The wrapper's ledger
extent has no trailing data. PUSH CS +2B8B and near CALL +2B8C enter +101D
with relative operand E48Eh, returning at +2B8F. +101D's FAR CALL +1045
uses the loaded resident segment operand at +1048. The resident is call-free
and returns with RETF 8; +101D uses RETF 4; +2B7D uses RETF 0Ah at +2BC5.
Identity remains **game_evidenced / provisional**. No natural gameplay
caller or source-level game name is established by this work.

## Live state and contract

The routine passes DS:5D92/5D94 to the selector, then stores returned DX:AX
in stack locals. A nonnull selected target with byte +32 nonzero returns
without resetting DS:6818. Otherwise, LES +2BA7 reloads DS:5D92/5D94 and
CMP/JG tests signed byte +2E against zero. The remaining path reloads that
pointer again at +2BB2 and CMP/JGE tests signed byte +73 against four.
The raw model reproduces subtraction flags and signed branches directly;
bytes 80h..FFh are negative, not large positive values.

The pointer reloads matter when stack locals alias the globals. A measured
fixture uses SS=DS and entry SP5D98, placing caller locals at DS:5D92/5D94.
The original active character has kind 0 and value 4, which would preserve
the amount. The selector returns the alternate record at 3000:4000; storing
that result replaces the active pointer. The late reload sees its kind 11
and clears DS:6818. C and original execution retain that live alias behavior.

The API accepts CPU14 (AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS), exact
1 MiB A20-off flat RAM, and optional diagnostic. Entry IP must be 2B7D and
SP must be 002E..FFFC. Ten caller argument bytes are unused. Effective
offsets wrap at 16 bits; words/LES read physically contiguous bytes. Reached
far-return frames must fit within SS:0000..FFFF. Code must be immutable and
mapped; host CPU, diagnostic, and RAM storage must be separate. There is no
MMIO, interrupt, fault, or self-modification model and no execution cap in
this finite closure.

Invalid host arguments reject atomically, including an unchanged diagnostic.
Valid entry initializes diagnostic fields to -1. The near-CALL guard checks
opcode/target before pushing the return IP, preserving preceding pushes on
CODE. Nested selector refusal returns CALLEE with the exact partial CPU/RAM
and dependency status. Unexpected dependency returns are guarded as TRANSFER.
Direct CODE and nested CALLEE(CODE) are measured; TRANSFER and nested ARGUMENT
remain defensive, unobserved paths. No final SHR occurs in this wrapper, so
no caller-selected undefined-AF policy is needed. No comparisons mask flags
or RAM.

## Author measurements

| Check | Result | Compared state |
| --- | --- | --- |
| Original-byte Unicorn | **1,064 cases:** 1,060 returns, 2 CODE, 2 CALLEE(CODE) | CPU14, all 1,048,576 RAM bytes, nested diagnostics; unmasked |
| Closure coverage | **130/130 starts, 27/28 conditional outcomes** | all 25 wrapper starts and all 8 wrapper outcomes |
| DOSBox-X normal/386 | **128 synthetic returns** | CPU14, 1,024 SS + 65,536 DS + 65,536 heap bytes; unmasked |
| Strict C11 + ASan/UBSan | **26,678 assertions** | signed boundaries, target guard, selector precedence, late pointer alias, stack cleanup, refusals, atomic arguments |
| Focused CTest | **3/3 passed** | new raw entry, existing callback entry, raw selector suite |

Original-byte cases cover all 256 selector flags, all 256 values of each
signed input and target guard byte, null/fallback/alternate targets, initial
flags and amounts, odd/boundary SPs, offset-only/segment-only pointers,
wrapped effective offsets, relocated resident code, A20 aliases, SS=DS,
target/local aliases, character links overlapping stack records, the late
active-pointer reload, changed call opcodes/targets, and random background.

Resident +2EBE -> +2F38 remains unobserved in this composed run. +101D
normally skips the resident call for a null link; this explains ordinary
nonalias cases but is not a general proof under arbitrary permitted aliases.
Prior standalone resident evidence exercised that outcome; it is not added
to the coverage of these 1,064 cases.

The native harness loads original U38 code at 5000 and actual resident code
at 7000, patches only the declared FAR CALL segment operand +1048, and invokes
+2B7D from a generated COM program at segment 0814. No selector stub is used.
Native evidence does not establish gameplay, refusal/TRANSFER, A20 aliases,
wrapped frames, or memory outside the captured 132,096 RAM bytes. The special
late-pointer alias is covered by original-byte and semantic tests, not by
these native captures.

Native RESULT.BIN SHA-256:
`4759e911d0173600fc8bffe7b0d3e16b3de921b9853d572dfceee0a9ae74a3e4`.
Media, generated original-bearing assembly/COM, input/capture files, logs,
queue/history evidence, and the hash receipt remain ignored under
`resources/`. The receipt pins tested/integration bases, public files,
unchanged compiled/imported dependencies, private artifacts, media, and tools.

## Reproduction

With owner-supplied media in `resources/DOS/POOLRAD/`, Capstone/Unicorn,
a C compiler, GNU as/ld, and DOSBox-X:

```sh
PYTHONPATH=resources/pythondeps:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/u38_target_reset_2b7d_cpu_audit.py \
  --unicorn --native --output resources/oracles.json
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 \
  -fsanitize=address,undefined -fno-omit-frame-pointer -I include \
  tests/recovered/test_u38_target_reset_2b7d_cpu.c \
  src/recovered/overlay_u38_target_reset_2b7d.c \
  src/recovered/overlay_u38_target_select_101d.c \
  src/recovered/resident_character_target_select.c -o resources/semantic
ASAN_OPTIONS=detect_leaks=1 resources/semantic
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target test_u38_target_reset_2b7d_cpu \
  poolrad_overlay_u38_target_reset_2b7d_tests test_u38_target_selector_cpu -j4
ctest --test-dir resources/build \
  -R '^(u38_target_reset_2b7d_cpu|poolrad_overlay_u38_target_reset_2b7d_tests|u38_target_selector_cpu)$' \
  --output-on-failure
```

## Adversarial assessment

- **target_function:** OVL_0038_2B7D, complete [2B7D,2BC8).
- **source_candidates:** extended `overlay_u38_target_reset_2b7d.c` with
  unchanged raw +101D/resident selector sources; legacy callback entry remains
  a separate API.
- **signals:** pinned body hashes, CALL operands/fixup, complete branch shape,
  signed flag behavior, live pointer reloads, stack cleanup, original full
  CPU/RAM comparison, and actual native call-chain capture.
- **score:** 5/5 author evidence categories supported (identity, control flow,
  side effects, original comparison, native comparison); not acceptance credit
  or a probability of correctness.
- **verdict:** **likely_match**, scoped to the documented profile.
- **review_notes:** one composed conditional outcome and defensive statuses
  remain unobserved; native memory scope is bounded and excludes the special
  alias fixture; identity is provisional. Independent verification and any
  credit remain with the reviewer/VP. No self-credit.
