# Turbo Pascal SYSTEM ParamStr at 0AF8:16BC (+ 1712): raw CPU (stays a declared boundary)

**No credit, and no accounting change.** `RES_0AF8_16BC` stays a DECLARED_BOUNDARY
([DECLARED_BOUNDARY_0AF8_16BC.md](DECLARED_BOUNDARY_0AF8_16BC.md)).

The C exists **only to compose**: the funded wall body `RES_0899_0059` calls `ParamStr(1)` at +00B0 and
compares the result with the CS literal `'STING'` (VP i20005, design i20064). ParamStr reads memory, not a
device. Its **real bytes run against a PSP**, and nothing is stubbed or patched. A new C file with no credit
row is therefore deliberate.

`pr_system_paramstr_16bc` (`src/recovered/system_paramstr_16bc_cpu.c`) models START.EXE `0AF8:[16BC,170B)`
for n ≠ 0, with its near helper `0AF8:[1712,1744)`.

## Media spans

| Piece | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| ParamStr | `0AF8:[16BC,170B)`, image `[0C63C,0C68B)` | 79 / 38 | `42c78869351a75004ad361295cc0170912fba23737c53d54f1191bd667fe878e` |
| word walk | `0AF8:[1712,1744)`, image `[0C692,0C6C4)` | 50 / 24 | `7a1426c52b336769caa137a060de90ccadbd8bfb86f86da78e6e6864313c39e2` |

The ParamStr hash matches the declaration's `42c78869…`. The audit asserts:

- exactly the returns `RETF 2` +1708 and `RET` +1743;
- that the only INT (+16CD, `INT 21h` AH=30h) lies inside the excluded mode-0 run.

## Behaviour (n ≠ 0)

1. `PUSH BP; MOV BP,SP; MOV DX,[BP+6]; OR DX,DX; JE 16CB` (not taken); `CALL 1712`.
2. 1712 sets `ES := PrefixSeg (DS:[4422])`, `DI := 80h`, and CX := the tail count byte `ES:[0080]`.
3. It walks the counted bytes. A **word** is a run of bytes above 20h, compared unsigned by `JA`/`JBE`.
   Tabs and control bytes are therefore blanks, and bytes 80h–FFh are word bytes.
4. For each word it does `INC BX; DEC DX` until DX reaches 0 or no word is left. It leaves SI at the word,
   AX = its length (0 past the last word), and CX = the count still unread.
5. Back at 16FB: `PUSH DS; PUSH ES; POP DS; LES DI,[BP+8]; CLD; STOSB` stores the length byte;
   `XCHG CX,AX`; `REP MOVSB` copies the text; then `POP DS; POP BP; RETF 2`.

The final AX is the unread count, BX the words passed, DX the remaining index, DI past the copy, and ES the
destination segment. The flags come from the last `SUB AX,SI` or `DEC DX`, with DF cleared.

## The PSP the body reads, exactly

The body reads only the **count byte at PSP:0080** and the **first `count` bytes from PSP:0081**. DOS stores a
`0Dh` terminator after the counted bytes; **the body never reads it**, because the walk is bounded by CX alone.
A count that runs past the 0Dh reads the 0Dh as a blank and the bytes after it as data, and the audit
constructs that case (`count-over-cr`).

The audit's synthetic PSPs at segment 4000h are written the way DOS writes them: the count byte, the tail
bytes, then 0Dh. The native **DOS-PSP** cases instead point PrefixSeg at the oracle program's own PSP, built
by DOS from the command line `ORACLE.COM STING foo`. The audit asserts that the tail DOS built is exactly:

- count `0Ah`;
- the bytes ` STING foo`;
- `0Dh`.

## Contract and stops

- Exactly 1 MiB of RAM; IP = 16BCh; entry SP in `[0006h,FFF6h]` (the deepest push is entry SP − 6, the highest
  argument byte entry SP + 9). Otherwise ARGUMENT, with nothing changed.
- **n = 0 is ARGUMENT, not modelled** (VP condition 2). Mode 0 is the DOS-version (`INT 21h` AH=30h) and
  environment-copy path at [16CB,16FB). `RES_0899_0059` always passes 1. A caller that can reach mode 0
  carries that gap in its own row, not here. The 20 instructions of that run are excluded from the start
  count for this reason, and the `JE` at +16C4 is accounted only as not-taken.
- Pure memory: no interrupt and no service on the modelled path. The Unicorn interrupt hook fails any run
  that raises one.
- The destination can alias the frame or the PSP. The model follows the original's order (the length byte
  first, then the text forwards), and the audit constructs both aliases.
- Stops: RETURNED, ARGUMENT. There is no loop without a bound: every loop is bounded by the 8-bit count.

## Validation

Receipt: 2026-10-20 21:03:37–21:03:48 PDT, branch from main `a22cbfac`.

**Unicorn: 95 cases**, comparing unmasked CPU14 and all 1,048,576 RAM bytes.

- Starts **42/42** on the n ≠ 0 path, with the mode-0 run excluded as above.
- Conditional outcomes **17/17** by successor: all eight sites in 1712 both ways, plus +16C4 not-taken.
- **90 independent value checks**: the destination's length byte and text equal a separately computed "n-th
  run of bytes above 20h within the counted tail".
- Cases:
  - ten tails (` STING`, empty, ` STING foo`, spaced, bare `STING`, tabs, bytes 80h/FFh/E9h, control bytes, a
    single character, 127 bytes) × n = 1, 2, 3;
  - n = 5, 7FFF, 8000 and FFFF;
  - count shorter than the tail, count 255, and a count past the 0Dh;
  - six flag profiles, including DF set (`CLD`);
  - SP 0006, 0007, 8001, FFF5 and FFF6; SS = DS; an A20 stack;
  - a destination straddling offset FFFF, a destination across A20, a PSP across A20;
  - **the destination over the stack frame**: the copy rewrites the saved DS, the saved BP and the return;
  - **the destination over the PSP tail**: the copy reads bytes it has already rewritten;
  - 40 random cases.

**Native DOSBox-X: 27 cases equal**, unmasked:

- nine synthetic tails × n = 1, 2;
- **three DOS-PSP cases** (n = 1, 2, 3 over ` STING foo`), whose words are also checked against the
  independent value;
- six flag profiles.

Each case compares CPU14, the whole 1 KiB stack window and the 272-byte destination region. There is no
interrupt on the path, so nothing is excluded.

- RESULT.BIN SHA-256 `150e302cf4695bef5800cc38068774011668d19242ff66d96edabfb0b7fca6cc`.
- Binary: `/usr/bin/dosbox-x`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`,
  `DOSBox-X version 2026.01.02 SDL2`. It is **not** the pinned `5a9c7c53` build; the receipt's `dosbox_x`
  block records this. Every flag writer here (CMP, INC, DEC, SUB, OR, XOR) has fully defined flags.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **19 hand-derived checks** pass. CTest 1/1. They cover:

- ` STING` n = 1, with every final register, the flags (0046h) and the frame words;
- n past the last word;
- `foo` from `STING foo`;
- tabs, control bytes and bytes above 7Fh;
- an empty tail;
- a short count (the 0Dh is never read);
- ARGUMENT for n = 0, bad SP or IP, and the wrong size, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0AF8_16BC',)`. `--compile-only` builds the source with `-Werror` before any media
access; it was verified with the media absent.

    python3 tools/overlay/system_paramstr_16bc_cpu_audit.py --compile-only
    python3 tools/overlay/system_paramstr_16bc_cpu_audit.py --unicorn --native --output resources/system-paramstr/final.json

## Rows

None. `RES_0AF8_16BC` stays in `DECLARED_BOUNDARY_ENDPOINTS`. No strict, identity, coverage, manifest or
driver_internal row is proposed.
