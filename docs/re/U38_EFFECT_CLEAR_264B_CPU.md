# U38 +264B through the raw target selector

## Result and provenance

The new `pr_u38_effect_clear_264b_cpu` entry composes the actual raw
`pr_u38_target_select_101d_cpu` and `pr_res_target_select_2e8b_cpu` APIs
introduced in [PR #1102](https://github.com/huntergdavis/poolrad-decomp/pull/1102).
It extends the existing +264B source/header; the legacy callback API and its
source body are unchanged. The implementation does not substitute a selector
result or native stub. Other callback callers are not automatically converted.

Shared history was queried before implementation with
`dunkingsheep memory context "poolrad U38 +264B target selector raw CPU ownership queue"`.
The returned Codex session `01a1386e-bea7-7883-8ddd-06fd2be8db9b` identifies
the earlier callback recovery. Its [legacy evidence](U38_EFFECT_CLEAR_264B.md)
provides the entry identity and behavior hypotheses, which were rechecked
against original instructions. The old `codex/fullc-u38-264b` branch at
`a4e2b22c1e4d17da82df5838c4c503db47eaad9a` was inspected as historical work;
it is not a second active implementation. The raw selector contracts and
harness structure reuse [U38_TARGET_SELECTOR_CPU.md](U38_TARGET_SELECTOR_CPU.md).

VP messages `i18575` and `i18577` confirm ownership and authorize this raw
composition. On tested main `84676cce093889b683ef910fb470a172f137356b`, the
fresh schema-8 successor to the schema-4 queue labels +264B **rank 436,
score 8**, with +101D `modeled_blocked`. That is a metadata limitation:
`RAW_CPU_INTERFACE_CAVEATS` retains the resident selector exclusion to flag
legacy callback callers, propagating it even through the merged raw +101D API.
Source inspection establishes actual composability here. The exact queue
snapshot is retained privately; the reviewer owns the metadata correction.
No classification, accounting, acceptance, or credit is changed by this PR.

## Original spans and behavior

| Artifact / routine | Extent | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| START.EXE | complete file | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | complete file | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U38 +264B | offsets [264B,268A), file [112A6,112E5) | 63 / 22 | `2350f9f299e129564727b0861075847546ca9ca91d1ae6e71f7c17e45209a6fb` |
| U38 +101D | offsets [101D,1086), file [FC78,FCE1) | 105 / 36 | `632b609ad14b0fff262cd578a81e144d5dbeab6418ee8ef68303ff9e60cea33a` |
| RES_00BA +2E8B | offsets [2E8B,2F57), expanded image [3A2B,3AF7) | 204 / 69 | `c7aa0a429219bbc7e162996a4af3e1a8e1c95bad8b847edcd0244e42ed54af67` |

The complete closure is **372 bytes / 127 instructions**. +264B ends at
RETF 0Ah at +2687; its ledger extent contains no trailing data. PUSH CS at
+2659 and near CALL +265A enter +101D, returning at +265D. The near relative
operand is E9C0h. +101D's FAR CALL +1045 enters the resident +2E8B body;
its live segment operand at +1048 is loader-relocated. The resident body has
no calls and returns with RETF 8. +101D returns with RETF 4.

The wrapper passes DS:5D92/5D94 to +101D. That selector uses the character's
+CC/CE link, the DS:5590 table, and the +F8/FA and +FC/FE alternative pointers;
its raw resident dependency implements selection precedence and +101D's
null-output fallback. +264B stores the selected DX:AX pointer in stack locals,
then clears DS:6818 when the pointer is null or when target byte +32 is zero
and target byte +31 is not B1h. Otherwise it preserves DS:6818. It retains
the original register, flag, stack, and memory effects in instruction order.

The entry remains **game_evidenced / provisional**. The existing item-96
installer reference supports a game-effect route; this work does not reach
that dispatcher from gameplay or establish a source-level game identity.

## API boundary

The raw entry accepts CPU14 (AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS),
an exact 1 MiB flat RAM image, and an optional diagnostic. It requires entry
IP264B and SP002E..FFFC. Ten caller argument bytes are unused. Effective
addresses wrap at 16 bits; words and LES operands read physically contiguous
bytes with A20 off. All reached far-return frames must fit within the stack
segment. The model uses immutable mapped original code, no MMIO, interrupts,
faults, or self-modification, and separate CPU, diagnostic, and RAM storage.
There are no execution caps or unmodeled callbacks in this finite closure.

Invalid host arguments return ARGUMENT atomically, including an unchanged
diagnostic. At a valid entry, diagnostic fields start at -1. The wrapper
checks the live near-CALL opcode/target before pushing its return IP; preceding
pushes remain visible on CODE. The raw +101D implementation checks its own
FAR CALL binding. Nested refusal returns CALLEE and retains the dependency
status, CPU and RAM. Unexpected dependency returns are guarded as TRANSFER.
The tests measure direct CODE and nested CALLEE(CODE); TRANSFER and nested
ARGUMENT remain defensive, unobserved paths. No flag bits are masked. This
wrapper has no final SHR and needs no caller-selected undefined-AF policy;
its comparisons overwrite the dependencies' intermediate arithmetic flags.

## Measured author evidence

| Check | Result | Compared state |
| --- | --- | --- |
| Original-byte Unicorn | **806 cases**: 802 returns, 2 CODE, 2 CALLEE(CODE) | CPU14 and all 1,048,576 RAM bytes, unmasked; nested diagnostics |
| Closure coverage | **127/127 starts**, **25/26 conditional outcomes** | all 22 wrapper starts and all 6 wrapper outcomes included |
| DOSBox-X normal/386 | **128 synthetic returns** | CPU14 plus 1,024 SS + 65,536 DS + 65,536 heap bytes, unmasked |
| Strict C11 + ASan/UBSan | **25,394 assertions** | selector precedence, per-record decisions, byte comparisons, stack cleanup, refusals, atomic invalid arguments |
| Focused CTest | **3/3 passed** | new raw entry, existing callback +264B, existing raw selector suite |

Original-byte cases cover all 256 selector flag values, all 256 values of
each compared target byte, null/fallback/alternative selection, initial flags,
amounts, SP bounds and odd SPs, offset-only/segment-only pointers, relocated
resident code, wrapped effective offsets, A20 aliases, SS=DS, target/local
aliases, character links overlapping stack records, changed CALL opcodes and
targets, and a deterministic random background.

The unobserved outcome is resident +2EBE -> +2F38 (null saved link). +101D
normally skips the resident call for a null character link. This is an
explanation for the ordinary nonalias cases, not a proof of unreachability
under every allowed alias. The prior standalone resident suite exercised
that outcome; it is not counted as covered by these 806 composed cases.

The native harness loads the original U38 prefix at 5000 and actual resident
prefix at 7000, patches only the declared FAR CALL segment operand at +1048,
and calls +264B from a generated COM program. It captures normal return at
COM segment 0814. It uses no synthetic selector stub. Native coverage does
not establish gameplay reach, refusal/TRANSFER behavior, A20 aliases, wrapped
frames, or untouched RAM outside the captured 132,096 bytes.

Native RESULT.BIN SHA-256:
`a719ed4f79cb0c620e94a15f4a5831a29e01808fdfed5ec477e55e4e499e5da9`.
Media, generated original-bearing COM/assembly, inputs, captures, logs,
queue snapshot, and the hash receipt remain under ignored `resources/`.
The receipt pins tested/integration bases, public files, compiled/imported
dependencies, private artifacts, media, and tool binaries.

## Reproduction

With owner-supplied media in ignored `resources/DOS/POOLRAD/`, Capstone and
Unicorn available to Python, GNU as/ld, a C compiler, and DOSBox-X:

```sh
PYTHONPATH=resources/pythondeps:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/u38_effect_clear_264b_cpu_audit.py \
  --unicorn --native --output resources/oracles.json
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 \
  -fsanitize=address,undefined -fno-omit-frame-pointer -I include \
  tests/recovered/test_u38_effect_clear_264b_cpu.c \
  src/recovered/overlay_u38_effect_clear_264b.c \
  src/recovered/overlay_u38_target_select_101d.c \
  src/recovered/resident_character_target_select.c -o resources/semantic
ASAN_OPTIONS=detect_leaks=1 resources/semantic
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target test_u38_effect_clear_264b_cpu \
  poolrad_overlay_u38_effect_clear_264b_tests test_u38_target_selector_cpu -j4
ctest --test-dir resources/build \
  -R '^(u38_effect_clear_264b_cpu|poolrad_overlay_u38_effect_clear_264b_tests|u38_target_selector_cpu)$' \
  --output-on-failure
```

## Adversarial parity assessment

- **target_function:** OVL_0038_264B, complete [264B,268A).
- **source_candidates:** extended `overlay_u38_effect_clear_264b.c`, composed
  with unchanged raw `overlay_u38_target_select_101d.c` and
  `resident_character_target_select.c`; legacy callback entry is a separate API.
- **signals:** pinned body hashes, call operands and fixup, complete control-flow
  shape, two byte comparisons and clear store, source-ordered aliases, stack
  cleanup, original CPU/full-RAM comparison, and native actual-chain capture.
- **score:** 5/5 author evidence categories supported (identity, control flow,
  side effects, original-byte comparison, native comparison); not an acceptance
  score or probability of correctness.
- **verdict:** **likely_match**, scoped to the documented flat-RAM profile.
- **review_notes:** one composed conditional outcome unobserved; native memory
  scope is bounded; TRANSFER/nested ARGUMENT unexercised; provisional identity;
  queue metadata lags the actual raw ABI. Independent verification and credit
  remain with the reviewer/VP. No self-credit.
