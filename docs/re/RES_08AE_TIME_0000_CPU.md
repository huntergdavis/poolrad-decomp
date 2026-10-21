# Resident 08AE:0000: time since midnight in hundredths (raw CPU, DRIVER_INTERNAL)

**DRIVER_INTERNAL candidate: translated and evidenced, composes as ready, NEVER a credit**
([DRIVER_INTERNAL_CLASS.md](DRIVER_INTERNAL_CLASS.md), [RES_08AE_IDENTITY_POLICY.md](RES_08AE_IDENTITY_POLICY.md)).
This is body #3 of the `OVL_006F_0010` program. No strict, identity or manifest row is proposed; the class
refuses them by test.

`pr_r08ae_time_0000_cpu` (`src/recovered/resident_08ae_time_0000_cpu.c`, generated per instruction by
`tools/overlay/emit_res08ae_time_0000.py`, whose `--check` regenerates it byte-for-byte) models START.EXE
`08AE:[0000,006C)`.

It composes Dos.GetTime `0A32:00A2` (runtime-coverage model merged in #1245; its coverage row is not yet
landed) and SYSTEM LongMul `0AF8:0A5B` (covered).

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| 08AE time | `08AE:[0000,006C)`, loader-expanded image `[08AE0,08B4C)` | 108 / 48 | `a8e33767ba204b74a7c909cb85a7580ddbbaa36d60dfafb5fb3425d8c16412ed` |
| Dos.GetTime | `0A32:[00A2,00C7)` | 37 / 19 | `175b7cf9fd00cb6dfa0db3b24bef93332f16cf3ca2c711c670f9c80333ced7fc` |
| SYSTEM LongMul | `0AF8:[0A5B,0A76)` | 27 / 16 | `74b9ab3b26ee7b54bf51f067f0825dea48772933b210dc5b38897464e5f65745` |

The audit asserts these points:

- each body's first return is its only return; this body's is `RETF` at +006B;
- the only far calls are `LCALL 0A32:00A2` at +001A and `LCALL 0AF8:0A5B` at +002A;
- their segment words sit at +001D and +002D and are read live.

## Behaviour (including a truncation the original really does)

The body reserves four word locals and passes their SS addresses to GetTime:

| Local | Holds |
| --- | --- |
| `[BP-6]` | hour |
| `[BP-8]` | minute |
| `[BP-0Ah]` | second |
| `[BP-0Ch]` | sec100 |

It then computes:

    DX:AX := LongMul(hour, 360000) + low16(minute * 6000) + low16(second * 100) + sec100

It stores the result in `[BP-4]`/`[BP-2]` and returns it in DX:AX. Each 16-bit `MUL` is followed by
`XOR DX,DX`, so **the minute term keeps only the low word of minute × 6000 and wraps for minutes ≥ 11**.
For example, 00:11:00.00 gives 464, not 66,000, and 13:42:07.55 gives 4,736,147, not 4,932,755. The second
term cannot wrap for valid seconds. This is the original's arithmetic, which the model reproduces exactly.
The C test and the audit's independent value check pin it.

## Contract and stops

- Exactly 1 MiB of RAM through `pr_bus` with **no EGA attached** (ARGUMENT otherwise). The body touches no
  port and no video memory, and both callees run on the flat RAM.
- IP must be 0000h. Code and both segment words are immutable; the model does not refetch code. Stack words
  are addressed at SS:(offset mod 64K).
- GetTime's `INT 21h`/2Ch is the **declared environment input** (`pr_dos_gettime_service`), passed through
  unchanged. GetTime's own contract, including its frame guard, applies to its call.
- The deepest frame is GetTime's: entry SP − 2Eh. The audit stays inside GetTime's guard.
- Stops:
  - RETURN;
  - ARGUMENT (null bus, RAM or service; an EGA attached; size ≠ 1 MiB; IP ≠ 0);
  - CALLEE: GetTime or LongMul did not return. The diagnostic keeps each leaf's status; GetTime's SERVICE is
    the only reachable case, and LongMul fails only on invalid host arguments;
  - TRANSFER: a callee returned elsewhere.

  The body has no conditional branch and no loop.
- Flags. The two `MUL`s leave SF/ZF/AF/PF architecturally undefined. A per-producer path analysis from the
  emitter shows both are overwritten on every path before any observation (`XOR DX,DX`, then the `ADD`/`ADC`
  chain). The final flags are those of `ADC DX,BX`.

## Validation

Receipt: 2026-10-20 19:41:54–19:43:03 PDT, run on this branch while it was stacked on #1245, before the rebase
onto main `c4d054c9`. The rebase changed nothing in these sources.

**Unicorn: 74 cases**, comparing unmasked CPU14 and all 1,048,576 RAM bytes. The interrupt hook supplies the
declared DOS time.

- Coverage: body **48/48** starts; closure (body + GetTime + LongMul) **83/83**. There are no conditional
  outcomes in the closure.
- Every returning case also checks DX:AX against the formula above, computed independently.
- Cases: every hour 0–23; the edges (0,0,0,0), (23,59,59,99), (24,60,60,100), (255,255,255,255), (11,59,59,99)
  and (12,0,0,0); six flag profiles; SP 0030, 0031, 8001, FFFC; SS = DS; an A20 stack; 32 random cases.

**Native DOSBox-X: 36 cases equal**, unmasked (hours, edges, six flag profiles). Each compares CPU14 and the
40 stack bytes from GetTime's INT-time SP up to F000. Those bytes hold the frame, the four locals, the result
words and LongMul's partial product.

- Declared stub: the same `INT 21h` hook as #1245. It answers AH=2Ch with the case's time and returns by
  `IRET`; its frame below that SP is DOS-side and not compared.
- The original body, GetTime and LongMul run at test segments 8800/6000/6800, with the two far-call segment
  words patched, which is the relocation.
- RESULT.BIN `797dbf57ec6cf54b3c36850a3bea83b0ab1573036d21408e99966688144550d4`.
- Binary: `/usr/bin/dosbox-x`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`,
  2026.01.02. It is **not** the pinned `5a9c7c53` build; the `dosbox_x` receipt block records this. What
  could differ: MUL's undefined flags, which are dead here; LES or stack wrap at FFFF, which the cases do not
  exercise natively.

**C test** built with `-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, **29 hand-derived checks** pass, CTest 1/1. The checks cover:

- the four values above, including both truncation examples;
- the stored locals and the result words;
- CALLEE with GetTime's SERVICE status when the DOS service refuses;
- ARGUMENT for a NULL service, an attached EGA, the wrong size and the wrong IP, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_08AE_0000',)`. `--compile-only` builds the five sources with `-Werror` before any media
access.

    python3 tools/overlay/emit_res08ae_time_0000.py --check
    python3 tools/overlay/res08ae_time_0000_cpu_audit.py --compile-only
    python3 tools/overlay/res08ae_time_0000_cpu_audit.py --unicorn --native --output resources/08ae-0000/final.json

The emitter gained one instruction form, `LEA` (effective offset; no memory access, no flags). After the
change, `emit_res0464_blit_1951.py --check` still regenerates 1951 byte-for-byte.

## Proposed member row (TEXT ONLY, for par-e's acceptance)

    docs/re/driver_internal_models.csv: RES_08AE_0000,docs/re/RES_08AE_TIME_0000_CPU.md,<this PR>,<reviewed head>,a8e33767ba204b74a7c909cb85a7580ddbbaa36d60dfafb5fb3425d8c16412ed,108

Closure-complete only once GetTime's `RUNTIME_COVERED_ENDPOINTS` row (proposed in #1245) lands; LongMul is
already covered.

## Reviewer acceptance record (added by the review lane, not the author)

Reproduced by par-e at author head `764442db` (merged as #1249) on main `76bc8e13`: Unicorn 74 cases (all
RETURN), body 48/48, closure 83/83, CPU14 + 1,048,576 B, flag masks 0; native 36 cases under the declared
`INT 21h` stub, RESULT.BIN `797dbf57…`, declared `dosbox_x` substitution; 29 C checks under `-Werror` ASan/UBSan.

**Minute truncation, checked in the bytes:** `+0033 MOV AX,1770h; +0036 MUL WORD [BP-8]; +0039 XOR DX,DX`.
The minute term is therefore `low16(minute * 6000)`, and it **wraps at minute 11, not at minute 1**:
10 * 6000 = 60,000 fits in 16 bits and 11 * 6000 = 66,000 does not. The fixtures exercise the wrap almost
everywhere, in both the Unicorn and native sets: the default time is `(13, 42, 7, 55)`, and the edges include
minutes 59 and 255. Every result is asserted against an independent formula that applies the truncation, so a
non-truncating model fails almost every case, not a rare one.

**Membership is closure-checked:** the member row landed together with Dos.GetTime's runtime coverage
(`0A32:00A2`) and with a loader closure check. LongMul `0AF8:0A5B` was already covered.
