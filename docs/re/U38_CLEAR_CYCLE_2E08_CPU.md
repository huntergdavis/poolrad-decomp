# U38 +2E08: four raw helper calls and the final byte gate

The complete public entry 109 calls U38 +0000 with keys 0B, 35, 34 and 37,
in that order. Each call carries the current AH in its pushed argument word.
The helper clears DS:6817/6818 when the key matches the latch. After the four
calls, a zero byte at DS:682A makes the caller write 64h to DS:6816. It returns
with RETF 0Ah. The new CPU14 API composes the merged raw helper and preserves
live stack/RAM aliases, including unexpected callee return destinations.

This is author evidence for independent review, with no accounting changes
or self-credit. The game identity remains game_or_other_library_unresolved /
unresolved; synthetic parity does not settle that classification.

## Identity, queue and prior work

| Span | File extent in GAME.OVR | Bytes / instructions | SHA-256 |
| --- | --- | --- | --- |
| U38 [2E08,2E37), entry 109 | [11A63,11A92) | 47 / 23 | 7142253f1af3c81015b05a5e1743c3cf19ecf776ec43da551cc00b4e07d374a1 |
| U38 [0000,001F), helper | [EC5B,EC7A) | 31 / 11 | f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240 |

START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The loader inventory identifies unit 38 at file EC5B and public slot 109 at
2E08. There are no fixups in this caller. Near calls at 2E0F, 2E16, 2E1D and
2E24 all target 0000; RETF 0Ah is at 2E34. **All 47 bytes are code.** The two
trailing data bytes reported for the preceding U7D +047A task do not apply here.

The schema-4 queue lineage currently emits schema 8. The selection snapshot
ranks this candidate 447, score 4, 1/1 ready. Its inline_in_strict hit is only
the neighboring +2DE4 header's end-address comment, not an implementation.
Higher ready rows were already raw implementations, except the U38 +101D
cluster whose resident selector remains callback-only. U25 +07BC is genuinely
implemented inline in overlay_u25_load_word_0e0c.c and was not duplicated.

Preimplementation shared-memory queries covered the U38 selector cluster,
U25 +07BC, and U38 +2E08. They did not establish a current +2E08 implementation.
Direct branch inspection found the old unmerged Codex prototype at
f783c27d62341e539afaeb33a5ac363a155830d0, with no PR. This change reuses its
pinned identity, key order, and semantic expectations. It replaces the old
prototype's duplicated helper with the current
[raw helper API](U38_HELPER_CLUSTER_CPU.md), and adds explicit CODE/TRANSFER
boundaries. The VP confirmed ownership in inbox i18548; the prior documentation
reservation was for this same Codex lane.

The raw helper source/header are unchanged. The full-RAM comparator follows
u7d_update_047a_cpu_audit.py, and the disk-fed native capture follows its native
harness. Old test counts/captures are historical context only; all measurements
below were produced for this implementation.

## API and execution profile

pr_u38_clear_cycle_2e08_cpu takes exact 1 MiB A20-off flat RAM, CPU14 state,
and an optional raw callee status. Entry IP must be 2E08, SP 000A..FFFC. CPU
and status storage must be distinct from RAM and one another. Reached far-return
frames must fit within the 16-bit SS offset space. This profile assumes
immutable mapped caller/helper bytes, synchronous flat RAM, no MMIO, faults,
interrupts or self-modifying code. The ten argument bytes are unused; RETF
consumes them. Effective offsets wrap at 16 bits, and bytes within a word or
far-return frame are physically contiguous with A20 wrap.

Each call checks its live E8 opcode and relative displacement after the
original PUSH AX and PUSH CS. It then pushes the real return IP, runs
pr_u38_helper_0000_cpu on the same CPU/RAM, and checks the returned CS:IP.
The next key is loaded only after a successful expected return. Saved BP,
return words, latch values, and the final gate are never snapshotted across
calls. The outer POP BP is taken from live SP; it is not replaced with MOV
SP,BP. All returned arithmetic flags are defined by CMP; no undefined-flag
mask or profile is needed.

| Result | State retained |
| --- | --- |
| RETURNED = 0 | Live outer RETF destination, CPU/RAM effects, SP advanced by 14 |
| ARGUMENT = 1 | Atomic rejection; CPU, RAM and optional status unchanged |
| CODE = 2 | Before the unsupported near CALL, after its argument and CS pushes |
| CALLEE = 3 | Raw helper failure state/result, without rollback |
| TRANSFER = 4 | Successful helper's unexpected live CS:IP and full partial state |

The optional callee status starts at -1 after valid entry, then retains the
last helper result. The helper's only nonzero status is invalid host arguments,
which are excluded by the validated caller contract; CALLEE remains a defensive
propagation path. There are exactly four finite helper calls and no loop cap.

## Measured results and limits

| Check | Result |
| --- | --- |
| Original-byte Unicorn, CPU14 + all 1,048,576 RAM bytes, unmasked | 551 cases: 538 returns, 5 transfers, 8 code refusals |
| Caller coverage | 23/23 starts and 2/2 conditional outcomes |
| Caller + helper closure | 34/34 starts and 5/6 conditional outcomes |
| DOSBox-X normal/386 | 256 returns, CPU14 + 1,024 stack + 65,536 DS + 65,536 heap bytes, unmasked |
| ASan + UBSan semantic assertions | 49,188 passed |
| Focused CTest | 1/1 passed |

The missing helper outcome is 0007 -> 0011 (key equals zero). The caller
always pushes one of the four nonzero immediates. Between that push and the
helper's first CMP, only adjacent, distinct CS/IP/BP stack words are written.
With immutable code and no interrupts, the key byte cannot become zero.
The general helper still implements that branch, and it remains in this
composition's six-outcome denominator.

Original cases include all 256 latch values on both final-gate branches;
additional signed/parity gate bytes; initial AH/flags variations; odd/minimum/
maximum SP; SS=DS; stack aliases with latch, amount, gate, saved BP and return
frames; A20 aliases for code, data and stack; random RAM background; and both
opcode and target refusal at every call. Four distinct CS aliases force the
first, second, third or fourth helper's return CS to zero; a fifth case repeats
the fourth-call transfer across A20. The native run covers all 256 latch
values, with four gate values and six initial flag patterns.

The semantic test independently checks all 256 latches, six gate values and
two AH values (3,072 combinations), expected final AX/flags, untouched CPU
fields/arguments, final stack words, four partial call refusals, four partial
transfers, and atomic bad-argument handling. The native output is compared
directly with C, not only with Unicorn.

Native RESULT.BIN SHA-256:
83b59bba64b8c279c90e625d221b70c7833589e9ed02b78836abbb75572f1adc.
Native evidence is synthetic and covers normal returns only. It makes no
claim about native transfers/refusals, whole-RAM native parity, native A20
or wrapping-return behavior, or naturally reached gameplay. Media, generated
COM bytes, inputs, captures, logs and hash receipts remain in ignored resources/.

## Reproduction and author assessment

With the pinned private media and Python dependencies available:

    PYTHONPATH=resources/pythondeps:tools/overlay python3 tools/overlay/u38_clear_cycle_2e08_cpu_audit.py --unicorn --output resources/original.json
    PYTHONPATH=resources/pythondeps:tools/overlay python3 tools/overlay/u38_clear_cycle_2e08_cpu_audit.py --native --output resources/native.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target test_overlay_u38_clear_cycle_2e08_cpu
    ctest --test-dir resources/build -R '^overlay_u38_clear_cycle_2e08_cpu$' --output-on-failure

- target_function: OVL_0038_2E08, complete [2E08,2E37).
- source_candidates: overlay_u38_clear_cycle_2e08_cpu.c plus the unchanged
  overlay_u38_helper_cluster_cpu.c raw helper.
- signals: pinned bytes and public slot; all four original call frames;
  complete live CPU/RAM differential; independent native C comparison;
  independent latch/gate arithmetic and boundary assertions.
- score: 5/5 author evidence categories present (identity, ABI, differential,
  native replay, semantic assertions), not an acceptance score.
- verdict: likely_match within the documented execution profile.
- review_notes: unresolved game identity; actual neighbor-comment false inline
  hit; unused general-helper zero-key branch retained in denominator; synthetic
  native scope; independent verification and credit remain with reviewer/VP.
