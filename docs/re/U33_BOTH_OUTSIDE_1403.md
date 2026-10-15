# U33 +1403: both signed bytes outside their ranges

`pr_u33_both_outside_1403` translates the complete 49-byte/18-instruction
U33:+1403..1433 predicate. AL is one exactly when signed byte a is outside
0..5 **and** signed byte b is outside 0..10; otherwise AL is zero. AH is
preserved. The application meaning of these two caller locals is unverified.
Natural game reach, verified game identity and strict-credit eligibility
remain separate PM gates; no accounting/classifier files change.

## Pins and caller ABI

Owner inputs: START.EXE (65,231 bytes), SHA256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR (206,105 bytes), SHA256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U33 file base is 50641, original stub segment 003Bh. Exact body bytes are
[55764,55813), SHA256
`a749aec8ac16739928c52f412b1bd4447e3bb668500b56bd76b1a5140288d62b`.
The next callable starts at +1434. All 49 bytes decode; terminal +1431 is
RETF 4, with four conditional sites/eight edges, one unconditional jump,
no outgoing calls, interrupts or relocations.

Exactly two raw E8 target sites align within U33:+1577: +17C8 and +18C8,
each preceded by PUSH CS. Their original prefixes load AL from BP-17h and
push AX, then load AL from BP-18h and push AX. Thus entry SS:SP+4 holds
argument a (low byte of the second push), and SP+6 holds b. Word upper
bytes retain the caller's AH but are ignored. Both following instructions
OR AL,AL: +17CD branches on zero, while +18CD branches on nonzero. The
audit pins each prefix instruction and this result use, not merely raw
opcode hits. The ranked ledger row is grade B, helper_candidate/unknown,
rank 448 in the current-main private selection artifact. PM reserved the
distinct leaf in inbox i3645; pG's U7F +0020 is excluded from this lane.

## Complete raw-state behavior

After PUSH BP and SUB SP,2, the caller's b byte is compared with zero.
For nonnegative b, a second CMP tests eleven and may take the zero-result
shortcut without reading a. Otherwise a is compared with zero, then six
if nonnegative. A negative a or a at least six yields one. The last executed
byte CMP supplies all six arithmetic flags; the source's signed branch
order is retained instead of deriving flags from a Boolean expression.

One local byte at BP-1 stores the result. MOV AL preserves the incoming AH;
MOV SP,BP, POP BP and RETF 4 use live RAM. The model returns actual CS:IP
after the far pop without following that destination. Other general/segment
registers remain unchanged. Effective offsets/SP wrap at 16 bits, contiguous
multi-byte operands and A20-off RAM at 20 bits. Code must remain immutable
and disjoint from writes. Null pointers/small RAM are rejected before mutation.

## Reproduction and evidence

```sh
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u33_both_outside_1403_audit.py \
  --output resources/u33-1403-parity/audit.json
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u33_both_outside_1403_native.py
```

The full original-byte oracle passes 4,090 cases, comparing the complete
1 MiB and fourteen CPU words, including all flags without masking. All
18 instruction starts and all eight conditional edges are covered. The
matrix sweeps every byte value across representative opposite arguments,
boundary pairs, all word upper-byte values and preserved AH, frame/operand
wrap and deterministic random initial memory/flags. Quick: 370 cases.
The finite original path is checked within 32 instructions; no loop-budget
exclusions apply.

DOSBox-X normal-core 386 passes 1,024 synthetic native cases executing the
original +17BF..17CA prefix and complete leaf in a separately allocated
code segment. A transparent far capture jump records the original +17CB
return before OR AL,AL. Four complete byte sweeps cover b=0/b=11/a=0/a=6,
with varied incoming AH and non-arithmetic flags. All fourteen CPU words,
exact flags and 64 frame bytes match C. Native RESULT.BIN SHA256:
`07a3f5291f0e5187ad4e4a0481ea5d2478d24a113770e7d4148aad4c6b4f35a7`.
Native frames are synthetic; actual loader dispatch and natural game calls
remain unmeasured. Native captures do not compare the entire RAM image.

The focused C test exhausts all 65,536 signed-byte pairs against the
independent mathematical predicate, varies preserved AH, uses unrelated
argument-word high bytes, and checks exact flag vectors, guards, frame
effects and unchanged registers. All 246 strict CTests pass with
-Wall/-Wextra/-Wpedantic/-Werror. Shell syntax, research checksums, staged
whitespace and public-media hygiene pass. All copied original bytes,
disassembly, generated COM files and captures stay under ignored resources/.

- `target_function`: U33:+1403..1433.
- `source_candidates`: the new uniquely named raw both-outside predicate.
- `signals`: exact extent/hash/return, two aligned argument/result call
  patterns, all path flags, complete original-byte RAM/CPU comparisons,
  exhaustive synthetic truth table and independent native execution.
- `score`: 18/18 starts and 8/8 edges; no whole-game percentage or count.
- `verdict`: match within the immutable-code/raw-frame contract.
- `review_notes`: caller-local meanings, natural reach, verified identity,
  credit eligibility and merge remain separate gates.

The bounded deja query returned no completed finding. Reused findings:
[merged raw CPU/offset conventions](U6C_RECORD_TABLE_SUM_3F31.md), the current
ranked-ledger selection and source pins, and the independently reviewed
[U7F separate-code/native capture design](https://github.com/huntergdavis/poolrad-decomp/blob/460dfd516d83a8090063a857fafa074ff1fbe8de/docs/re/U7F_RECORD_WORD_ADD_003A.md).
This branch requires no unmerged symbol. Its CMake registration is adjacent
to five pending registrations; integration must preserve all entries.

Second distinct candidate queued: U3C2 public entry 1 at +0000..002D,
46 bytes/17 instructions, terminal RETF 2. It returns signed-word signum
(-1,0,+1), has two conditional sites/four edges and no calls/interrupts.
Source SHA256 is
`5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf`,
file [195410,195456). Two aligned PUSH CS callers are U3C2:+01A0 sites
+01F9/+0213; two fixup-confirmed aligned far callers are U86:+30D9 sites
+3150/+316D through public stub 03DB:0025. It is grade A,
helper_candidate/unknown, rank 445, with no matching implementation/claim
or public open-PR collision found. It remains queued, not translated here.
