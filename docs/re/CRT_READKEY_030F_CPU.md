# Turbo Pascal Crt.ReadKey at 0A40:030F (+ break check 0143): raw CPU (stays a declared boundary)

**No credit, and no accounting change.** `RES_0A40_030F` stays a DECLARED_BOUNDARY
([DECLARED_BOUNDARY_0A40_030F.md](DECLARED_BOUNDARY_0A40_030F.md)).

The C exists **only to compose**: the funded wall body `RES_0899_0059` calls ReadKey at +0060 and inside its
drain loop (VP i20005, design i20064). The design runs the real Crt bytes; BIOS INT 16h is the only
substituted surface, and no LCALL segment word is patched. A new C file with no credit row is therefore
deliberate.

`pr_crt_readkey_030f` (`src/recovered/crt_readkey_030f_cpu.c`; types in
`include/poolrad/recovered/crt_keyboard_cpu.h`, shared with KeyPressed #1268, on which this PR is stacked)
models START.EXE `0A40:[030F,0331)` and its near helper `0A40:[0143,014B)`.

## Media spans

| Piece | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| ReadKey | `0A40:[030F,0331)`, image `[0A70F,0A731)` | 34 / 14 | `10e41540687b2d6279a0c41c0aba6324bfa555ac4b9cc0b3637e798d58561b3e` |
| break check | `0A40:[0143,014B)`, image `[0A543,0A54B)` | 8 / 3 | `33829488593660a57d88cc10534e3e439fd5ce6c75742aedfec89696001df063` |

The ReadKey hash matches the declaration's `10e41540…`. The audit asserts:

- exactly the returns `RET` +014A and `RETF` +0330;
- a single `INT 16h` (+031D);
- a single near `CALL 0143` (+032D);
- the four conditional sites.

## Behaviour

    030F  MOV AL,[8501] ; MOV BYTE [8501],0 ; OR AL,AL ; JNE 032D   ; a buffered scan code is this key
    031B  XOR AH,AH ; INT 16h                                      ; BIOS: read key
    031F  OR AL,AL ; JNE 032D                                      ; ordinary key
    0323  MOV [8501],AH ; OR AH,AH ; JNE 032D                      ; extended: buffer the scan code, return 0
    032B  MOV AL,3                                                 ; AL = AH = 0: synthesised Ctrl-C
    032D  CALL 0143 ; RETF
    0143  CMP BYTE [8502],0 ; JNE 014B ; RET                       ; Ctrl-Break pending?

## Contract and stops

- Exactly 1 MiB of RAM; IP = 030Fh; entry SP in `[0002h,FFFCh]`, because the near CALL writes SS:SP−2 and the
  RETF frame must not cross SS:FFFF. Otherwise ARGUMENT, with nothing changed.
- **Declared environment input: BIOS INT 16h.** The contract is the one in #1268: the service gets the CPU at
  +031D with AH 00h and returns at +031F, or the model stops with SERVICE.
- **Fail-closed:** only AH 00h and 01h are listed, and any other AH is **ENV** (VP condition 4). Here AH is
  the constant 00h, so ENV is unreachable. The Unicorn hook refuses any other interrupt or AH by failing the
  run.
- **Stated limit:** the service writes no RAM.
- **BREAK is an EXIT-class stop, not a LIMIT** (VP condition 1). If DS:[8502] is non-zero (a Ctrl-Break that
  Crt's INT 1Bh handler recorded), the original goes on at +014B:
  1. it clears the flag and drains the keyboard with INT 16h 01h/00h;
  2. it writes `^C` (+047E, +0477);
  3. it raises **INT 23h**, which terminates the process by default.

  The model stops at 0A40:014B with the CALL's return word pushed. It is in the same family as `0899:0000`'s
  Halt: a routine that looks like a pure service can end the process, and a model that returned from it
  would be wrong. **DS:[8502] must be 0 to continue.**
- Order matters, and the model follows it. The CALL's return word is written *before* +0143 reads [8502],
  so a stack placed over the flag can itself raise BREAK; two audit cases construct exactly that.
- Stops: RETURNED, BREAK, ARGUMENT, SERVICE, ENV (unreachable here).

## Validation

Receipt: 2026-10-20 20:42:44–20:45:17 PDT, run on the branch stacked on #1268 (main `decd4e16`), which
then merged main `3a2b1bd0` with no change to these files.

**Declared stub, identical in every oracle.** For AH=00h the stub sets AX := the case's key word and leaves
the flags as they were at the INT (IRET with the frame's flags). Natively the INT 16h vector is saved and
restored around each case, and interrupts stay off.

**Unicorn: 92 cases**, comparing unmasked CPU14 and all 1,048,576 RAM bytes.

- Stops: **67 RETURN, 25 BREAK**. A BREAK run is stopped at 014B, which is never executed.
- Starts **17/17**; conditional outcomes **8/8** by successor.
- The service is called exactly as often as the original executes the INT.
- Cases:
  - buffered 01/3B/80/FF;
  - keys 1E61, 3B00, 0000, 8000, 0003, E0E0 and FF00;
  - break flag 01/80/FF × {key, buffered, extended};
  - six flag profiles × five modes;
  - SP 0002, 0003, 8001, FFFB and FFFC; SS = DS; an A20 stack; DS across A20;
  - **`alias-push-onto-8502`** (SS = DS, SP 8504h): the return word sets [8502] := 30h, giving BREAK;
  - **`alias-push-onto-8501`** (SP 8503h): the word overwrites the buffered byte and sets [8502] := 03h,
    giving BREAK. Both are asserted to reach BREAK;
  - 32 random cases.

**Native DOSBox-X: 50 cases equal** (35 RETURN, 15 BREAK), unmasked. Each compares CPU14, the stack from
entry SP − 2 (the CALL's return word) to F000, and DS:8500–850F.

- Below that point lies the BIOS stub's IRET frame, which is BIOS-side; this is the stated limit.
- **Harness capture, declared:** the test copy holds only the two modelled pieces. The harness writes a far
  `JMP` at test-copy CS:014B, which is the first byte after the modelled break-check piece; no game byte is
  placed or patched there. A BREAK therefore lands in a capture routine instead of running `^C`/INT 23h.
  The capture records CS:IP as 6000:014B.
- RESULT.BIN SHA-256 `ebe38e4986e83a74fd47a8254460a1ef0933185f6a4beaa2b948f54565ebc87f`.
- Binary: `/usr/bin/dosbox-x`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`,
  `DOSBox-X version 2026.01.02 SDL2`. It is **not** the pinned `5a9c7c53` build; the receipt's `dosbox_x`
  block records this. The flag writers are OR/XOR/CMP with zero, whose flags are all defined; their AF is 0
  in both oracles.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **21 hand-derived checks** pass. KeyPressed's 18 checks still
pass against the extended header; CTest 2/2. The checks cover:

- the buffered, ordinary, extended and synthesised-3 paths, including the buffer byte;
- the CPU at the INT;
- BREAK at 014B, with the return word;
- SERVICE on a decline and on a wrong continuation;
- ARGUMENT, including SP 0001 and FFFD, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0A40_030F',)`. `--compile-only` builds the source with `-Werror` before any media
access; it was verified with the media absent.

    python3 tools/overlay/crt_readkey_030f_cpu_audit.py --compile-only
    python3 tools/overlay/crt_readkey_030f_cpu_audit.py --unicorn --native --output resources/crt-readkey/final.json

## Rows

None. `RES_0A40_030F` stays in `DECLARED_BOUNDARY_ENDPOINTS`. No strict, identity, coverage, manifest or
driver_internal row is proposed.
