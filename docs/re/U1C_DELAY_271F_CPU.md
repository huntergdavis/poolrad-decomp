# U1C +271F: raw CPU delay caller

The raw companion `pr_u1c_delay_271f_cpu` extends the existing
`overlay_u1c_delay_271f.c/.h` pair. It increments the live ECL PC word and
composes the merged raw resident wrapper and CRT Delay, including their
CPU, flags, stack, RAM and budget-stop effects. The existing timing-hook
API and implementation remain unchanged. Gameplay routing is not migrated.

**Span correction:** the schema-8 successor of the schema-4 queue reports
86 bytes / 38 decoded instructions for this row; the executable body is
exactly **[271F,272D), 14 bytes / 6 instructions**. The trailing Pascal
strings are **DATA**, excluded from the C body, extraction and all code
totals. Initial queue rank 358, score 25 selected this existing callback boundary
(rank 357, score 25 after integrating the latest acceptance/queue changes);
the ready resident dependency is now available as raw C. Queue/accounting
files are not changed by this PR.

## Identity and dependency closure

| Body | Executable span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| U1C caller | GAME.OVR file [3465,3473), U1C [271F,272D) | 14 / 6 | `aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1` |
| Resident wrapper | 0709:[0DBE,0DD3) | 21 / 10 | `a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d` |
| CRT Delay including near spin helper | 0A40:[029C,02C9) | 45 / 19 | `d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb` |

The static closure is **80 bytes / 35 instructions**. The caller executes
PUSH BP, MOV BP,SP, INC word DS:[49ED], LCALL 0709:0DBE, POP BP and RETF.
The resident loads the live DS:[49E2] byte, multiplies it by 225 and
calls CRT Delay. CRT reads the live DS:[84FC] calibration word and
0040:0000 BIOS byte; a zero calibration count wraps to 65,536 passes.
The reused [raw resident contract](RES_0709_DELAY_0DBE_CPU.md) supplies
the full loop and flag behavior.

Hash-locked owner-supplied inputs, kept under ignored `resources/`:

- START.EXE (65,231 bytes):
  `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
- GAME.OVR:
  `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

The audit decodes and checks every caller instruction, both resident
identities, the call operands and all exact span lengths.

## API and supported profile

The entry accepts exactly 1 MiB of ordinary RAM, a 14-word CPU state at
IP 271F, and an already-present far-return frame. A20 is off. Entry SP
is 0010..FFFC, which keeps the deepest reached near-call frame and the
outer return frame from wrapping the 16-bit stack offset. Physical
addresses still wrap at 1 MiB. Code stays immutable during execution;
invalid call-operand fixtures stop at their guards. DS/SS may alias each
other and ordinary data, including the BIOS byte and stack frames.

INC preserves CF and computes OF/SF/ZF/AF/PF before the call guard.
The outer LCALL validates opcode and offset and reads the live segment.
It pushes the real return frame and calls
`pr_r0709_delay_wrapper_0dbe_cpu` directly. No timing callback is used.

| Status | Preserved state |
| --- | --- |
| OK | Outer RETF has completed; actual popped return address and restored BP are retained. |
| ARGUMENT | Invalid pointer, RAM size, entry IP or SP; CPU/RAM unchanged. |
| CODE | At caller 2726, after PUSH BP and INC, before the call frame. |
| CALLEE | Resident failure or CRT budget stop; all nested CPU/RAM retained without unwind. |
| TRANSFER | Defensive guard for an unexpected successful resident return; not reached by these fixtures. |

The report starts with both statuses at -1. Once reached, it carries the
resident status and CRT status separately. The 64-bit loopback cap passes
unchanged to CRT Delay. A rejected taken backward jump stops **at its
target**, with the real stack frames still present. This is a bounded
execution API, not a duration guarantee.

The resident's existing undefined MUL-flag profile is inherited:
SF/ZF/PF from AX, AF clear, CF/OF zero. It is observable at the resident
CODE stop and matches Unicorn. A successful CRT entry overwrites those
flags. No cross-CPU or native partial-stop claim is made for undefined
MUL flags. The inherited logical-operation AF behavior also belongs to
the measured emulator profile.

## Measured evidence

The author harness is
[`u1c_delay_271f_cpu_audit.py`](../../tools/overlay/u1c_delay_271f_cpu_audit.py).
The original execution decides the stop and return addresses independently
of C. The comparison checks all **14 CPU words and 1,048,576 RAM bytes**,
with **no masks**.

| Original-byte outcome | Cases |
| --- | ---: |
| Full return, resident and CRT both returned | 37 |
| Caller CODE guard, resident not entered | 50 |
| Resident CODE guard, CRT not entered | 257 |
| CRT LIMIT propagated through resident and caller | 30 |
| Total | **374** |

Coverage is **6/6 caller instruction starts**, **35/35 closure starts** and
**8/8 conditional outcomes**, all observed in this caller's runs. Cases
include every byte factor at the resident guard, INC flag and PC rollover
boundaries, wrong opcode/offset guards, zero and nonzero factors, factor
255, calibration 0/1/2/FFFF, budget stops at each backward-branch target,
relocated resident segments, random registers/flags/RAM, SP limits,
A20 wrapping, stack/PC/factor/count aliases, and the near-call write
aliasing 0040:0000. Zero calibration is tested with a bounded stop; this
caller audit does not run all 65,536 passes to completion for that input.

The DOSBox-X normal-core/386 synthetic oracle executes the original caller
at 9000:271F, relocates its wrapper operand to 8800:0DBE, and reaches CRT
at 0A40:029C. It passes **32 full-return cases**, including **24 nonzero
delays** and factor 255. Every case compares all **14 CPU words**, the
**256-byte stack window**, and the **entire 65,536-byte DS segment**,
with **no masks**. The harness records BIOS byte 0040:0000 (F8h), keeps
interrupts disabled during the call, and replays that byte in C.

Native RESULT.BIN SHA-256:
`55643d6608dbd77c0c2b0383353a16f6a2fa827223450401e100091f00c32bab`.

Synthetic C tests pass **1,051,953 checks under ASan/UBSan**. They exhaust
all 65,536 PC inputs with both CF values at the outer guard and all 256
factor values through the resident guard and complete returns. They also
check argument rejection without CPU/RAM mutation, allowed write locations,
nested diagnostics and exact loop-budget target state. All four focused
CTests pass: new raw caller, preserved hook caller, legacy CRT model,
and the resident raw wrapper/CRT tests.

## Reproduction

With Capstone and Unicorn installed:

```sh
python3 tools/overlay/u1c_delay_271f_cpu_audit.py \
  --unicorn --native --output resources/u1c-delay-report.json
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target poolrad_u1c_delay_271f_cpu_tests \
  poolrad_overlay_u1c_delay_271f_tests \
  poolrad_resident_0709_delay_wrapper_0dbe_cpu_tests
ctest --test-dir resources/build --output-on-failure \
  -R 'u1c_delay_271f|resident_0709_delay_wrapper_0dbe_cpu'
```

The native audit additionally requires GNU as/ld and DOSBox-X. It fails if
the observed COM segment is not 0814; it does not silently relocate the
absolute CRT hole to a guessed address. Original media, generated guest
code, capture binaries and evidence receipts remain ignored.

## Reuse and adversarial check

Dunking Sheep memory query
`U1C 271F delay 0709 0DBE A40 029C raw CPU native` returned Claude session
`281c8700-5acc-438b-b93d-c061e28437eb`, including the earlier delay-path
approval i18063. Reuse is the merged resident APIs, their documented
contracts, extraction and native-capture pattern; this PR measures the
new caller composition itself. Ownership was approved in i18790.

- **target_function:** U1C [271F,272D), six instructions, with the two
  reached raw resident bodies.
- **source_candidates:** the raw companion in
  `src/recovered/overlay_u1c_delay_271f.c`; the preserved callback entry
  remains a distinct API with a different contract.
- **signals:** exact media and span hashes, six decoded caller instructions,
  live DS operands, call/return frames, independent original stop selection,
  full CPU/RAM comparison, native return captures and sanitizer tests.
- **score:** 9/10 author confidence within the explicit emulator/RAM profile;
  this heuristic is not an acceptance or credit score.
- **verdict:** likely_match within that profile, pending independent review.
- **review_notes:** no elapsed-time or native-timing parity; no asynchronous
  interrupt effects, native budget/guard-stop evidence, full native RAM
  capture, MMIO model, wrapped stack-frame offsets, self-modifying code,
  defensive TRANSFER coverage, or end-to-end gameplay claim.

The earlier [saved-game bounded comparison](SLOT_A_DELAY_WRAPPER_CHAIN_COMPARE.md)
left 17 lower-stack bytes unexplained after supplying the observed timer
advance, and did not record SI/DI/FLAGS at the native returns. This
synthetic evidence does not resolve those gameplay observations.

No classifier, acceptance record or credit total is edited. Independent
review and accounting remain with the VP/reviewer.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_001C_271F --require-current-main
```
