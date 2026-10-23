# Unit 0x5E +0x0E52 byte half helper

`pr_u5e_half_0e52` translates the complete call-free DOS v1.3 routine
`+0x0E52..+0x0E7A`. It conditionally increments a byte according to the
low bit of DS `[0x6DE3]`, then halves it. The increment wraps before
division: argument 255 and an odd flag return zero, rather than 128.

Before decoding, `deja "u5e"` recovered Codex history `01a12855-add`,
which identifies prior head `d388495f74f1b2ca0ad5b7bc25861154c795e36c`.
`git show` confirmed that it implements the distinct
[+0x2879 party-value routine](U5E_PARTY_VALUE_2879.md). This work reuses
its 1 MiB RAM/CPU audit helpers and descriptor parser, but adds a separate
function, API, tests, and evidence. The PM redirected this lane after
unit 0x9B's four existing translations exhausted its game-specific code.

## Source and ownership

| Artifact | Measured bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x5E | GAME file base `0x127FB`, stub segment `0x0068` |
| Routine | GAME file `0x1364D..0x13675`; **41 bytes, 17 instructions, one conditional branch, zero calls** |
| Routine SHA-256 | `c99969f9b7b28b6bdd15297e8b711a8491b605df8055135eb114b6215dfc82a2` |
| Exit | `RETF 2` at `+0x0E78`, ending at `+0x0E7B` |

The prologue inventory gives a 48-byte upper bound ending at `+0x0E82`.
The final seven bytes are an inline constant following the return,
not additional instructions on a reachable path. There is no branch
into them. The only conditional edge is `+0x0E63 -> +0x0E68`, which
skips the byte increment when the flag is even. This is the smallest
untranslated call-free body found in this unit, excluding its empty
frame and the translated `+0x2879` routine.

Entry 6 at `+0x0D23` has an instruction-boundary-confirmed `PUSH CS;
CALL +0x0E52` at `+0x0DBE/+0x0DBF`. It passes DS `[0x681A]`, stores
the returned byte in an SS local, and conditionally writes it to the
same character record's `+0x113` field at `+0x0E47`. That enclosing
body also reads character fields `+0xA1`, `+0xCC`, and `+0x108`.
Three segment-fixup-confirmed far calls reach entry 6:
unit `0x2F:+0x03C4/+0x0410` and unit `0x31:+0x1818`, each targeting
`0068:003E`. The source audit checks those fixups and instructions,
rather than accepting a raw E8 byte scan as a caller.

DS `[0x6DE3]` is also read by the verified
[unit 0x38 entry-74 effect routine](OVERLAY_UNIT38_EFFECT_ENTRIES.md).
Together, the character-record caller and shared game-state byte
establish application ownership beyond the generic arithmetic shape.
The flag's gameplay meaning and saved-game reachability remain unmeasured.

## Complete ABI and ordered effects

The API starts after the call, with return IP/CS at SS:SP and the
argument word at SS:SP+4. Only its low byte is used. It performs:

1. Push old BP, establish BP, reserve two local bytes.
2. Read `[BP+6]` and write local byte `[BP-2]`.
3. Read DS `[0x6DE3]` **after** those writes. If its low bit is set,
   increment `[BP-2]` modulo 256.
4. Reload the local, zero AH, shift AX right once, and copy AL through
   `[BP-1]` before returning it.
5. Restore BP; read the four-byte far-return frame contiguously; consume
   the return frame and argument, leaving SP advanced by six bytes.

The saved BP and both local bytes remain in RAM after return. DS globals,
arguments, return words, and stack locals share the same A20-off RAM,
so a DS global that aliases a local sees the ordered write. Effective
offset arithmetic wraps to 16 bits. Multi-byte PUSH/POP/RETF accesses
cross an offset boundary contiguously, with physical addresses masked
at 1 MiB. In particular, RETF's CS word must not be fetched by wrapping
SP+2 separately; the boundary differential case detects that mistake.

BX, CX, DX, SI, DI, DS, ES, and SS are preserved. AX holds a zero-extended
result; BP, SP, CS, and IP follow the actual frame. The final SHR defines
CF from the adjusted byte's low bit, PF from result parity, and ZF from
zero; SF/OF are zero. Other defined flags are preserved. SHR leaves AF
undefined: C represents it as zero, matching Unicorn; DOSBox-X can set
it. The native comparison masks **only AF**. The enclosing caller's
next arithmetic instruction is CMP at `+0x0DC5`, which overwrites these
arithmetic flags before any conditional branch uses them.

The sole host guard rejects null RAM, non-1-MiB RAM, or null CPU state
before mutation. There are no external hooks, core-only approximations,
loop limits, or unresolved callees.

## Validation and parity review

The C test enumerates all 65,536 argument/flag-byte combinations and
checks overflow, preserved registers, stack remnants, segment/physical
wrapping, and invalid host arguments. The original-byte differential
compares the entire 1 MiB RAM and every returned CPU word, including
stack/global aliases and RETF boundary cases. The separate DOSBox-X
oracle copies the hash-locked leaf into an ignored synthetic COM and
compares all returned registers, defined flags, ten frame bytes, the
argument, and the global for 512 input/low-bit combinations.

The DOSBox-X oracle is a synthetic native execution of the original
leaf, **not a live saved-game capture**. It does not establish that the
saved route reaches this routine. Its copied original code, assembly,
COM, log, and binary results remain under ignored `resources/`.

Measured on 2026-10-14: **791 original-byte/C cases passed**, comparing
all 1 MiB of RAM and all 14 returned CPU words. **512 DOSBox-X cases
passed** for returned registers, defined flags, frame RAM, argument, and
global. The ignored native result file has SHA-256
`d20587cbc34d41955d3cd100622edbd1976f1c1d87e3ec125d4e5c30af33effe`.
Its header includes the DOS allocation segment, so a different emulator
environment can produce a different result hash while retaining parity.
The complete project built with `-Wall -Wextra -Wpedantic -Werror`;
all **178 CTest targets passed**, including both unit 0x5E targets and
the 65,536-case new test.

Reproduce with local Capstone 5, Unicorn 2, GNU binutils, and DOSBox-X:

```sh
cmake -S . -B resources/build-u5e-0e52 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u5e-0e52
ctest --test-dir resources/build-u5e-0e52 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u5e_half_0e52_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x5E:+0x0E52..+0x0E7A` |
| `source_candidates` | `src/recovered/overlay_u5e_half_0e52.c` |
| `signals` | Hash-locked body; complete CFG/ABI; ordered whole-RAM differential; synthetic native oracle |
| `score` | 4/4 independent checks passed; a checklist, not a probability |
| `verdict` | `match` for defined routine behavior under the documented RAM/CPU contract |
| `review_notes` | AF is undefined and differs between emulators; saved-game reachability unmeasured; prior +0x2879 translation excluded |

After independent parity review and PR #383 merge
`fb342dbef3b38fcd33c88e907f0c2dda69316948`, the PM approved exactly one new
verified identity and one strict full-C original for `+0x0E52` (inbox i2920).
The separate [accounting refresh](FUNCTION_ACCOUNTING_REFRESH.md) updates the
classifier and credit manifest. Saved-game reachability remains unmeasured.

## Pinned AF after the final SHR: model defect fixed (2026-10-23)

par-b measured route 3 at 269/269 `C_DIFFERS`, AF only (C `7202` vs original `7212`; `7246` vs `7256`). The body ends `zero AH, SHR AX`, and the model wrote `flags = (flags & ~08D5h) | CF` then PF/ZF, which **cleared** AF; pinned DOSBox-X sets AF after a non-zero SHR (`get_AF` = `count & 1Fh`, here 1), and that value reaches the RETF. The audit had passed only because raw Unicorn also clears AF, and its native run **masked AF** (`0xFFEF`, 'SHR AF is undefined').

Fix (PR after #1450): the model sets AF after the SHR; the C test asserts OF/SF clear and AF set; the Unicorn differential uses the shared `pinned_shift_flags` split (AF set after a non-zero shift, every other bit exact); the native run now compares **full FLAGS, no mask**. Runs: Unicorn **791** cases, split used at all 791 returns (each follows the SHR); DOSBox-X native **512/512** with AF included. Mutation check: with the AF line removed, the unmasked native run fails at case 0 (`C=0246, DOSBox-X=0256`).

**Per-credit sentence (OVL_005E_0E52): the credit's evidence previously did not cover AF** (Unicorn agreed with the defect, and native masked it). It now does: the model matches pinned DOSBox-X in every FLAGS bit on 512 native cases. Credit stands on the corrected model.
