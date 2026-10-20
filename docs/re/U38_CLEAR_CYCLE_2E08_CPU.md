# U38 +2E08: four raw helper calls and the final byte gate

The complete public entry 109 calls U38 +0000 with keys 0B, 35, 34 and 37,
in that order. Each call carries the current AH in its pushed argument word.
The helper clears DS:6817/6818 when the key matches the latch. After the four
calls, a zero byte at DS:682A makes the caller write 64h to DS:6816. It returns
with RETF 0Ah. The new CPU14 API composes the merged raw helper and preserves
live stack/RAM aliases, including unexpected callee return destinations.

This is author evidence for independent review, with no accounting changes
or self-credit. Synthetic parity does not settle identity. The identity
argument is stated in [Identity via the credited installer](#identity-via-the-credited-installer)
for the review lane to reproduce.

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
cluster, whose resident selector was then callback-only (since replaced by the
raw selector in [U38_TARGET_SELECTOR_CPU.md](U38_TARGET_SELECTOR_CPU.md)). U25 +07BC is genuinely
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

## Identity via the credited installer

Written against the two-factor standard in
[REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20)
(design A2 for table entries).

**Factor 1: reached from game code, through a credited installer's table.**
Reproduced by the author on GAME.OVR `872964a3…`, the regenerated loader
inventory and the checked-in ledger on main 5453ccb2:

1. Unit 0x38's public entry table has 117 entries. `entries[109] = 2E08h`,
   and 109 is the only index holding 2E08h.
2. The unit's memory segment is 41h (inventory `memory_segment` 65, derived
   from the next descriptor, sequential check true). Entry k's stub is at
   `0041:(20h + 5k)`. This is measured, not assumed: all 136
   `MOV AX,imm; MOV DX,0041h` pairs in 31A2 satisfy imm = 20h + 5k, and
   every one names a ledger function's entry. (The inventory field
   `stub_header_len` = 22 is not used for this offset.)
   Entry 109's stub is therefore `0041:0241`.
3. `OVL_0038_31A2` contains, at +37E4..+37F0,
   `MOV AX,0241h; MOV DX,0041h; MOV [6A1C],AX; MOV [6A1E],DX`. It stores
   exactly that stub far pointer in DS:6A1C/6A1E (slot 125 of the effect
   table). The `0041h` immediate word at +37E8 carries a loader fixup. It is
   the only `MOV AX,0241h` in any unit 0x38 ledger function.
4. `OVL_0038_31A2` is identity-credited (reviewed identity #467, commit
   `aca640cf…`) and strict-credited (U38_EFFECTS_INSTALL_31A2.md).

So +2E08 is installed as a game effect handler by a credited game body (the
A2 reading). Slot arithmetic per the standard: `(6A1C - 6828) / 4 = 125`.
The older sentences calling 31A2 unresolved were corrected in #1196 (main
`554083a2`). The credit status above is read from the two CSVs, not the prose.

**Factor 2: operates on documented game state.** +2E08 pushes no pointer; its
subject is DS game state that credited bodies' evidence docs define:

- the four calls go to the credited helper `OVL_0038_0000`. Per
  [U38_HELPER_CLUSTER_CPU.md](U38_HELPER_CLUSTER_CPU.md), with key k it
  clears `DS:6818` and `DS:6817` when k is 0 or `DS:6817` equals k. +2E08
  supplies k = 0Bh, 35h, 34h, 37h;
- its final gate `CMP BYTE [682A],0` → `DS:6816 := 64h` is the same documented
  state operation that the credited `OVL_0038_29F0` performs (same doc,
  entry 95);
- `DS:6818` is also the byte that the credited +25BC halves
  ([U38_TARGET_MASK81_25BC.md](U38_TARGET_MASK81_25BC.md)).

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
composition's six-outcome denominator. Kind: **unreachable-by-composition**.
Feasible state: `[BP+6] = 0` at helper +0003, i.e. a caller passing key 0.
The credited entries 50 (+1490), 94 (+29D9) and 96 (+2A11) do exactly that,
so the branch is real code outside this composition.

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

**Author re-run for registration** (2026-10-20 12:10:45–12:11:39 PDT, main
5453ccb2 + this audit; resources/2e08/final.json, ignored, SHA-256
`75b37d72…589d`). It reproduces the table: 551 cases (538 returns, 5
TRANSFER, 8 CODE) on CPU14 + 1,048,576 bytes unmasked; caller 23/23 starts
and 2/2 outcomes; closure 34/34 starts and 5/6 outcomes, the missing outcome
being exactly the excluded +0007→+0011; native 256 returns with an identical
RESULT.BIN. The ASan/UBSan figure in the table is from the original author's
run and was not repeated here.

`CHECKER_ROWS=('OVL_0038_2E08',)` registers this audit with the handoff gate.
`--compile-only` builds the two sources before any media access and prints
`CHECKER_BUILD_OK`; it was verified with the owner media absent.
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

### Reviewer acceptance record (added by the review lane, not the author)

Reproduced at author head `5d69fc39` merged with main `84da517f`, 2026-10-20.

- **Unicorn:** 551 cases (538 return, 5 TRANSFER, 8 CODE) on CPU14 +
  1,048,576 B, unmasked; caller 23/23 starts and 2/2 outcomes; closure 34/34
  starts and 5/6 outcomes, the only missing outcome being `0007->0011`.
  Each labelled case reached its named stop.
- **Native:** 256 returns, RESULT.BIN `83b59bba…1adc`.
- **C:** fresh CTest 1/1; strict C11 `-Werror` + ASan/UBSan rebuilt and run
  by the reviewer, **49,188 checks** (the author's figure, now repeated).
- **Gates on the merged tree:** `check_checker_builds.py --all
  --require-current-main` 21/21; pairing, evidence-doc and record-SHA OK.

**Identity (A2), checked against the CSVs and media, not the prose:**
`entries[109] == 0x2E08` (unique), `memory_segment` 0x41, stub `0041:0241`.
All 136 `MOV AX,imm; MOV DX,0041h` pairs in 31A2 fit `20h + 5k` with an
in-range entry. The store at `+37E4..+37F0` writes `DS:6A1C/6A1E` (slot 125),
`0041h` at `+37E8` is a unit fixup, and the only other `1C 6A` bytes in
GAME.OVR lie outside every unit's code. `OVL_0038_31A2` is in both
`strict_full_c_addresses.csv` and `reviewed_function_identities.csv`. Factor 2:
the helper +0000 is strict-credited, and its key test reads the pushed byte.

**Exclusion check (reviewer, construct-before-exclude):** +2E08 hardcodes the
keys as `MOV AL,imm`, and the C model does likewise, so `0007->0011` is
reachable only if a key immediate becomes 0. The reviewer tried to build that
state:

- **DS alias:** the helper's zero writes to `DS:6817/6818` cannot overlay a
  key immediate (`2E13`, `2E1A`, `2E21`), because the alignment differs mod 16
  for every CS.
- **Stack alias:** any SS:SP that zeroes an immediate with a pushed word also
  writes the return-IP high byte `2Eh` over the preceding `B0` opcode, which
  breaks the code first.

So the exclusion holds within the stated immutable-code, no-interrupt
contract. The feasible state is `[BP+6] = 0`; the key-0 callers are
`+09EF` (09C0), `+14E2` (1490, entry 50), `+2567/+2599` (2556), `+296D` (2915),
`+29E9` (29D9, entry 94) and `+2A21` (2A11, entry 96).

**Follow-up (reviewer, non-blocking):** the checker reports
`missing_outcomes` but does not assert that it equals `['0007->0011']`, so a
newly reachable or newly lost outcome would not fail the run. A one-line
assertion would close this. The documented `--output resources/2e08/...`
needs the parent directory to exist.
