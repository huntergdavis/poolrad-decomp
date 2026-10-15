# U3C6 entry 8: both signed bytes within 0..6

`pr_u3c6_in_range_06e9` translates complete U3C6:+06E9..0719.
AL is one exactly when both signed argument bytes are within 0..6;
otherwise zero. AH is preserved. The raw-state model retains the original
comparison order and flags, local byte, saved BP and RETF 4 effects.
The current classification is helper_candidate/unknown. Natural reach,
verified game identity, strict-credit eligibility and merge remain separate
PM gates; no accounting/classifier files change.

## Source and caller pins

Owner inputs: START.EXE (65,231 bytes), SHA256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR (206,105 bytes), SHA256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3C6 file base is 198458; public entry slot 8 is +06E9 through
03E0:0048. Exact body file [200227,200276), 49 bytes / 18 instructions,
SHA256 `1495986874fc88753b5419d14380e9f7846b537a2b7015c57f7e24a1a761a7b9`.
All bytes decode, terminal +0717 is RETF 4. Four conditional sites have
eight edges, with one unconditional jump. No body calls, interrupts or
relocations exist; every branch stays inside the exact body.

Five raw near targets are aligned in callable bodies: +0127 site +019B,
+052B sites +0615/+0696, +071A site +07A4, and +0996 site +0AB2.
Each has PUSH AX; PUSH CS before CALL +06E9 and OR AL,AL after return.
One aligned, fixup-confirmed far caller is U5E:+0712 site +07F0 via
03E0:0048. The audit checks complete raw near/far-fixup caller sets.

The original +068D..0698 prefix loads AL from BP+0Ah, pushes AX,
loads AL from BP+8, pushes AX, then PUSH CS; CALL +06E9. Thus after
CALL the word at SS:SP+4 supplies byte a and SP+6 supplies byte b.
Both word upper bytes are ignored. The prefix retains AH in both words.
Other callers use caller-local/argument bytes and sums/differences; the
broader application meaning of the pair remains unverified.

## Complete state behavior

After PUSH BP and SUB SP,2, the b byte is compared with zero, then six
when nonnegative. A negative b or b greater than six returns zero without
reading a. Otherwise a is compared with zero and, if nonnegative, six.
Only a within 0..6 reaches the one-result store. All six arithmetic flags
come from the last executed byte CMP. None are masked or undefined here.
Deriving flags from the Boolean result would lose source behavior.

One byte at BP-1 holds the result; MOV AL preserves AH. MOV SP,BP,
POP BP and RETF 4 read live RAM, producing actual popped CS:IP and
final SP = entry SP + 8. Each RETF word pop wraps SP independently;
CS is read at SS:(entry SP+2) modulo 65536. Other general and segment registers remain
unchanged. Effective offsets/SP wrap16; contiguous operands and A20-off
RAM wrap20. Code must remain immutable and disjoint from writes. Invalid
pointers or RAM shorter than 1 MiB are rejected before mutation.

## Reproduction and evidence

```sh
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u3c6_in_range_06e9_audit.py \
  --output resources/u3c6-06e9-parity/full.json
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u3c6_in_range_06e9_native.py
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u3c6_in_range_06e9_native.py --wrap-returns
```

The full original-byte audit passes 4,083 whole-1-MiB/all-fourteen-CPU-word
cases; quick mode passes 363. Six SP=FFFE/FFFF cases use native-supported
wrapped CS expectations because Unicorn 2.1.4 reads that CS linearly.
Those six CS values are explicitly reported as substitutions, rather than
claimed as raw-Unicorn CS agreement. Complete RAM and the other CPU words
remain compared without substitution; all flags are exact. Both modes cover
all 18 instruction starts and eight conditional edges without flag masks.
The full matrix sweeps each byte against representative opposite inputs,
boundary pairs, ignored argument high bytes and preserved AH, 66 stack/
physical-wrap frames and 128 deterministic random whole-RAM backgrounds.
Every original invocation reaches RETF within 32 instructions, with no
budget exclusions. Capstone 5.0.7 / Unicorn 2.1.4 were used.

DOSBox-X normal-core 386 passes 1,024 synthetic cases executing the
original +068D..0698 prefix and complete leaf in a separately allocated
code segment. A transparent far jump captures original return +0699
before its OR AL,AL. Four complete byte sweeps use b=6, b=7, a=6 and
a=7, varying AH and non-arithmetic flags. Fourteen CPU words, all flags
and 64 frame bytes match C. Native RESULT.BIN SHA256:
`9f86b6215bc329ffdef25c99d2c27b106dccf2263a2a495f1411057da508f0e9`.
This native capture compares the frame region, rather than the entire
native RAM. Actual loader dispatch and natural game calls are unmeasured.

A separate 16-case native gate executes the same original prefix with
entry SP=FFFE and FFFF across eight predicate inputs each. A 128 KiB
program allocation keeps linear-shadow bytes above SS:FFFF in private
program memory. Fourteen CPU words, exact flags and 104 bytes match C:
SS:FFB0..FFFF (80), SS:0000..0007 (8) and physical shadow bytes at
SS base+10000h..1000Fh (16). The shadow is deliberately distinct from the
wrapped CS word. RESULT.BIN SHA256:
`b61fb3f0802cc1436538c98ac2a7162269e37d35423b818376027aaee147d834`.
The pre-fix head 2445a19 returned CS=A5A5 in all 16 cases while native
returned the loaded code segment; the other CPU/frame observations matched.
The corrected CS pop and an independent two-frame C regression close this
edge. The previous whole-matrix wrap pass was insufficient because both
its return-frame setup and Unicorn used a linear CS assumption.

The focused C test exhausts all 65,536 signed-byte pairs against an
independent mathematical predicate. It varies preserved AH, uses unrelated
argument high bytes and checks exact flag vectors, local/saved frame,
return cleanup, guards and unchanged registers. All 246 CTests pass after a -Wall/-Wextra/-Wpedantic/-Werror build.
Shell syntax, research checksums, staged whitespace and public-media hygiene
pass. Extracted originals, disassembly, generated COM files and captures
stay ignored under resources/.

- `target_function`: U3C6 public entry 8, +06E9..0719.
- `source_candidates`: new uniquely named raw-state range predicate.
- `signals`: source/extent/public-entry pins, aligned near/far callers,
  exact CMP flags, original whole-RAM/CPU parity, exhaustive synthetic
  truth table and separate native original-prefix execution.
- `score`: 18/18 instruction starts and 8/8 edges; no game denominator.
- `verdict`: match within the immutable-code/raw-frame contract.
- `review_notes`: broader argument meaning, natural reach, verified
  identity and strict accounting remain separate gates.

Bounded deja found no completed finding. Reused findings: the measured
ranked-ledger row (rank 446, grade A), the pinned selection dossier,
[merged CPU/offset conventions](U6C_RECORD_TABLE_SUM_3F31.md) and the
independently reviewed [U33 frame/flags and native-capture design](https://github.com/huntergdavis/poolrad-decomp/blob/3986c34c00232eb12c662ea3bd39767b69888d98/docs/re/U33_BOTH_OUTSIDE_1403.md).
PM reserved this target in inbox i3713 and released it in i3728 after U3C2
review. No unmerged symbol is required. The CMake insertion shares an area
with seven pending branches; integration must preserve every registration.
