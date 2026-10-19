# U38 +2D88 through the raw target selector

## Result and provenance

The new `pr_u38_effect_scale_clear_2d88_cpu` entry composes the complete
wrapper with merged raw +101D and RES_00BA_2E8B. It extends the existing
source and header; the callback API and its source body remain unchanged.
A null target clears DS:6818. A nonnull target with byte +32 nonzero
preserves it. Otherwise, byte +31 equal to B1h halves the unsigned amount,
and other values clear it. Both byte-32 comparisons and their FLAGS effects
are retained in original instruction order.

Before implementation, shared memory was queried with
`dunkingsheep memory context "poolrad U38 2D88 raw effect scale clear ownership"`.
The returned Codex session `01a1386e-bea7-7883-8ddd-06fd2be8db9b` identifies
the earlier callback recovery. Its [legacy evidence](U38_EFFECT_SCALE_CLEAR_2D88.md)
provided entry/behavior hypotheses rechecked against original instructions.
Historical branch `codex/fullc-u38-2d88` at
`5f29c6c6b212b692f7538ecead53593380927f51` was inspected as provenance,
not as an active competing implementation.

This work reuses the raw selector contracts and explicit SHR profiles from
[PR #1102](https://github.com/huntergdavis/poolrad-decomp/pull/1102) and
[its evidence](U38_TARGET_SELECTOR_CPU.md), plus actual-chain harness structure
from [PR #1105](https://github.com/huntergdavis/poolrad-decomp/pull/1105).
VP message `i18626` confirms ownership and scope. The fresh schema-8 queue
(successor to schema 4) at main `838128b4` ranks +2D88 **396, score 18,
composable**, with +101D `modeled`. Higher selector siblings have already
been authored and merged. Other callback callers, including +2AE6, are not
automatically converted. No accounting, classification, or acceptance
changes are included; no self-credit is claimed.

## Complete original closure

| Artifact / routine | Extent | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| START.EXE | complete file | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | complete file | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U38 +2D88 | [2D88,2DE4), file [119E3,11A3F) | 92 / 32 | `cf1d8b0d9a97cc7958605493f70ebdf99288eb78ab49496dbc2ab37ee61290cd` |
| U38 +101D | [101D,1086), file [FC78,FCE1) | 105 / 36 | `632b609ad14b0fff262cd578a81e144d5dbeab6418ee8ef68303ff9e60cea33a` |
| RES_00BA +2E8B | [2E8B,2F57), expanded image [3A2B,3AF7) | 204 / 69 | `c7aa0a429219bbc7e162996a4af3e1a8e1c95bad8b847edcd0244e42ed54af67` |

The complete closure contains **401 bytes / 137 instructions**. The wrapper's
ledger extent has no trailing data. PUSH CS +2D96 and near CALL +2D97 enter
+101D with relative operand E283h and return at +2D9A. +101D's FAR CALL
+1045 reads the resident segment from relocated operand +1048. The resident
body is call-free and returns with RETF 8; +101D uses RETF 4; +2D88 uses
RETF 0Ah at +2DE1.

The wrapper passes the active character at DS:5D92/5D94 through the selector
and stores returned DX:AX in stack locals. Null jumps directly to the clear
store at +2DD9. For a nonnull target, CMP +2DAB checks byte +32; if zero,
CMP +2DB5 checks byte +31 against B1h. Equality takes the amount load,
zero extension, SHR AX,1 at +2DC1, and store at +2DC3. JMP +2DC6 preserves
the shift's returned FLAGS. Other nonnull paths reload the target and repeat
the +32 comparison at +2DCB, then either preserve or clear the amount.
Dropping that comparison would change FLAGS on the clear path.

Identity remains **game_evidenced / provisional**. The earlier installer
reference to dispatcher item 123 supports a game-effect route; this work
does not measure gameplay reaching that item or establish a source-level
game function name.

## Raw API and flags

The raw entry accepts CPU14 (AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS),
exact 1 MiB A20-off RAM, an explicit shift profile, and optional diagnostic.
Entry IP must be 2D88 and SP must be 002E..FFFC. Ten caller argument bytes
are unused. Effective offsets wrap at 16 bits; words/LES read physically
contiguous bytes. Reached far-return frames must fit in SS:0000..FFFF.
Code is immutable and mapped; host CPU, diagnostic, and RAM storage must be
separate. No MMIO, interrupt, fault, or self-modification model is included.
This finite closure has no execution cap or selector callback.

Invalid arguments, including invalid profiles, reject atomically without
changing the diagnostic. Valid entry sets diagnostic fields to -1. The
near-CALL opcode/target guard runs before pushing its return IP, preserving
earlier pushes on CODE. Nested raw-selector refusal returns CALLEE with
exact partial CPU/RAM and dependency status. Unexpected dependency return
targets are guarded as TRANSFER. Direct CODE and CALLEE(CODE) are measured;
TRANSFER and nested ARGUMENT remain defensive, unobserved paths.

The existing `pr_target_shift_profile` enum from the +25BC header is reused
without changing that header or implementation. Unicorn's profile clears
undefined AF at the final SHR; DOSBox-X normal/386's profile sets it. These
are measured emulator profiles, not universal x86 rules. The
[prior SYSTEM flags evidence](SYSTEM_GETMEM_308_SELECTION_GATE.md) documents
the same distinction. All oracle CPU and RAM comparisons are unmasked.

## Author measurements

| Check | Result | Compared state |
| --- | --- | --- |
| Original-byte Unicorn | **1,574 cases:** 1,570 returns, 2 CODE, 2 CALLEE(CODE) | CPU14, all 1,048,576 RAM bytes, nested diagnostics; unmasked |
| Closure coverage | **137/137 starts, 27/28 conditional outcomes** | all 32 wrapper starts and all 8 wrapper outcomes |
| DOSBox-X normal/386 | **128 synthetic returns** | CPU14, 1,024 SS + 65,536 DS + 65,536 heap bytes; unmasked |
| Strict C11 + ASan/UBSan | **33,301 assertions** | selector precedence, final comparison flags, preserve/halve/clear, both profiles, stack cleanup, refusals, atomic arguments |
| Focused CTest | **3/3 passed** | new raw entry, unchanged callback entry, raw selector suite |

Original-byte fixtures cover all 256 selector flag values, all 256 values
of both target bytes, and all 256 amounts under halve, preserve and clear
gates. Other cases include null/fallback/alternate targets, initial flags,
odd and boundary SPs, offset-only/segment-only pointers, wrapped effective
offsets, relocated resident code, A20 aliases, SS=DS, target/local aliases,
character links sharing stack records, changed call opcodes/targets, and
random background.

Resident +2EBE -> +2F38 remains unobserved in this composed run. +101D
normally skips the resident call for a null link; this explains ordinary
nonalias fixtures but is not a general proof under every permitted alias.
The earlier standalone resident evidence covered that outcome; it is not
added to the coverage of these 1,574 cases.

The native harness loads original U38 code at 5000 and actual resident code
at 7000, patches only the declared FAR CALL segment operand +1048, and calls
+2D88 from a generated COM program at segment 0814. There is no selector stub.
Native case 2 starts with FLAGS0002 and amount18, then returns AX0009 and
FLAGS0016. The corresponding Unicorn-profile C fixture returns FLAGS0006;
the receipt asserts both. Non-halving profiles agree. Native evidence does
not establish gameplay, refusal/TRANSFER, A20 aliases, wrapped frames, or
memory outside the captured 132,096 RAM bytes.

Native RESULT.BIN SHA-256:
`92c197aad56855dc0531a065e1d33ee48274538782ed4fcfa25c94d55e459133`.
Media, generated original-bearing assembly/COM, inputs/captures, logs,
queue/history evidence, and the hash receipt remain ignored under
`resources/`. The receipt pins tested/integration bases, public files,
unchanged compiled/imported dependencies, private artifacts, media, and tools.

## Reproduction

With owner-supplied media in `resources/DOS/POOLRAD/`, Capstone/Unicorn,
a C compiler, GNU as/ld, and DOSBox-X:

```sh
PYTHONPATH=resources/pythondeps:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/u38_effect_scale_clear_2d88_cpu_audit.py \
  --unicorn --native --output resources/oracles.json
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 \
  -fsanitize=address,undefined -fno-omit-frame-pointer -I include \
  tests/recovered/test_u38_effect_scale_clear_2d88_cpu.c \
  src/recovered/overlay_u38_effect_scale_clear_2d88.c \
  src/recovered/overlay_u38_target_select_101d.c \
  src/recovered/resident_character_target_select.c -o resources/semantic
ASAN_OPTIONS=detect_leaks=1 resources/semantic
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target test_u38_effect_scale_clear_2d88_cpu \
  poolrad_overlay_u38_effect_scale_clear_2d88_tests test_u38_target_selector_cpu -j4
ctest --test-dir resources/build \
  -R '^(u38_effect_scale_clear_2d88_cpu|poolrad_overlay_u38_effect_scale_clear_2d88_tests|u38_target_selector_cpu)$' \
  --output-on-failure
```

## Adversarial assessment

- **target_function:** OVL_0038_2D88, complete [2D88,2DE4).
- **source_candidates:** extended `overlay_u38_effect_scale_clear_2d88.c`
  with unchanged raw +101D/resident selector sources; the legacy callback
  entry remains a separate API.
- **signals:** pinned hashes, CALL operands/fixup, complete branch shape,
  repeated comparison and final FLAGS, unsigned amount shift, stack cleanup,
  source-ordered aliases, original full CPU/RAM and actual native call chain.
- **score:** 5/5 author evidence categories supported (identity, control flow,
  side effects, original comparison, native comparison); not acceptance credit
  or a probability of correctness.
- **verdict:** **likely_match**, within the documented profile.
- **review_notes:** undefined AF requires an explicit measured profile; one
  composed conditional outcome and defensive statuses remain unobserved;
  native memory coverage is bounded; identity remains provisional. Independent
  verification and any credit remain with the reviewer/VP. No self-credit.
