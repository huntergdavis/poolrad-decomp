# Turbo Pascal Crt.KeyPressed at 0A40:02FD: raw CPU (stays a declared boundary)

**No credit, and no accounting change.** `RES_0A40_02FD` stays a DECLARED_BOUNDARY
([DECLARED_BOUNDARY_0A40_02FD.md](DECLARED_BOUNDARY_0A40_02FD.md)).

The C exists **only to compose**: the funded wall body `RES_0899_0059` calls KeyPressed in its drain loop
(VP i20005, design i20064). The design runs the real Crt bytes; the BIOS keyboard service is the only
substituted surface, and no LCALL segment word is patched anywhere. A new C file with no credit row is
therefore deliberate, not an accounting omission.

`pr_crt_keypressed_02fd` (`src/recovered/crt_keypressed_02fd_cpu.c`, types in
`include/poolrad/recovered/crt_keyboard_cpu.h`, shared with the coming ReadKey model) models START.EXE
`0A40:[02FD,030F)`.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Crt.KeyPressed | `0A40:[02FD,030F)`, loader-expanded image `[0A6FD,0A70F)` | 18 / 8 | `83af1ee6b1d5cebb5dee97b742228b8a4fb3cbb12ade84b39e5ce82275e7fc5e` |

The hash matches the declaration's `83af1ee6…`. The audit asserts:

- a single `RETF`, at +030E;
- a single `INT 16h`, at +0306;
- conditional sites at exactly +0302 and +030A.

## Behaviour

    02FD  CMP BYTE [8501],0   ; extended-key byte buffered by ReadKey
    0302  JNE 030C
    0304  MOV AH,1
    0306  INT 16h             ; BIOS: key pending? (ZF = none)
    0308  MOV AL,0
    030A  JE 030E
    030C  MOV AL,1
    030E  RETF

KeyPressed **does not** read the BIOS buffer at 0040:001A/001C. With nothing buffered at DS:8501 it asks the
BIOS, so the BIOS service is the device; this was verified from the disassembly (i20063).

- If a byte is buffered, AH and the flags come from `CMP` (CF, OF and AF clear; ZF, SF and PF from the byte).
- Otherwise AH and the flags are whatever the BIOS returned, and AL is set from ZF.

## Contract and stops

- Exactly 1 MiB of RAM; IP = 02FDh; entry SP ≤ FFFCh, so the return frame does not cross SS:FFFF.
  Otherwise ARGUMENT, with nothing changed.
- **Declared environment input: BIOS INT 16h.** The service receives the CPU at the INT (IP 0306h,
  AH 01h) and returns the complete CPU after the BIOS returns at 0308h. The model checks that CS, IP, SS, SP
  and BP are that continuation; a declined call or any other result is **SERVICE**, stopped at the INT.
- **Fail-closed, per VP condition 4:** only AH 00h and 01h are listed for INT 16h. Any other AH at an INT is
  an **ENV** stop, never a default return. Here AH is the constant 01h, so ENV is a structural guard that
  this body cannot reach. In the audit, the Unicorn interrupt hook refuses any interrupt other than 16h,
  and any AH other than 01h, by failing the run.
- **Stated limit:** the service must not write RAM. The BIOS's own interrupt frame and stack below SP are
  outside the model, and the native comparison excludes them.
- Stops: RETURNED, ARGUMENT, SERVICE, ENV (unreachable here).

## Validation

Receipt: 2026-10-20 20:27:27–20:35:22 PDT, run on a branch from main `16af6e2c`, which was then rebased
onto `decd4e16` with no change to these files.

**Declared stub, identical in every oracle.** For AH=01h the stub sets AX := the case's key word and
ZF := no key pending, and changes no other flag.

- Unicorn plays it with an interrupt hook.
- Natively, an INT 16h hook edits ZF in its IRET frame. The vector is saved and restored around each case,
  and interrupts stay off.
- The C service applies the same rule.

**Unicorn: 73 cases**, comparing unmasked CPU14 and all 1,048,576 RAM bytes.

- Starts **8/8**; conditional outcomes **4/4** by successor (`JNE` 030C/0304, `JE` 030E/030C). Every case
  RETURNs.
- The service is called exactly as often as the original executes the INT.
- Cases:
  - buffered byte 00/01/03/80/FF × pending or not;
  - six flag profiles × {no key, key, buffered};
  - key words 0000, 011B, 3B00 and FFFF;
  - SP 0000, 0001, 8001, FFFB and FFFC; SS = DS; an A20 stack; DS across A20;
  - 32 random cases.

**Native DOSBox-X: 32 cases equal**, unmasked: buffered/pending, six flag profiles × three modes, and key
words. Each compares CPU14, the stack from the entry SP to F000, and DS:8500–850F.

- RESULT.BIN SHA-256 `353f9654715a2ab8187a7f07ec9860904e4b2e8e2b1b848568482082669cf080`.
- Binary: `/usr/bin/dosbox-x`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`,
  `DOSBox-X version 2026.01.02 SDL2`. It is **not** the pinned `5a9c7c53` build; the receipt's `dosbox_x`
  block records this. The only flag writer here is `CMP r/m8,0`, whose flags are all defined.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **18 hand-derived checks** pass. CTest 1/1. They cover:

- the buffered path, with its CMP flags (3Bh and 80h);
- the CPU at the INT;
- the key-pending and no-key results;
- SERVICE on a decline and on a wrong continuation;
- ARGUMENT, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0A40_02FD',)`. `--compile-only` builds the source with `-Werror` before any media
access; it was verified with the media absent.

    python3 tools/overlay/crt_keypressed_02fd_cpu_audit.py --compile-only
    python3 tools/overlay/crt_keypressed_02fd_cpu_audit.py --unicorn --native --output resources/crt-keypressed/final.json

## Rows

None. `RES_0A40_02FD` stays in `DECLARED_BOUNDARY_ENDPOINTS`. No strict, identity, coverage, manifest or
driver_internal row is proposed.
