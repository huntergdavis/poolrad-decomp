# U6C +4235 record-selected dice sum

`pr_u6c_record_roll_sum_4235` translates U6C:+4235..42EC and its reached
U9E entry-8 / SYSTEM random / SYSTEM seed bodies. It accepts a 1 MiB RAM
image, fourteen CPU words, and two loaded dependency code segments. There
are no result callbacks. Overlay loading and natural game reach are unmeasured.

## Current selection and provenance

The refreshed schema-6 successor to the requested schema-4 queue, evaluated
against main 00038c00, ranks this caller 358 with score 32 and 73 instructions.
It is the highest available composable hard target: the only higher hard
caller, U33 +0810, is actively owned by Claude. Full git history correctly
reports +4235 as an unpublished branch rather than a new implementation.
The read-only operations ledger had no +4235 body event, and no PR existed
for that branch. The reuse claim was sent to ds4 before submission.

This PR brings forward commit 7f200207f6f3c488d64d5d8d1aeac7e0a600e795,
including its complete C implementation, regression and original/native
oracles. The implementation is unchanged. Integration preserves current
CMake targets, and both audit entry points now accept explicit --exe and
--ovr paths instead of requiring the historical media directory. This is
newly submitted work, not a claim to have newly authored the reused body.
A fresh targeted deja query yielded no reusable session text; the concrete
reuse source is the cited commit and the dependency documents below.

Independent verification and credit remain with the reviewer. This branch
changes no acceptance event, strict manifest, credit count or denominator.

## Source pins and identity signals

| Body | Extent (exclusive end) | Bytes / instructions | SHA256 |
| --- | --- | --- | --- |
| U6C caller | +4235..42ED | 184 / 73 | fdb467838a6d2067178154c2b0bf3d60f0acff37060f337977e3d66d24391d60 |
| U9E entry 8 | +0DE3..0E2E | 75 / 29 | 7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba |
| SYSTEM random | +11C0..11D6 | 22 / 10 | e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9 |
| SYSTEM seed | +120F..1245 | 54 / 22 | f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242 |

Owner media: START.EXE, 65,231 bytes, SHA256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR, 206,105 bytes, SHA256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U6C starts at file offset 104831; the caller occupies [121780,121964).
U9E occupies [179405,179480). Resident expansion maps SYSTEM random to
file [48694,48716), seed to [48773,48827). SYSTEM CS:+1245 is word 8405h.
The audit verifies each span and segment relocation, reusing the merged
U9E ABI/seed provenance checks.

Two incoming calls are instruction-aligned: +056C calls at +1D3B, and
+29AE at +2FB1. A full-unit raw E8 target scan finds exactly these two.
The former passes a saved record and mask FFh; the latter passes the active
DS:5D92/5D94 pointer and a local mask. Both push CS before the near CALL.
Native fixtures execute the original +2FA4..2FB3 prefix and capture +2FB4
before its following XOR AH,AH. These are static game-role signals.

## Raw-state contract

Entry SS:SP holds a far return, SS:SP+4 a mask word (low byte used), and
SS:SP+6 a far record. Indices 0..7 test signed record bytes +96h+index.
Positive gates select DS:05DE+index mask bits. A nonzero intersection calls
U9E with count DS:08B6+index and range DS:08BE+index, both byte arguments.

Each die executes SYSTEM +11C0 and +120F. The ordinary nonaliased seed
recurrence is `seed = seed * 08088405h + 1` modulo 2^32. SYSTEM returns
the new high seed word modulo its live range word, or zero for range zero.
Zero range still advances the seed. U9E adds one per die with byte wrap;
count zero makes no SYSTEM call. Low seed word DS:4428 is written before
high word DS:442A. BX/CX scratch values, DX effects, and real near/far
frames remain observable.

After U9E returns the caller rereads the record gate. Gate one enforces
minimum `floor(2 * live_range / 3)`; other positive gates retain the roll.
Selected rolls sum modulo 256. The final index CMP defines all arithmetic
flags. RETF 6 returns AL, retaining actual AH and remaining CPU effects.
Tables, record, mask, seed and nested locals are live RAM reads, including
aliases. Offsets/SP wrap at 16 bits; contiguous multi-byte operands and
A20-off physical RAM wrap at 20 bits. Code, multiplier and loaded layout
must remain immutable and disjoint from writes.

`ARGUMENT` rejects null pointers or RAM below 1 MiB before mutation.
`TRANSFER` stops at the first changed internal CS:IP after its real return
pop, without executing arbitrary code. Seed near returns and U9E returns
have defined arithmetic flags. SYSTEM DIV writes the pinned DIVW flags and SYSTEM OR clears AF (pinned FillFlags). Pinned DOSBox-X `DIVW`/`IDIVW`/`DIVB` write all six arithmetic flags (AF=SF=OF=0, ZF=(rem==0 && quo odd), CF=((rem&3) in 1..2), PF over rem:quo; `instructions.h:635-761`), measured natively in [U33_PARTY_ATTACH_12E1_CPU.md](U33_PARTY_ATTACH_12E1_CPU.md) (256 IDIV checkpoints, all FLAGS bits unmasked); the model writes those values. The raw-Unicorn oracle
masks. The raw-Unicorn oracle masks only the bits where it differs from the pinned profile, and only if the
corresponding internal far return changes. Normal returns compare all flags.

Seed/frame aliases can rewrite live loop bounds or indexes. The C model has
no artificial iteration cap. The original-byte oracle uses a declared
50,000-instruction budget per case; exhausted cases are separately reported
and excluded from C parity, with no nontermination claim. Arbitrary alias
termination and self-modifying code are outside the verified evidence.

## Reproduction and review

```sh
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u6c_record_roll_sum_4235_audit.py \
  --exe /path/to/ignored/resources/START.EXE \
  --ovr /path/to/ignored/resources/GAME.OVR \
  --output resources/u6c-4235-parity/audit.json
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u6c_record_roll_sum_4235_native.py \
  --exe /path/to/ignored/resources/START.EXE \
  --ovr /path/to/ignored/resources/GAME.OVR
```

The oracle executes all four pinned bodies with verified segment relocations
and a transparent same-segment JMP at loaded U9E:+0048 to +0DE3. No seed
effect or RNG result is stubbed. It compares all 1 MiB and fourteen CPU words.
Conditional coverage excludes U9E +0DFE -> +0E1F: the bound just copied a
nonzero count and AL is one, making unsigned JA impossible. Other edges
must be covered individually.

DOSBox-X normal-core 386 allocates three separate code blocks plus a
synthetic program/data/stack segment. Only verified segment words and
transparent entry/capture jumps are patched. Original bodies and the
original incoming caller prefix execute unchanged otherwise. Captures compare
fourteen CPU words, exact flags, 64 frame bytes, 256 record bytes, eight mask
bytes, sixteen dice-table bytes, and four seed bytes. Cases cover signed gates,
mask combinations, zero count/range, minimum paths, byte wrap, seed carry,
record/frame and record/seed aliases. Native captures do not compare all RAM.
All copied media, generated COM files, output and logs stay ignored under
resources/u6c-4235-parity/.

Fresh measured result: 1,609 complete RAM/CPU comparisons (1,593 normal returns,
16 internal transfers), all 134 instruction sites and 17 feasible edges;
39,175 SYSTEM calls and 9,178 threshold DIVs. All normal returns and
12 changed returns compare exact FLAGS. Four changed SYSTEM returns
(seed-frame 443F, 4440, 4441, 4442) mask only the architecturally undefined
arithmetic bits, mask 08D5h. Eight original-only cases
exhausted the 50,000-instruction budget: SS=DS=1000h, entry SP=442Eh,
442Fh, 4430h, 4431h, 443Ah, 443Ch, 443Dh, 443Eh. They are excluded from
those 1,609 parity cases. No C parity or termination assertion is made for
them. The historical quick matrix measured 735 cases (728 returns and
seven internal transfers), with five separately excluded budget cases;
only the complete matrix is rerun for this submission.

Fresh native run: 620/620 returns, exact flags, result SHA256
`a35c1eeea02b43dddbf34e8d9e8a915d523edfa57822b08dacc7067a0450cede`.
Fresh validation on current main: strict C11 -Wall/-Wextra/-Wpedantic/
-Werror and ASan/UBSan pass for the synthetic regression, including the
changed-return case. The recovered library builds and the focused +4235
CTest passes. These are the checks rerun for this submission; the earlier
246-test result belongs to the reused commit and is not claimed as current.

- `target_function`: U6C:+4235 and reached U9E:+0DE3, SYSTEM:+11C0/+120F.
- `source_candidates`: the new caller source and its private raw RNG helpers.
- `signals`: pinned spans, two aligned callers, three tables, +96h gates,
  RNG constants, real frames/seed writes, original-byte and native oracles.
- `score`: bounded parity evidence; no whole-game percentage.
- `verdict`: author evidence submitted for independent acceptance.
- `review_notes`: loader, natural invocation, arbitrary alias termination,
  and strict-accounting eligibility require separate evidence.

Bounded deja queries for this target/seed discrepancy returned no reusable
completed finding. Reused findings: [merged U9E ABI/seed pins](U9E_ENTRY8_ROLL.md),
[merged U6C CPU/offset conventions](U6C_RECORD_TABLE_SUM_3F31.md), and completed
+3FB3/+408C separate-code/frame audit design. No unmerged symbol or accounting
metadata is required by this branch.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_006C_4235 --require-current-main
```
