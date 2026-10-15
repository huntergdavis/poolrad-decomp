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
after the far pop without following that destination. RETF pops IP at the
entry SP, then CS at (entry SP+2) modulo 65536: the two word reads use
separately wrapped offsets. Each individual word still reads contiguous
physical bytes. Other general/segment
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

The full original-byte oracle passes 4,084 cases, comparing the complete
1 MiB and fourteen CPU words, including all flags without masking. All
18 instruction starts and all eight conditional edges are covered. The
matrix sweeps every byte value across representative opposite arguments,
boundary pairs, all word upper-byte values and preserved AH, frame/operand
wrap and deterministic random initial memory/flags. Quick: 364 cases.
The finite original path is checked within 32 instructions; no loop-budget
exclusions apply. Six SP FFFE/FFFF cases across three SS values are
excluded because Unicorn uses a linear second RETF word read there;
those two SP boundaries are verified by the separate DOSBox-X proof.
All compared registers and flags remain unmasked.

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
effects and unchanged registers. Three additional native-backed
regressions at SP FFFD/FFFE/FFFF compare all fourteen CPU words and
the local result byte, with contradictory linear CS decoys at the
two wrapped boundaries. The repair's focused predicate/adjacent U33
CTests pass 2/2 with -Wall/-Wextra/-Wpedantic/-Werror. Shell syntax, research checksums, staged
whitespace and public-media hygiene pass. All copied original bytes,
disassembly, generated COM files and captures stay under ignored resources/.

- `target_function`: U33:+1403..1433.
- `source_candidates`: the new uniquely named raw both-outside predicate.
- `signals`: exact extent/hash/return, two aligned argument/result call
  patterns, all path flags, complete original-byte RAM/CPU comparisons,
  exhaustive synthetic truth table and independent native execution.
- `score`: 18/18 starts and 8/8 edges; no whole-game percentage or count.
- `verdict`: match within the immutable-code/raw-frame contract.
- `review_notes`: separate wrapped RETF pops are proven natively; caller-local meanings, natural reach, verified identity,
  credit eligibility and merge remain separate gates.

The bounded deja query returned no completed finding. Reused findings:
[merged raw CPU/offset conventions](U6C_RECORD_TABLE_SUM_3F31.md), the current
ranked-ledger selection and source pins, and the independently reviewed
[U7F separate-code/native capture design](https://github.com/huntergdavis/poolrad-decomp/blob/460dfd516d83a8090063a857fafa074ff1fbe8de/docs/re/U7F_RECORD_WORD_ADD_003A.md).
This branch requires no unmerged symbol. Its CMake registration is adjacent
to five pending registrations; integration must preserve all entries.


## Native-backed RETF repair

This repair starts from published head
3986c34c00232eb12c662ea3bd39767b69888d98 on a fresh branch. At
entry SP FFFE or FFFF the former linear physical-IP+2 CS read selected
the wrong word. The model now wraps the second word-pop offset to
0000 or 0001, matching original-byte DOSBox-X execution. The original
extent, branch behavior, predicates, flags and caller ABI are unchanged.

The new u33_both_outside_1403_wrap_native.py checks two original-byte
native cases at those SP boundaries, one true and one false predicate.
It compares all fourteen CPU words, 96 stack/sentinel bytes and sixteen
auxiliary sentinel bytes, with no masks. The wrapped CS word and a
distinct linear CS decoy are both primed. Flag-preserving dual return
trampolines record the actual returned CS before entering the shared
capture code; both cases must reject the linear decoy. Native entry AX
is 1234; AH is checked along with the predicate and exact CMP flags.

The COM reserves 64 KiB for its code/data, then obtains a separate
DOS-owned 0x1100-paragraph block for its stack and alternate return
trampoline. The proof primes and compares SS:FFE0..FFFF, SS:0000..001F
and (SS+1000):0000..001F, including saved BP, local result, return frame,
arguments and the linear decoy. CLI covers priming through capture;
DOS output resumes with the original SS and SP. No naturally reached
game frame or loader dispatch is claimed. Generated original
bytes/COM/assembly/results stay ignored under resources/.

Native wrap RESULT.BIN SHA256:
`f468e5a6e06f0db384ac7d22d21cfcbe897d1f81c88d1a80f446772850e392ee`.

The full-state oracle reuses one emulator, restoring every RAM byte and
all fourteen CPU words before each case. Hash-locked immutable code
stays at the same address. This removes repeated emulator setup after
the original per-case setup exceeded a 300-second run budget; comparisons
and coverage requirements remain unchanged.

```sh
cmake -S . -B resources/build-u33-1403-retf \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u33-1403-retf --target \
  poolrad_overlay_u33_both_outside_1403_tests poolrad_overlay_u33_byte_flags_08a4_tests
ctest --test-dir resources/build-u33-1403-retf \
  -R 'poolrad_overlay_u33_(both_outside_1403|byte_flags_08a4)_tests' --output-on-failure
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  timeout 300s python3 tools/overlay/u33_both_outside_1403_audit.py \
  --output resources/u33-1403-parity/retf-repair-audit.json
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  timeout 180s python3 tools/overlay/u33_both_outside_1403_native.py
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  timeout 180s python3 tools/overlay/u33_both_outside_1403_wrap_native.py
```

The bounded repair recall query
timeout 30s deja "Poolrad U33 3986c34 RETF stack CS pop FFFE FFFF native repair"
ended 124 without results. The proof adapts the independently reviewed
[U7F native RETF experiment](https://github.com/huntergdavis/poolrad-decomp/blob/0395ec049aaf0b8e53e2fa10fdcf6a3953e36271/docs/re/U7F_RECORD_SUBTRACT_0020.md).
Unicorn's disagreeing boundary states are documented and tested natively;
no expected CS is silently normalized. The original-byte fixture builder
also constructs return IP and CS with the proper separate word offsets.

The former U3C2 follow-on is reserved to pP, who also owns U3C6.
This repair claims no additional game identity or strict count.
