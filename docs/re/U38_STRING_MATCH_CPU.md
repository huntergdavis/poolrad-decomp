# U38 +0679 and +0947: raw-CPU string-match handlers (strict, composed)

`pr_u38_string_match_0679_cpu` is a raw-CPU, full-state model of GAME.OVR
unit `0x38` `+0679..+06E3`, the U9E effect handler in slot 18 (public
entry 17). It composes the merged raw-CPU SYSTEM string compare
`pr_system_strcmp_0c10` (PR #1001). Its twin `pr_u38_string_match_0947_cpu`
models `+0947..+09B1`, the handler in slot 26; see
[the twin](#the-twin-0947).

Issue #1019 lists the handler as `OVL_0038_0679 — slots 18; missing`. No
model existed: the strict queue's prior-translation hits are only ledger
and classification rows, and no branch names it. It is offered as strict
body credit for `OVL_0038_0679`. The VP approved it on 2026-10-18 (i18027).

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `0x38` `+0679..+06E3`, GAME.OVR file offset `0xF2D4` |
| Size | 107 bytes, 37 instructions, `RETF 0Ah` at `+06E1` |
| Call | `LCALL 0AF8:0C10` at `+06CD` (SYSTEM string compare) |
| SHA-256 | `6b35963f948d9c8eb53df129ee2321b8696a34e5401465654a55c01efb05c4d6` |

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`. The
string compare is `0AF8:0C10..0C3A` of the expanded START.EXE (43 bytes, 20
instructions), SHA-256
`11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58`.

## Behaviour

The U9E dispatcher reaches the handler with a far call and five argument
words. After `PUSH BP; MOV BP,SP; SUB SP,2` the frame holds:

| Frame slot | Contents |
| --- | --- |
| `[BP+6]`, `[BP+8]`, `[BP+0Ah]` | argument words the routine does not read |
| `[BP+0Ch]`, `[BP+0Eh]` | a far pointer (offset, segment) |
| `[BP-1]` | the loop index i |

1. **Pointer** (`+067F..+0692`). `LES DI,[BP+0Ch]`, then `LES
   DI,ES:[DI+108h]`. The far pointer at `ES:[DI+0Ah]/[DI+0Ch]` is stored at
   `DS:[6826h]/[6828h]`; call it P.
2. **Gates** (`+0696..+06AB`). Unless P's bytes `+9Fh` and `+6Ch` are both
   1 (`CMP; JNE`), the routine returns.
3. **Loop** (`+06AD..+06DC`). For i = 1..4 in `[BP-1]`:
   - P and the table string `DS:(0332h + 16i)` are pushed (`XOR AH,AH;
     SHL DI,4; ADD DI,0332h`).
   - `LCALL 0AF8:0C10` compares P's string (left) with the table string
     (right).
   - An equal pair (ZF) increments the byte `DS:[6822h]`.
   - `CMP BYTE [BP-1],4; JNE` loops back to `INC BYTE [BP-1]`.
4. **Return** (`+06DE`). `MOV SP,BP; POP BP; RETF 0Ah`.

**After a return:**

- The flags are those of the last `CMP`.
- After a loop, AX, BX, CX, DX, SI, DI, ES and DF are the last compare's:
  AL and AH are the two lengths, DX is DS, and `CLD` has cleared DF.
- After a gate fails, AX:DX holds P's words and ES:DI holds P.
- `SHL`'s flags never survive `ADD DI`.

**Addressing.** P and every operand are reread in source order. LES pairs,
word operands, `POP` and the `RETF` frame are read physically contiguous.
Offsets wrap at 16 bits and physical addresses at 1 MiB. The code must sit
at its original offsets (IP = `0679h`).

## Composition and stops

| Status | Meaning |
| --- | --- |
| `OK` | returned through `RETF 0Ah` |
| `ARGUMENT` | null pointers, RAM other than 1 MiB, or IP not `0679h`; nothing changed |
| `CODE` | the `+06CD` bytes are not `9A 10 0C`; stops there with the four argument words pushed |
| `CALLEE` | the string compare stopped; `*callee_status` holds its status |
| `TRANSFER` | the compare's `RETF 8` popped a CS:IP other than `+06D2` (a guard that is never exercised) |
| `LIMIT` | `max_loopbacks` loop-backs were taken; the next one stops at `+06B3` |

The call follows the merged U7D `+0565` convention. The opcode and offset
are checked at the site. The segment is read from RAM. After CS and the
return IP `06D2h` are pushed, `pr_system_strcmp_0c10` runs. Without
aliasing the loop takes three loop-backs. The index byte can alias
`DS:[6822h]` or a string, so the host cap guards against a long loop.


## Validation

**Focused C test.** `test_overlay_u38_string_match_cpu.c` runs under CTest
with ASan/UBSan. The source compiles with `-Wconversion -Wsign-conversion
-Wshadow -Werror`. It passes **33** hand-predicted checks with synthetic
records and strings, 24 of them for `+0679`:

- two matches out of four, with the lengths in AX, DX = DS and the final
  flags;
- four matches that wrap the count byte from `FEh` to `02h`;
- each gate failing, with the `CMP` flags;
- `LIMIT` at `+06B3` with cap 0;
- `CODE` at `+06CD` with the pushed words;
- the guards.

**Unicorn differential.** `tools/overlay/u38_string_match_cpu_audit.py`
loads unit `0x38` at its original offsets and the original StrCmp bytes at
`0AF8:0C10` into one 1 MiB image. It compares the whole RAM and all 14 CPU
words with **no mask**. The original decides each stop, and a fetch outside
the two spans is an error. The argument, link and string records sit in
separate 64 KiB segments.

- **Cases: 161** (158 return, 1 `CODE`, 2 `LIMIT`).
- **Coverage.** 37/37 instruction starts and 8/8 conditional outcomes.
- **Inputs.**
  - Equal, prefix, extended, one-byte-off, empty and unrelated table
    strings.
  - Gate bytes of 1, 0, 2, 81h and FFh, and count bytes 0, 7Fh, FFh and
    random.
  - Argument offsets whose `+108h` wraps past `FFFFh`, and link offsets near
    the segment end.
  - SS across A20, DF set, several flag seeds, a random SP and a random
    background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **32
cases**, 15 of them with a match, with no mask. Each compares all 14 CPU
words, the stack window and the whole data segment. The `LCALL` reaches the
StrCmp bytes at absolute `0AF8:0C10`. The result SHA-256 is
`44d2c21e4719b745bba3c37778f5714dd16f48629f46d6eec2c1ab4a55669f31`.

Original bytes and generated images stay under ignored `resources/`.

## The twin +0947

| Item | Value |
| --- | --- |
| Span | unit `0x38` `+0947..+09B1`, GAME.OVR file offset `0xF5A2` |
| Size | 107 bytes, 37 instructions, `RETF 0Ah` at `+09AF` |
| Slot | U9E slot 26 (public entry 23); issue #1019 lists `OVL_0038_0947 — slots 26; missing` |
| SHA-256 | `f00c39518b7e9a889a61c2285f7256392eefbb9b41a3e4b97dd856eb75af5414` |

`+0947` is `+0679`'s code shifted by `2CEh`. The two differ in exactly two
bytes:

- `ADD DI,0372h`, so the table strings are `DS:(0372h + 16i)`;
- `CMP BYTE [BP-1],8`, so i runs from 1 to 8.

The `LCALL` is at `+099B` (fixup at `+099E`), the loop-back `INC` at
`+0981`, and without aliasing the loop takes seven loop-backs. The ledger's
121-byte extent also covers a 14-byte string constant at `+09B2..+09BF`,
just before `+09C0`. `+0947` never references it, so the body is the 107
bytes above. No model of `+0947` existed either: the queue's prior hits are
ledger rows and the boundary lines of the neighbouring `+0918` note.

**Evidence.**

- The focused test's 9 `+0947` checks cover three matches out of eight,
  `LIMIT` at `+0981` with cap 6, `CODE` at `+099B` and the entry guard.
- The Unicorn differential runs with `--routine 0947`: **161** cases (158
  return, 1 `CODE`, 2 `LIMIT`), 37/37 instruction starts and 8/8
  conditional outcomes, no mask.
- The DOSBox-X oracle passes **32** cases, 17 of them with a match, no mask.
  The result SHA-256 is
  `7aad1bafee67d8b464224b7999030c22a4aa08356b7b5b9bebf8a07980fa4a14`.
- The parameterized audit reproduces `+0679`'s 161 cases and native SHA
  `44d2c21e…` unchanged.

## Reproduce

```sh
ctest --test-dir <build> -R poolrad_overlay_u38_string_match_cpu --output-on-failure
for r in 0679 0947; do
  PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1500s \
    python3 tools/overlay/u38_string_match_cpu_audit.py --routine $r --unicorn --native --output resources/u38-$r/report.json
done
```

`deja "Poolrad U38 +0679 string compare handler"` hit its 90-second timebox
with no result, so nothing is reused.

No classifier, queue or count ledger is changed. Crediting `OVL_0038_0679`
and `OVL_0038_0947` is the verifying lane's call.
