# U3C2 +0909 synthetic caller windows

`pr_u3c2_caller_windows_0909` models two argument-setup/CALL windows inside
U3C2's +0909 caller. Each window executes 24 instructions and stops at
+057B **before any callee instruction**. This is bounded synthetic evidence:
zero strict full-C credit, no natural/native frame, and no measured path from
the caller's entry to either window. The caller contains other dependencies.
The separately reviewed +057B prefix is not linked or executed here.

| Artifact | Measured identity |
| --- | --- |
| GAME.OVR | 206105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Caller +0909..+0BD8 | File `[197723,198443)`; 720 bytes / 297 instructions; SHA-256 `38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed` |
| Window +0A71..+0AA4 | 52 bytes / 24 instructions; SHA-256 `8f2e8a7f8d31d6e257b3be0471fcbf140fcd6a0c98ff455925750cb73cb2b344` |
| Window +0B73..+0BA6 | 52 bytes / 24 instructions; SHA-256 `0baa523f4b2a1dd5d0c083b9e623f446fffae163f6e9f424e949302a23ff4898` |

Both windows gather five byte arguments into AX and push five words, then
push CS and execute a near CALL to +057B. The model reads locals again after
each push, retaining effects when a pushed word aliases a later local or
indexed input. BP-based operands use SS, effective offsets and SP wrap at
16 bits, and word stores use contiguous physical bytes with A20 disabled.

At the stop, SP is its initial value minus 14, BP is unchanged, and the
stack contains the return IP, CS, and five arguments in reverse push order.
The return IP is +0AA5 or +0BA7. These are synthetic caller-created frames;
neither the callee prologue nor its return executes. In particular, after a
future callee PUSH BP, its BP+0E argument would be the first pushed byte.
This audit does not execute that step or assert the rest of its behavior.

Four XOR AH,AH instructions establish zero AH. XOR leaves AF architecturally
undefined: the interface deliberately chooses zero AF, matching pinned
Unicorn 2.1.4. All 14 CPU words are compared, including that convention;
there is no claim of native AF parity. Other defined status flags follow
XOR and non-status flags remain unchanged. Code must stay immutable and
asynchronous traps are excluded; invalid pointers, short RAM or other entry
IPs are rejected without mutation.

## Reproduction

With Capstone 5.0.6 and Unicorn 2.1.4 importable (the measured versions), run:

```sh
python3 tools/overlay/u3c2_caller_windows_0909_audit.py --quick \
  --output resources/u3c2-0909-prefix/quick-parity.json
```

The quick audit passes 924 cases (462 per window), comparing every byte of
1 MiB RAM and all 14 CPU words; all 48 window sites execute. Cases cover
every byte index, stack/effective-offset/A20 boundaries, stack/local aliases,
six initial flag states, all 52 code-byte read aliases per window, and four
seeded full-RAM inputs per window. Each case has a 25-instruction cap and
stops before the first callee instruction. No external calls execute, no
flags are masked, and no emulator substitutions are used.

The focused C test independently checks 216 full-RAM/CPU frame snapshots
and rejection behavior. C11 warning-clean ASan/UBSan validation passes.
The focused CMake target builds with `-Wall -Wextra -Wpedantic -Werror`;
`poolrad_overlay_u3c2_caller_windows_0909_tests` passes CTest (1/1).
Original bytes, private disassembly and oracle reports stay under ignored
`resources/`; the public tests contain synthetic data only.

## Parity review

- `target_function`: U3C2 +0909, restricted to the two windows above.
- `source_candidates`: `src/recovered/overlay_u3c2_caller_windows_0909.c`.
- `signals`: pinned original hashes, aligned near-call targets, five-word
  argument order, SS-based indexed reads, stack alias ordering, defined
  flags, unchanged registers, and whole-RAM original-byte comparisons.
- `score`: 48/48 window sites and 924/924 synthetic oracle cases; this is
  coverage of bounded windows, not a probabilistic full-function score.
- `verdict`: `match` for these windows under the documented preconditions
  and undefined-AF convention; whole-caller parity remains `unclear`.
- `review_notes`: the caller's earlier control flow and other calls remain
  unmodeled. No natural/native input, callee body, return path or strict
  function completion is demonstrated. Local aliases are tested; mutable
  instruction bytes and asynchronous exceptions are excluded explicitly.

Bounded `deja "poolrad U3C2 0909 0AA2 0BA4 caller bounded prefix"` produced
no reusable finding within its 15-second bound. Reused conventions come from merged
`u86_procvars_3413_audit.py` and `overlay_u6c_record_table_sum_3f31.h`; the
zero-XOR flag convention also exists in those merged recovered C models.
The separately reviewed `2235fc5` prefix informed the finite stop-before-
callee audit design; this branch has no code dependency on that commit.
