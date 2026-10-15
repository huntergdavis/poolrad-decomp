# Unit 0x7F public entry 8: ordered resource subtraction

`pr_u7f_resource_subtract_0a01` recovers the complete original owner and
uses the approved `pr_u7f_record_subtract_0020` dependency. This is a
candidate for PM review; this change does not edit classification or counts.

| Evidence | Measurement |
| --- | --- |
| START.EXE SHA256 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR SHA256 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Owner | unit `0x7F`, public entry 8, `+0x0A01..+0x0A7C` inclusive |
| GAME file span | `0x2568A..0x25705` inclusive |
| Complete body | 124 bytes, 49 instructions, `RETF 8` at `+0x0A7A` |
| Body SHA256 | `3e39eb5fa8ac9c4fa437e885b9a452cd5a7913ee3a3f2cfbefafde2d0f543aa2` |
| Calls | one near call, `+0x0A33 -> +0x0020`; no external runtime calls |
| Closed callee | `+0x0020..+0x0039`, 26 bytes, 9 instructions, `RETF 6` |
| Callee SHA256 | `f1e091b372c0401a7b90dac9e6a32f12c64b898c8f0039a69df380669e24e13a` |
| Natural aligned caller | U77 owner `+0x24A7`, far call `+0x270C`, GAME file `0x23746` |
| Caller relocation | segment word at U77 `+0x270F` is an overlay fixup, target stub `0x8F:0x0048` |

The caller pushes live DS words `0x5D94`, `0x5D92` (record segment and
offset), amount from `[BP-0x12E]`, and index AL from `[BP-2]`. The oracle
finds this call by decoding from its function prologue and checking the
encoded far target, relocation, and exact argument pushes. A name match is
not used as identity evidence. The owner is disjoint from the separately
owned `+0x003A` helper and its callers.

## ABI and natural effects

At entry after CALL, SS:SP contains return IP, SP+2 return CS, SP+4 index
word (only its low byte is used), SP+6 amount word, and SP+8/SP+10 the
record far pointer. The index is sign-extended from AL, then doubled with
16-bit wrapping. The original subtracts amount from record word
`+0x88 + 2*signed(index)`, then calls `+0x0020` to subtract amount from
record word `+0x102`. At index 61 these are the same word, so the two
ordered subtractions both occur.

After the callee actually returns, DS byte `0x49F3` selects the final
effect. Modes 1 and 6 add the zero-extended amount to the 32-bit DS value
at `0x67F4 + 4*signed(index)` using ADD/ADC. Other modes skip the addition.
The original rereads arguments and far pointers at their original instruction
positions; the C does too. It does not collapse aliased record/argument
writes into an unordered mathematical expression.

Normal return restores BP and finishes with SP equal to entry SP+12.
BX/CX/DX/AX/DI are modeled according to the executed branch, including the
two final shifts and CBW. Every CPU field is exposed. Memory is exactly
1 MiB with A20 disabled; effective offsets wrap to 16 bits, while the bytes
within a word or LES operand are consecutive physical bytes modulo 1 MiB.
Original instruction bytes must remain immutable.

## Nested return frames and transfer boundaries

The near call uses `PUSH CS; CALL +0020`. Its return IP is at entry
SP-12 and its CS at SP-10. The record's second write can alias either word.
The C resumes the callee's actual IP/CS, including recovered instruction
boundaries inside the owner and helper. It does not assume return to
`+0x0A36`. A changed CS or a target outside recovered instruction boundaries
returns `CONTROL_EXIT` with the exact CPU/RAM state at that transfer, for
the host dispatcher. This boundary is explicit, rather than silently
continuing the natural owner path or inventing external code behavior.

Both RETF instructions pop IP and CS as separate word operations, wrapping
SP between the pops. Native cases cover outer SP `FFFE/FFFF` and outer SP
`000A/000B` (inner SP `FFFE/FFFF`). Contradictory linear-CS decoys are
present. Additional native fixtures overwrite the inner IP to `+0x0A79`
and overwrite its CS to two distinct segments with transparent capture
trampolines. They check the resulting natural effect or control exit.

## Flags and independent checks

The final SHL has an architecturally undefined AF bit for its nonzero
count, as specified in the SAL/SAR/SHL/SHR entry of
[Intel's instruction reference](https://cdrdv2-public.intel.com/782151/253667-sdm-vol-2b.pdf).
DOSBox-X normal-core 386 sets AF in every one of the 518 native ordinary
and wrap fixtures ending in the shift; Unicorn clears it. The C pins this
undefined bit to the measured native value. Its public header declares the
choice. It is not claimed to be a portable hardware guarantee.

The whole-RAM oracle excludes only this one architecturally undefined bit
when SHL is the last flag-writing instruction, reports the number of such
cases, and requires all other CPU bits to match. It does not patch the
original instructions or emulator state. Native comparison checks the
entire FLAGS word without a mask. The separately wrapped RETF discrepancy
in Unicorn is handled by explicit native-only fixtures, not by substituting
the emulator's wrong return CS into the recovered source.

Validation includes 65,536 independent index/mode arithmetic cases; all
256 index and mode bytes; overlapping record fields; 32-bit carry and wrap;
stack, argument, record, DS and A20 aliases; redirected returns; all 58
owner/callee instruction starts; and both edges of both owner conditionals.
The full original-byte oracle compares every byte of 1 MiB and all CPU
fields, subject to the sole AF exception above. Native verification uses
780 original-byte executions, all 14 CPU words and 2,592 bytes covering
every execution write footprint plus sentinels. The C additionally must
leave every RAM gap outside those observed blocks unchanged.

Native output SHA256:
`b5c6632ecdd4f810b7f2f3c605ae535f6f564e7af46ffb59e1d03ae69d9c3297`.
All original media, sparse code images, COM programs, generated assembly,
and captures remain under ignored `resources/`. Public tests are synthetic.

```sh
cmake -S . -B /tmp/poolrad-u7f-0a01-build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-u7f-0a01-build --target poolrad_overlay_u7f_resource_subtract_0a01_tests poolrad_overlay_u7f_record_subtract_0020_tests -j 4
ctest --test-dir /tmp/poolrad-u7f-0a01-build -R 'poolrad_overlay_u7f_(resource_subtract_0a01|record_subtract_0020)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u7f_resource_subtract_0a01_audit.py --unicorn
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 tools/overlay/u7f_resource_subtract_0a01_audit.py --native
```

## Adversarial parity review

- `target_function`: U7F public entry 8 `+0x0A01..+0x0A7C` and its approved +0020 dependency.
- `source_candidates`: `overlay_u7f_resource_subtract_0a01.c`, with `overlay_u7f_record_subtract_0020.c`.
- `signals`: exact hashes, bounded return, aligned relocated natural caller,
  live game-pointer frame, ordered writes, closed callee, whole-RAM oracle,
  independent arithmetic, native full CPU/FLAGS and both RETF-wrap proofs.
- `score`: 9/10 pending independent PM review of scope and the explicit transfer interface.
- `verdict`: `likely_match` within the declared immutable-code and transfer-boundary contract.
- `review_notes`: changed inner return targets are captured rather than
  ignored; undefined AF is a visible backend disagreement with independent
  native evidence; external or unaligned targets require host dispatch.
  Synthetic native entry frames reproduce the pinned game caller's ABI;
  this is not a claim of a saved-game live replay through U77.

Bounded `deja` recall for U7F +0A01 and candidate ownership returned timeout
124 without results. Reused repository evidence is the approved +0020
source/audit, `overlay_inventory.find_units`, and the synthetic allocated
stack/trampoline approach of the approved native wrap audit. The owner
bytes, natural caller and native effects were independently rechecked.
