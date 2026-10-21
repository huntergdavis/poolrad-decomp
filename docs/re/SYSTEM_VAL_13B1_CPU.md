# Turbo Pascal SYSTEM Val (longint) at 0AF8:13B1 (+ parser 12CE): raw CPU, runtime coverage

**Runtime coverage, not credit.** This is a Turbo Pascal SYSTEM library routine, and no identity is claimed. It was uncovered
(`runtime_unmapped`) in **24 closures**, all of which also reach `0899:0059`. In 4 of them it is the only uncovered endpoint:

- `OVL_007F_0294`;
- `OVL_0077_24A7`;
- `OVL_007F_0CEB`;
- `OVL_0020_0D38`.

The counts come from my fresh export of main `a4c2998d` and par-a's of `04e5bc7b`, which agree. VP i20211/i20214 asked for
this on the GetTime/Intr pattern.

`pr_system_val_13b1` (`src/recovered/system_val_13b1_cpu.c`) models START.EXE `0AF8:[13B1,13E2)` and its near parser
`0AF8:[12CE,1366)`.

## Media spans

| Piece | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Val | `0AF8:[13B1,13E2)`, image `[0C331,0C362)` | 49 / 22 | `ccbbe96993e3b9e4e8097e2f236090ce7a432c36825b80535b4530076735bfa0` |
| parser | `0AF8:[12CE,1366)`, image `[0C24E,0C2E6)` | 152 / 74 | `c64b8556e8ee0c75d1634824466a6bd1273b5299e94331dfb00777cd10c55748` |

The audit asserts:

- the returns `RET` +132E and +1330, and `RETF 8` at +13DF;
- the single near `CALL 12CE`;
- no INT, IN, OUT or far call: the routine is pure memory.

## Behaviour

Entry is a far call with `[SP+4]` = @Code (a word) and `[SP+8]` = @S (a length-prefixed string).

1. `LES DI,S`, then CX := the length.
2. Skip the leading 20h bytes; tabs and other bytes are **not** skipped.
3. The parser handles an optional `+` or `-`, then either:
   - decimal digits, where each step checks `TEST DX,F000h` (overflow if the value is ≥ 2^28 before the step), computes
     ×10 + digit with `SHL/RCL/ADD/ADC`, and checks `JS` (overflow if the result is ≥ 2^31). So `2147483648` **and**
     `-2147483648` both fail; or
   - `$` and hex digits, with a–f folded to A–F. Each digit is four `SHL/RCL` steps, and overflow is a bit shifted out of
     DX.
4. A `-` negates the result.
5. On an error, or when characters remain, Code := the 1-based position of the first bad character (DI − S offset) and
   DX:AX := 0. Otherwise Code := 0 and DX:AX is the value.
6. The routine ends with `LES DI,@Code; MOV ES:[DI],CX; POP BP; RETF 8`.

## Contract and stops

- Exactly 1 MiB of RAM; IP = 13B1h.
- Entry SP must lie in `[000Ah,FFF4h]`. The pushes reach entry SP − 0Ah (BP, the near return and three parser pushes), and
  the arguments reach entry SP + 0Bh. Outside that range the call returns ARGUMENT and changes nothing.
- **Flags.** Every instruction's flags are computed. SHL's AF is architecturally undefined. On every path it is overwritten
  (by ADD, DEC or SUB) before anything reads it, so the model sets it as DOSBox-X does with no observable consequence. The
  unmasked differentials confirm this.
- Stops: RETURNED, ARGUMENT.

## Validation

Receipt: 2026-10-20 22:14:15–22:14:25 PDT, branch from main `04e5bc7b`.

**Unicorn: 108 cases**, comparing unmasked CPU14 and all 1,048,576 RAM bytes.

- Starts **96/96**; conditional outcomes **44/44** by successor, covering all 22 conditional and `LOOP` sites.
- **103 independent value checks** compare (DX:AX, Code) against a separately written Python statement of Val's semantics.
- Cases:
  - 41 texts: signs, blanks, `0`, leading zeros, decimal and hex limits, both overflows, `$`, `-$10`, empty, lone signs,
    bad characters, `--1`, `+-1`, 40 nines, a tab and a NUL;
  - six flag profiles; SP 000A, 000B, 8001, FFF3 and FFF4; SS = DS; an A20 stack;
  - a string straddling offset FFFF; a string across A20; a short count; a count of 255;
  - **Code over the string**, and **Code over the return frame** (the redirect is asserted);
  - 48 random cases.

**Native DOSBox-X: 47 cases equal**, unmasked: the 41 texts and six flag profiles. Each compares CPU14, the 1 KiB stack, the
272-byte string region and the Code word. There is no interrupt or port on the path, so nothing is excluded and nothing
varies between runs. RESULT.BIN `cb5fed454a0c5b0332b4e7660a0f185b4125ea914ca73a895f05537375816622`. The binary is
`/usr/bin/dosbox-x` 2026.01.02 `99d74601…`; it is **not** the pinned `5a9c7c53` build, which is declared.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **50 hand-derived checks** pass. CTest 1/1. They cover:

- `123`, with the registers, the final ES:DI and the flags (0096h);
- `-45`, with ADC's flags (0086h);
- hex in both cases; blanks; `+`;
- error positions, with the XOR flags (0046h);
- the decimal limit and both overflow forms; hex overflow;
- ARGUMENT, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0AF8_13B1',)`. `--compile-only` builds the source with `-Werror` before any media access; it was
verified with the media absent.

    python3 tools/overlay/system_val_13b1_cpu_audit.py --compile-only
    python3 tools/overlay/system_val_13b1_cpu_audit.py --unicorn --native --output resources/system-val/final.json

## Proposed coverage row (text only; the acceptance lands it)

    strict_candidate_queue.RUNTIME_COVERED_ENDPOINTS:  (0x0AF8, 0x13B1): "system Val longint (system_val_13b1_cpu, #<this PR>)"

No strict, identity, manifest or driver_internal row is proposed. Per VP i20214, the rows it unlocks are to be reported as a
delta once the row lands, never as a credit.

## Acceptance of the coverage row (added by the review lane, par-e, VP i20245)

The row `(0x0AF8, 0x13B1): "system Val longint (system_val_13b1_cpu, #1303)"` lands in `RUNTIME_COVERED_ENDPOINTS`. It
is readiness only, never a credit. It was reproduced at main `e7362e73`, not proofread.

- **Unicorn: 108 cases, all `RETURN`.** Starts 96/96, conditional outcomes 44/44, 103 independent value checks, CPU14
  plus all 1,048,576 RAM bytes, flag masks 0. The spans and the two SHA-256 values equal the table above.
- **Native DOSBox-X: 47 cases equal.** CPU14, the 1 KiB stack and 272 string bytes, flag masks 0. RESULT.BIN
  `cb5fed45…` equals this document's value; this one is not a per-run receipt, because no live port enters it. The
  emulator is `/usr/bin/dosbox-x` 2026.01.02, SHA-256 `99d74601…`, not the pinned `5a9c7c53` build (a declared
  substitution).
- **ASan/UBSan with `-Werror`:** `test_system_val_13b1_cpu.c` passes 50 semantic checks.

**Measured readiness delta** (candidate queue, before and after this row): 440 / 51 / 9 → 440 / **52** / 9.
- `OVL_007F_0294` goes 1 → 0 unready and **becomes composable**. Its model is #1307.
- Its callers `OVL_0077_2156` (7 → 6), `OVL_0077_24A7` (3 → 2) and `OVL_007F_0CEB` (4 → 3) each lose one blocker.
- Composable is not credited. `0294` still needs its own acceptance. Its argument-borne factor 2
  ([U7F_NUMBER_0294_CPU.md](U7F_NUMBER_0294_CPU.md)) is a claim for that review, not settled here.

**Corrected in the same commit:** the `1254` and `1366` rows named `system_str_res1037_cpu`, which is a CTest target,
not a model. The model is `src/recovered/system_str_1366_cpu.c`. A new test requires every coverage row's label to name
a model source in the tree. It fails on the pre-fix table.
