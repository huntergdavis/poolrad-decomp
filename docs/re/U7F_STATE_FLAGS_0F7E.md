# Unit 0x7F entry 9 +0x0F7E state flags

`pr_u7f_state_flags_0f7e` translates the complete call-free public
entry 9 `+0x0F7E..+0x0FD1`. For disjoint state it writes two byte flags:
whether any of the seven dwords at DS `0x67F4..0x680F` is nonzero,
and whether the head pair at DS `0x6810/0x6812` is nonzero.
The algorithm retains the original output order and shared-RAM aliases.

The required bounded recall
`timeout 30s deja "Poolrad U7F 0F7E entry 9 full RAM CPU translation callers shared_dwords"`
ended 124 without results. Inspection of current-main recovered
sources found no independent translation of this entry. Reused
evidence is the [seven-dword block audit](U7F_DIV5_DWORDS_ENTRY11.md),
the [ECL CLEARMONSTERS state map](U1C_CLEARMONSTERS_125D.md), and the
existing [full-state memory/oracle primitives](U7D_CARRY_02F2.md).
The new source/header/test/oracle are separate files.

## Exact original, callers and game ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x7F | GAME file base `0x24C89`, public entry 9 through stub `0x008F:0x004D` |
| Function | GAME file `0x25C07..0x25C5A`; **84 bytes, 30 instructions, zero calls** |
| Function SHA-256 | `5db57d103e9038273938138f1980a39a0af093ac1b811d8e50fa178f5371c51b` |
| Exit | `RETF 8` at `+0x0FCF`; exact end `+0x0FD2` exclusive |

Every original branch stays within the 84-byte body. The audit verifies
the public-entry descriptor and finds exactly seven fixup-backed far
calls to the entry's stub. Each site is also decoded in alignment
from its owning routine's prologue:

| Unit and owning routine | Call site | GAME file offset |
| --- | --- | --- |
| `0x1E:+0x0D09` | `+0x0D4A` | `0x052C1` |
| `0x1E:+0x0D09` | `+0x0E1E` | `0x05395` |
| `0x20:+0x0D38` | `+0x0DF2` | `0x06370` |
| `0x20:+0x0ECD` | `+0x0EF1` | `0x0646F` |
| `0x20:+0x0ECD` | `+0x10A4` | `0x06622` |
| `0x23:+0x053E` | `+0x05C9` | `0x0729D` |
| `0x23:+0x053E` | `+0x068D` | `0x07361` |

The block audit independently ties the same 28 bytes to active-character
state changes and public entry-1 routines in units `0x1E` and `0x23`.
The mapped ECL handler clears exactly that block and traverses the
separate head at `0x6810/0x6812`. These game-owned globals and callers
establish this entry's ownership. No gameplay name for either flag
or individual dword is inferred. The routine has no external callee,
callback or untranslated dependency.

## Complete ABI and ordered effects

The API enters after CALL. SS:entry-SP+4 holds the far head-output
pointer; SS:entry-SP+8 holds the far shared-dwords-output pointer.
The routine saves BP and reserves two bytes. Its byte index lives at
`[BP-1]`; the other local byte is untouched unless aliases change it.
`RETF 8` consumes the return frame and both arguments, so final SP is
entry SP + 12 modulo 65536.

It first loads the shared-output argument and stores byte zero, then
loads the head-output argument and stores zero. It initializes the RAM
index to zero. Each iteration sign-extends the current index, multiplies
it by four with 16-bit wrapping, and ORs the corresponding two DS words.
A nonzero dword reloads the shared-output argument and stores one.
The loop compares the freshly reread RAM index with six and otherwise
increments that byte. It does not replace the original with a fixed
host loop or an early exit after the first nonzero dword.

After the loop, AX becomes the OR of the two head words. A nonzero
result reloads the head-output argument and stores one. Earlier output
writes can change later arguments, inputs, the RAM index, saved BP or
return frame. The two outputs may also overlap. All reloads and writes
remain in source order; directed cases exercise these effects.

BX, CX, DX, SI, DS and SS are preserved. BP, IP and CS reload from RAM;
AX retains the head OR, and DI/ES follow the final reached loads.
Returned arithmetic flags come from that final 16-bit OR: CF and OF
clear; PF, ZF and SF follow its result. Logical AF is undefined and
represented as zero; both tested engines agree exactly without a
flag mask. Other flag bits are preserved.

Effective offsets wrap at 16 bits. Words, LES operands and the four-byte
return frame cross offset boundaries contiguously; physical addresses
wrap at 1 MiB with A20 disabled. Null RAM/CPU or a size other than
exactly 1 MiB is rejected before mutation.

An output alias can keep resetting the index and prevent source return.
The C model retains the original loop without a host cap. Its parity
claim covers terminating source paths. The directed all-nonzero
shared-output-to-index fixture fails the original's 20,000-instruction
return budget; uncapped C is not called on it. The terminating
counterpart with only dword zero nonzero is compared. The budget
failure alone does not prove nontermination for arbitrary states.

## Validation and parity review

The independent C11 test checks **1,536 cases**: every seven-dword mask,
six head forms (including zero offset or segment alone, sign and parity
bits), and separate/overlapping output pointers. It checks the output
flags, unchanged disjoint inputs, returned AX/flags, preserved state,
far return and invalid host arguments. The new test and existing
party-count test both pass with `-Wall -Wextra -Wpedantic -Werror`.

The original-byte oracle passes **389 returning cases**, comparing
the entire 1 MiB RAM and all 14 CPU words. These cover all 30 instruction
starts and both outcomes of all three conditional branches. Fixtures
include every mask/head presence pair, random words, stack boundaries,
A20 wrapping, overlapping outputs, output/input/global/argument/frame
aliases and preserved flags. The one original return-budget failure
is checked separately and does not contribute to returning coverage.

The DOSBox-X normal-core/386 synthetic oracle passes **256 cases**,
every mask/head presence pair, with exact comparison of all 14 CPU
words, 32 frame/sentinel bytes, all seven dwords, the head pair and
both output bytes. No flag or compared RAM byte is masked. The result
SHA-256 is `68e6fe3174f768eb23f9b94d43b2ba51691111cd97ed3216fc79ca8e47bee356`.
Its header includes a DOS allocation segment, so another setup may
produce a different result hash while retaining parity.

Native execution uses a copied hash-locked leaf in a synthetic COM.
Maskable hardware interrupts are disabled from frame priming through
snapshot capture; entry FLAGS is `0x0002`. Interrupts are restored
before DOS file output. This isolates the leaf's stack writes using
the measured [U7D interrupt boundary](U7D_CARRY_02F2.md); it does not
mask any returned state.

The seven aligned original callers establish callable ownership.
They do not establish a naturally reached saved-game call/return
frame. No natural-route, loaded-game-unit segment or full-game replay
parity is claimed. Original bytes, generated assembly/COM, native
logs/results and build files remain under ignored `resources/`.
Public fixtures are synthetic. Compilation/CPU helpers reuse the
[party reorder oracle](U6A_PARTY_BEFORE_14A9.md) and
[unit 0x5E utility](U5E_PARTY_VALUE_2879.md); the directed differential
and native snapshot mechanics adapt [U7D's oracle](U7D_CARRY_02F2.md).

```sh
cmake -S . -B resources/build-u7f-0f7e \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u7f-0f7e --target \
  poolrad_overlay_u7f_state_flags_0f7e_tests poolrad_overlay_u7f_party_count_05e0_tests
ctest --test-dir resources/build-u7f-0f7e \
  -R 'poolrad_overlay_u7f_(state_flags_0f7e|party_count_05e0)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn timeout 180s \
  python3 tools/overlay/u7f_state_flags_0f7e_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x7F` public entry 9 `+0x0F7E..+0x0FD1` |
| `source_candidates` | `src/recovered/overlay_u7f_state_flags_0f7e.c` |
| `signals` | Exact hash/CFG, seven aligned fixup-backed callers and mapped game state, whole-RAM/CPU differential, independent output tests and native execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` for terminating source paths under the documented RAM/CPU contract |
| `review_notes` | Original return-budget failure excluded from C/coverage; logical AF represented as zero; native IRQ isolation; natural saved-game route unmeasured |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision. U33 `+0x0B97` stays a separate follow-on.
