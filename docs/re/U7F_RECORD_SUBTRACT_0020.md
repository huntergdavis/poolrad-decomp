# Unit 0x7F +0x0020 record subtraction

`pr_u7f_record_subtract_0020` translates the complete call-free
`+0x0020..+0x0039` routine. It subtracts a word argument from the word
at far-record displacement `0x0102`, returning the difference in AX
and retaining the exact SUB16 flags. The raw-RAM model includes the
saved BP, ordered argument/record accesses and far return.

## Prior work and game ownership

The bounded query
`timeout 30s deja "Poolrad U7F 0020 record 0102 subtract flags full C ownership pP"`
returned no indexed results. Current-main recovery inspection found
no implementation of this leaf. PM explicitly reserved it separately
from pP's adjacent `+0x003A..+0x0053` addition helper. Neither that
leaf, its nine callers nor pP's U33 `+0x1403` is edited.

Three aligned near calls belong directly to descriptor public entries:

| Public entry / owner | Call site | Argument pushes before PUSH CS/CALL |
| --- | --- | --- |
| 7 / `+0x044C` | `+0x04AD` | `+04A2` ES:[DI+2] segment; `+04A6` ES:[DI] offset; `+04A9` SS:[BP+8] amount |
| 2 / `+0x0508` | `+0x05A8` | `+0591` SS:[BP-2] segment; `+0594` SS:[BP-4] offset; `+05A2` ES:[DI+88] amount after indexed address calculation |
| 8 / `+0x0A01` | `+0x0A33` | `+0A29` SS:[BP+0C] segment; `+0A2C` SS:[BP+0A] offset; `+0A2F` SS:[BP+8] amount |

The oracle pins these aligned pushes, calls and public entries.
Entry 2 loads the game party head from DS `0x5D96/0x5D98` at
`+0x050E/+0x0511`. It visits party records, reads indexed words
at record `+0x88`, adds them to DS `0x67F4/0x67F6` dwords and
passes that record and word to this subtraction helper. Entries 7
and 8 also update record `+0x88` words before invoking it.
The existing [U7F game-state recovery](U7F_STATE_FLAGS_0F7E.md)
independently documents these game-owned party/resource structures.
This establishes game ownership without inventing a meaning for
record field `+0x0102`. It is static callable ownership, not a
naturally reached saved-game call frame.

## Exact original and full ABI

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x7F | GAME file base `0x24C89`; public stub segment `0x008F` |
| Function | GAME file `0x24CA9..0x24CC2`; **26 bytes, 9 instructions, zero calls/branches** |
| Function SHA-256 | `f1e091b372c0401a7b90dac9e6a32f12c64b898c8f0039a69df380669e24e13a` |
| Exit | `RETF 6` at `+0x0037`; end `+0x003A` exclusive |

The API enters after CALL. Return IP/CS are at SS:entry-SP; amount
is at entry-SP+4, record offset at +6 and record segment at +8.
The routine pushes BP and reserves no local storage. It then:

1. Loads the live far record argument with LES.
2. Reads the word at ES:[DI+0102], then the amount word from SS:[BP+6].
3. Subtracts modulo 65536, retaining the difference in AX and all flags.
4. Reloads the live far record argument with LES and writes AX to
   ES:[DI+0102].
5. Pops BP, reads return IP at SS:entry-SP, then reads CS at
   SS:((entry-SP+2) modulo 65536). RETF performs separate word pops:
   offset wrapping occurs between IP and CS. Final SP is entry SP + 10
   modulo 65536.

The initial PUSH BP can change an aliased record word before it is
read. The write can change saved BP, return IP/CS, amount or pointer
argument bytes. Both LES loads occur before the record write; DI and
ES retain the reloaded record address even if the write then changes
the argument. DI retains the base offset, not base+0102.
BX, CX, DX, SI, DS and SS are preserved. BP, CS and IP follow their
live return-frame reads.

CF, PF, AF, ZF, SF and OF are all defined by SUB16 and survive the
subsequent LES/MOV/POP/RETF. CF is the unsigned borrow, PF the low
result byte's even parity, AF the low-nibble borrow, ZF/SF the word
result, and OF the signed subtraction overflow. Other flag bits are
preserved. No flags are masked or normalized.

Effective offsets wrap at 16 bits; word/LES/RETF reads and word
stores cross offset boundaries contiguously with A20-off physical
wrapping at 1 MiB. Instruction bytes must remain immutable; the
original-byte oracle verifies that condition before C runs.
Null RAM/CPU or any RAM size other than exactly 1 MiB is rejected
before mutation. There are no callbacks, source loops, runtime caps
or unresolved dependencies inside this complete leaf.

## Validation and parity review

The independent C11 test passes **524,288 arithmetic cases** plus
**three native-backed RETF regressions** at SP FFFD/FFFE/FFFF: every record word
under seven boundary amounts and an XOR-derived amount that itself
covers every word value. It checks the result/sentinels, every CPU
word, mathematically derived flags, saved BP, far return and invalid
host arguments. Both states of each of the six SUB flags are required.
The new test and neighboring U7F state-flags test pass with
`-Wall -Wextra -Wpedantic -Werror`.

The original-byte audit passes **558 cases**, each comparing the
entire 1 MiB RAM and all 14 CPU words. It covers all nine instruction
starts and all eight internal fallthrough edges; there are no branches.
Cases include 256 word-boundary pairs, 256 random inputs/record offsets,
stack/record offset wrap, A20-crossing words, record aliases with
saved BP/return/argument bytes, incoming arithmetic flags and DF.
Every original case returns with the original body unchanged before
the candidate runs.

Unicorn reads RETF's second word linearly at entry SP FFFE/FFFF for
these fixtures. Those two cases are excluded from the 558 Unicorn
full-state comparisons and tested with the native proof below.
No register is masked or normalized to hide that emulator difference.

The DOSBox-X normal-core/386 harness passes **512 synthetic cases**:
256 boundary pairs and 256 seeded random word pairs, with aligned
and unaligned record fields. It compares all 14 CPU words, 32
frame/sentinel bytes and 16 record/sentinel bytes. Both states of
each SUB flag are checked. No flags or compared RAM bytes are masked.
The native result SHA-256 is
`f351d721aaa18923db1d1e0de696d59c6e4e73b9f73aa259e876c3a2d73f0df6`.
Its header contains a DOS allocation segment; another setup may
change the hash while retaining parity.

A separate DOSBox-X proof passes **two SP=FFFE/FFFF cases**, comparing
all 14 CPU words, 96 stack/sentinel bytes and 16 record/sentinel bytes.
Each frame contains the original CS at the wrapped second-pop offset
and a distinct decoy CS at the linear physical IP address plus two.
Two flag-preserving return trampolines record the actual returned CS
before returning to the capture code. Both cases select the wrapped
word; no flags or compared bytes are masked. The result SHA-256 is
`4cf72ddaa8c972c24f0d46c523f0d39c280ebd631b113bc52f45705bce981924`.

The wrap harness shrinks its COM allocation to 64 KiB, then obtains
an additional DOS-owned 0x1100-paragraph block for the synthetic stack,
alternate return trampoline and linear decoy. It primes and compares
SS:FFE0..FFFF, SS:0000..001F and (SS+1000):0000..001F. The stack and
both return paths are inside owned, disjoint allocations.

The stack-wrap debugging recall query
`timeout 30s deja "Poolrad RETF SP FFFE FFFF stack wrap CS separate pop Unicorn DOSBox"`
ended 124 without results. The PM's adversarial gate and original-byte
native experiment supplied the correction. The C model uses separate
16-bit offsets for the two RETF reads.

The synthetic native frame enters with SP EFF6: return IP at EFF6,
CS at EFF8, amount at EFFA, record offset at EFFC and record segment
at EFFE. Saved BP is at EFF4; final SP is F000. Record fields lie
inside the primed `9100..910F` sentinel block. Code/data below 2600,
record block and frame EFE0..EFFF are proven disjoint. Interrupts
remain disabled from priming through capture, with entry FLAGS 0002,
and are restored for DOS output. The harness copies only the
hash-locked leaf, not an unrecovered caller.

The model reuses the [U7F raw-RAM conventions](U7F_STATE_FLAGS_0F7E.md).
The oracle adapts the
[reviewed U33 whole-state/native harness](https://github.com/huntergdavis/poolrad-decomp/blob/0712614e7426339c1c0d9579e6ab47889259367a/docs/re/U33_PAIR_WRITER_0B97.md),
[party reorder compilation/CPU utilities](U6A_PARTY_BEFORE_14A9.md),
[unit 0x5E RAM helpers](U5E_PARTY_VALUE_2879.md) and
[U7D interrupt isolation](U7D_CARRY_02F2.md).
Original bytes, generated assembly/COM, results/logs and build output
remain ignored under `resources/`; public fixtures are synthetic.

These measurements do not establish a naturally reached saved-game
call/return frame, loaded game-unit segment or full-game replay parity.
No classifier or count ledger is changed; merge and credit remain
PM decisions.

```sh
cmake -S . -B resources/build-u7f-0020 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u7f-0020 --target \
  poolrad_overlay_u7f_record_subtract_0020_tests poolrad_overlay_u7f_state_flags_0f7e_tests
ctest --test-dir resources/build-u7f-0020 \
  -R 'poolrad_overlay_u7f_(record_subtract_0020|state_flags_0f7e)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn timeout 180s \
  python3 tools/overlay/u7f_record_subtract_0020_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x7F:+0x0020..+0x0039` |
| `source_candidates` | `src/recovered/overlay_u7f_record_subtract_0020.c` |
| `signals` | Exact hash/extent, three aligned direct public-entry argument frames, party-record ownership, independent word/flag cases, full-RAM/CPU original parity and native execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the immutable-code, A20-off RAM/CPU contract |
| `review_notes` | Defined SUB flags survive; separate wrapped RETF pops are proven natively; PUSH BP and record-write aliases affect live returns; native caller is synthetic; pP's adjacent leaf untouched |
