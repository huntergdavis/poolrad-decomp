# U6C +42ED record arithmetic, loaded-code candidate

The candidate translates U6C:+42ED..4453 and reached SYSTEM multiply,
software signed long divide, and conditional absolute-value bodies. It uses
1 MiB RAM, fourteen CPU words, an explicit loaded SYSTEM code segment, and
real far-call/PUSHF/POPF frames. This is a local bounded parity candidate.
SYSTEM's error handler, overlay loading, native execution, and natural game
reach remain unmeasured; no identity, strict-credit or merge count is claimed.

## Pinned sources and caller

| Body | Offsets (exclusive end) | Bytes / starts | File bytes | SHA256 |
| --- | --- | --- | --- | --- |
| U6C caller | 42ED..4454 | 359 / 127 | [121964,122323) | c6e6d64b5bc9585934ecdd62ce2f7b5289a9401b74f4abc6d8c4fbd8dd7d3627 |
| SYSTEM multiply | 0A5B..0A76 | 27 / 16 | [46801,46828) | 74b9ab3b26ee7b54bf51f067f0825dea48772933b210dc5b38897464e5f65745 |
| SYSTEM divide | 0A76..0AE4 | 110 / 55 | [46828,46938) | 131b97677c63a6daa216f4219868f045485e35f999a551feb16daf4ec50d69e5 |
| SYSTEM absolute | 0AFC..0B0B | 15 / 7 | [46962,46977) | 23987438713f64d2c32a7535df1ebf23ea044fdba8848526cb97817fa84a25e2 |

Owner inputs: START.EXE (65,231 bytes), SHA256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR (206,105 bytes), SHA256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U6C file base is 104831. Resident expansion provenance is checked byte by
byte. The audit verifies all six segment-fixup operands at 4301, 4345,
4392, 43D5, 440A and 4429.

Exactly one raw E8 target site, +2ECF, is instruction-aligned within U6C
+29AE and preceded by PUSH CS. Its +2EBF prefix pushes signed amount 1000
and the active far record at DS:5D92/5D94. The updater accesses record words
+88h onward and live DS dwords +0D24 onward. These are static role signals.

## Raw behavior and boundaries

At entry, SS:SP holds a far return, SP+4 a far record, and SP+8/+10 the
low/high signed amount words. The caller multiplies the amount by 200 with
32-bit wrap, saves a far record pointer, and starts index zero.
While the amount is positive, it divides by a live denomination, adds one
to the quotient, keeps its low word, and caps it to the live record word.
It subtracts denomination times that low word from the amount and removes
that many units from the record. The index increments as a byte.

For a resulting negative amount, SYSTEM absolute value runs, the index
starts at four, and a descending loop divides by each denomination without
the increment/cap. It subtracts denomination times the low quotient word
and adds that word to the record. Wrapped INT32_MIN stays negative after
absolute value, so the second loop exits immediately. RETF 8 follows.

All argument, table, record and local reads remain live. The software divider
performs the actual 33 restoring rounds, retains remainder/scratch registers,
and saves/restores the original sign-test FLAGS in RAM. Multiply retains
temporary words on the real stack. No result callback or host division
substitutes for the reached software algorithm. LES and far returns use
contiguous physical operands; effective offsets/SP wrap at 16 bits and
A20-off RAM at 20 bits. Code/layout must remain immutable and disjoint from
writes. Asynchronous interrupts are outside the contract.

`ARGUMENT` rejects null pointers/small RAM before mutation. `TRANSFER` stops
after the first changed internal far return. Zero divisor is a software
error path: SYSTEM pops BP, sets AX=00C8h, then jumps to +00E2.
`UNSUPPORTED_RUNTIME` stops there before executing the unmodeled handler;
it does not invent a hardware divide exception or handler result. AF after
OR is undefined, including saved PUSHF words; the model chooses zero and
the measured Unicorn execution agrees. Normal caller returns have defined
final CMP flags, compared without masking.

The C loops have no artificial cap. The audit budgets original execution
at 100,000 instructions per case; exhausted cases are reported separately,
never passed to C for parity, and carry no termination claim.

## Small parity gate and feasible coverage

```sh
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u6c_record_arithmetic_42ed_audit.py \
  --output resources/u6c-42ed-parity/small.json
```

Original-byte execution compares the complete 1 MiB and all fourteen CPU
words at normal returns, changed internal returns, or the explicit +00E2
boundary. All media, disassembly and generated output stay ignored under
resources/. Synthetic C tests independently check weighted-value conservation
for sufficient records, signed multiplication/absolute wrap, argument guards,
and the exact zero-divisor stop state.

There are 205 source instruction starts, 193 feasible from this caller.
The twelve excluded starts are caller 43C0 and divider 0A91/0A93/0A95/0A98,
0ABF/0AC1/0AC3/0AC6/0AC9/0ACA/0ACC. Seven conditional edges are infeasible:

- Caller 43BE -> 43C0: the preceding positive loop just stopped, and nothing
  writes the amount before its repeated sign check.
- Caller 43C7 -> 43CC: unsigned CMP low,0 cannot set carry.
- Divider 0A8F -> 0A91 and 0ABD -> 0ABF: every division call loads the
  amount after a positive check with no intervening write; saved sign flags
  restore that nonnegative dividend sign.
- Both 0ACA edges: their enclosing negative-dividend path is unreachable.
- Absolute 0AFE -> 0B0A: this call loads the amount after a negative check
  with no intervening write, so its nonnegative shortcut cannot execute.

The gate requires all 193 feasible starts and 31 feasible conditional edges.
Excluded paths are implemented source translations, with no independent
negative-dividend helper parity claim.

The small matrix passes 183 complete RAM/CPU comparisons: 176 returns,
six changed internal returns, and one zero-divisor stop at SYSTEM:+00E2.
All 193 feasible instruction starts and 31 feasible edges pass. Nine original
cases exhaust the 100,000-instruction budget and are excluded: amount-1000
with the deliberately insufficient default record, negative-divisor,
large-divisor, empty-record, large-high-divisor, record-frame-7F80,
table-frame-D30, table-frame-D38, table-frame-D3C. A distinct amount-1000
case with sufficient record units passes. No termination claim is made for
budget cases. All 246 CTests pass with -Wall/-Wextra/-Wpedantic/-Werror,
including the focused synthetic test. Shell syntax, research checksums,
staged whitespace and public-media hygiene checks pass.

- `target_function`: U6C:+42ED and its reached arithmetic bodies.
- `source_candidates`: new uniquely named updater and private runtime helpers.
- `signals`: exact source pins, sole aligned caller/ABI, live record/table
  effects, raw frames/FLAGS, whole-RAM/CPU oracle and synthetic conservation.
- `score`: bounded source/state evidence, no percentage or whole-game count.
- `verdict`: match within the stated loaded-code and small-gate boundaries.
- `review_notes`: the error handler and native/natural routes remain open.

Bounded deja queries returned no completed matching finding. Reused findings:
[U7F long-arithmetic dependency identities](U7F_DIV5_DWORDS_ENTRY11.md),
[merged raw CPU/offset conventions](U6C_RECORD_TABLE_SUM_3F31.md), and the
reviewed +4235 immutable-code/changed-return audit design. No unmerged symbol
or accounting metadata is required. Adjacent CMake registrations overlap
the three pending U6C branches; integration must preserve all entries.
