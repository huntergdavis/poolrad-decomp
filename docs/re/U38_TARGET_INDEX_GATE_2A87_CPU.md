# U38 +2A87 through the raw target selector

## Result and provenance

`pr_u38_target_index_gate_2a87_cpu` executes the complete U38 +2A87 body
through merged raw +101D and RES_00BA_2E8B. It extends the existing source
and header, preserving the callback API and its source body. The new entry
retains both original target-index reads. A nonnull target halves DS:6818
when its indexed table byte is zero or has bit 0 set; all other cases
preserve that byte.

Before implementation, shared memory was queried with
`dunkingsheep memory context "poolrad U38 2A87 2B7D 2D88 next raw selector ownership"`.
The returned Codex session `01a1386e-bea7-7883-8ddd-06fd2be8db9b` identified
the earlier callback recovery. Its [legacy report](U38_TARGET_INDEX_GATE_2A87.md)
provided entry/behavior hypotheses rechecked against the original bytes.
Historical branch `codex/fullc-u38-2a87` at
`9ce2db449e5c0e59bfc1ecf46fb06de479af0830` was inspected, not treated as
an active second implementation.

This work reuses the raw selector contracts and flag profiles from
[PR #1102](https://github.com/huntergdavis/poolrad-decomp/pull/1102) and
[its evidence](U38_TARGET_SELECTOR_CPU.md), plus the actual-chain harness
structure from [PR #1105](https://github.com/huntergdavis/poolrad-decomp/pull/1105).
VP message `i18592` confirms ownership, API scope, and explicit flag-profile
reuse. Fresh schema-8 queue metadata (the schema-4 queue's successor) at
main `22ff6530` ranks +2A87 **394, score 18, composable**, with +101D
`modeled`. The #1104 readiness correction is present. Existing callback
callers are still not auto-converted; the next +2B7D/+2D88/+2AE6 bodies
remain separate work. No accounting, classification, or acceptance changes
are included, and no self-credit is claimed.

## Identity and complete closure

| Artifact / routine | Extent | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| START.EXE | complete file | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | complete file | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U38 +2A87 | [2A87,2AE6), file [116E2,11741) | 95 / 37 | `7e7c8ac21b1334814e21a84a1403ca7964c9397ea0477acde8a989e5d271fc40` |
| U38 +101D | [101D,1086), file [FC78,FCE1) | 105 / 36 | `632b609ad14b0fff262cd578a81e144d5dbeab6418ee8ef68303ff9e60cea33a` |
| RES_00BA +2E8B | [2E8B,2F57), expanded image [3A2B,3AF7) | 204 / 69 | `c7aa0a429219bbc7e162996a4af3e1a8e1c95bad8b847edcd0244e42ed54af67` |

The closure contains **404 bytes / 142 instructions**. The wrapper's ledger
extent has no trailing data. PUSH CS at +2A95 and near CALL +2A96 enter
+101D with relative operand E584h, returning at +2A99. +101D's FAR CALL
+1045 reads its loaded resident segment from the fixup operand at +1048.
The resident body is call-free and returns with RETF 8; +101D uses RETF 4;
+2A87 returns with RETF 0Ah at +2AE3.

The wrapper passes DS:5D92/5D94 into the selector, which resolves the
character's link and alternate pointers with the resident's table gates
and +101D's null-output fallback. +2A87 stores DX:AX into stack locals.
For a nonnull target, it reads target byte +2E, shifts that index left four,
and tests DS:[5589 + 16*index]. A zero byte branches directly to halving.
Otherwise it reloads the target/index, reads the table byte again, and tests
bit 0 before halving. The amount is zero-extended before SHR AX,1 and AL is
stored to DS:6818. Both shifts of DI have their intermediate arithmetic
flags overwritten by CMP or AND/OR before a visible boundary.

Identity remains **game_evidenced / provisional**. The legacy installer
reference to dispatcher item 115 supports a game-effect route, but this
work does not reach that dispatcher from gameplay or establish a recovered
source-level game name.

## Contract and flags

The raw entry accepts CPU14 (AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS),
an exact 1 MiB RAM image, an explicit `pr_target_shift_profile`, and optional
diagnostic. Entry IP must be 2A87 and SP must be 002E..FFFC. Ten caller
argument bytes are unused. Effective offsets wrap at 16 bits; words and
LES operands read physically contiguous bytes with A20 off. Reached far-return
frames must fit in SS:0000..FFFF. Code is immutable and mapped; there is no
MMIO, interrupt, fault, or self-modification model. Host CPU, diagnostic,
and RAM storage must be separate. The finite closure has no execution cap.

Invalid arguments, including an invalid shift profile, reject atomically.
Valid entry initializes both diagnostic fields to -1. The near-CALL guard
checks opcode/target before pushing its return IP, retaining earlier pushes
on CODE. Nested raw-selector refusal returns CALLEE with the original
partial CPU/RAM and dependency status. Unexpected dependency return targets
are guarded as TRANSFER. Direct CODE and nested CALLEE(CODE) are measured;
TRANSFER and nested ARGUMENT are defensive, unobserved paths.

The existing enum from the +25BC header is reused, without changing that
header or implementation. `PR_TARGET_SHIFT_UNICORN` clears undefined AF
at the final SHR; `PR_TARGET_SHIFT_DOSBOX386` sets it. These are measured
emulator profiles, not universal x86 rules. The
[prior SYSTEM flags evidence](SYSTEM_GETMEM_308_SELECTION_GATE.md) explains
the same distinction. The C-vs-Unicorn differential masks nothing; the separate
DOSBox native checker `tests/recovered/native_overlay_u38_target_index_gate_2a87.py`
does mask exactly one bit - AF (`& ~0x0010`) - and only for the SHR modes, because
DOSBox-X sets AF where Unicorn and the C model clear it on this architecturally
undefined bit. Every other CPU word and every RAM byte is compared exactly.
Native case 2 starts with FLAGS=0002 and amount=18, then returns AX=0009,
FLAGS=0016. The Unicorn profile produces FLAGS=0006 for the corresponding
fixture. Non-halving paths agree across the two profiles.

## Author measurements

| Check | Result | Comparison scope |
| --- | --- | --- |
| Original-byte Unicorn | **1,575 cases:** 1,571 returns, 2 CODE, 2 CALLEE(CODE) | CPU14, all 1,048,576 RAM bytes, nested diagnostics; unmasked |
| Closure coverage | **142/142 starts, 25/26 conditional outcomes** | all 37 wrapper starts and all 6 wrapper outcomes |
| DOSBox-X normal/386 | **128 synthetic returns** | CPU14, 1,024 SS + 65,536 DS + 65,536 heap bytes; unmasked |
| Strict C11 + ASan/UBSan | **32,053 assertions** | distinct selected records, gate/amount decisions, both profiles, stack cleanup, refusals, atomic arguments |
| Focused CTest | **3/3 passed** | new raw entry, unchanged callback entry, existing raw selector suite |

Original-byte fixtures cover all 256 selector flag values, all 256 table
bytes and indexes, all 256 amounts under four representative table gates,
null/fallback/alternate targets, initial flags, odd and boundary SPs,
offset-only/segment-only pointers, wrapped effective offsets, relocated
resident code, A20 aliases, SS=DS, target/local aliases, character links
sharing stack records, changed call opcodes/targets, and random background.

Resident +2EBE -> +2F38 remains unobserved in this composed run. +101D
normally skips the resident call for a null link; this explains ordinary
nonalias fixtures but is not a general proof for every allowed alias.
The standalone resident evidence exercised that branch; it is not counted
as coverage in these 1,575 cases.

Native execution loads the original U38 prefix at 5000 and actual resident
prefix at 7000, patches only FAR CALL segment operand +1048, and invokes
+2A87 from a generated COM program at segment 0814. There is no selector
stub. Native evidence does not cover gameplay, refusal/TRANSFER, A20 aliases,
wrapped frames, or RAM outside the captured 132,096 bytes.

Native RESULT.BIN SHA-256:
`763d2b10e48b8f0924c8774900131433776228a0f682472fb2abcb6202aec405`.
Media, generated original-bearing assembly/COM, input and capture files,
queue/history evidence, logs, and hash receipt remain ignored under
`resources/`. The receipt pins public files, unchanged compiled/imported
dependencies, tested/integration bases, private artifacts, media, and tools.

## Reproduce

With owner-supplied media in `resources/DOS/POOLRAD/`, Capstone/Unicorn,
a C compiler, GNU as/ld, and DOSBox-X:

```sh
PYTHONPATH=resources/pythondeps:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/u38_target_index_gate_2a87_cpu_audit.py \
  --unicorn --native --output resources/oracles.json
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 \
  -fsanitize=address,undefined -fno-omit-frame-pointer -I include \
  tests/recovered/test_u38_target_index_gate_2a87_cpu.c \
  src/recovered/overlay_u38_target_index_gate_2a87.c \
  src/recovered/overlay_u38_target_select_101d.c \
  src/recovered/resident_character_target_select.c -o resources/semantic
ASAN_OPTIONS=detect_leaks=1 resources/semantic
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target test_u38_target_index_gate_2a87_cpu \
  poolrad_overlay_u38_target_index_gate_2a87_tests test_u38_target_selector_cpu -j4
ctest --test-dir resources/build \
  -R '^(u38_target_index_gate_2a87_cpu|poolrad_overlay_u38_target_index_gate_2a87_tests|u38_target_selector_cpu)$' \
  --output-on-failure
```

## Adversarial assessment

- **target_function:** OVL_0038_2A87, complete [2A87,2AE6).
- **source_candidates:** extended `overlay_u38_target_index_gate_2a87.c`
  with unchanged raw +101D/resident selector sources. Legacy callback entry
  remains a separate API.
- **signals:** pinned body hashes, CALL operands/fixup, repeated index reads,
  all wrapper branches, unsigned amount shift, stack cleanup, source-ordered
  aliases, full original CPU/RAM comparison, and actual native call chain.
- **score:** 5/5 author evidence categories supported (identity, control flow,
  side effects, original comparison, native comparison); not acceptance credit
  or a probability of correctness.
- **verdict:** **likely_match**, within the documented profile.
- **review_notes:** undefined AF requires the explicit measured profile;
  one composed conditional outcome and defensive statuses remain unobserved;
  native RAM scope is bounded; identity is provisional. Independent acceptance
  and any credit remain with the reviewer/VP.
