# Turbo Pascal Dos.GetTime at 0A32:00A2: raw CPU, runtime coverage

**Runtime coverage, not credit.** This is a Turbo Pascal Dos-unit library routine. No identity or credit is
claimed. It is covered so that the driver-internal `RES_08AE_0000` (the first body of the `OVL_006F_0010`
program, [DRIVER_INTERNAL_CLASS.md](DRIVER_INTERNAL_CLASS.md)) can compose it, on the same footing as the
#1215 SYSTEM Real endpoints. VP i19943 approved the declared-DOS-time design.

`pr_dos_gettime_00a2` (`src/recovered/dos_gettime_00a2_cpu.c`) models START.EXE `0A32:[00A2,00C7)`.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Dos.GetTime | `0A32:[00A2,00C7)`, loader-expanded image `[0A3C2,0A3E7)` | 37 / 19 | `175b7cf9fd00cb6dfa0db3b24bef93332f16cf3ca2c711c670f9c80333ced7fc` |

The image is unpacked with `resident_loader_map.unpack`. The audit asserts that the only return is
`RETF 10h` at +00C4, which is also the first, and that the only interrupt is `INT 21h` at +00A7.

## Behaviour

`PUSH BP; MOV BP,SP; MOV AH,2Ch; INT 21h; XOR AH,AH`. Then, for DL, DH, CL and CH in turn, the body does
`MOV AL,r8; LES DI,[BP+n]; STOSW` through the far arguments Sec100 (`[BP+6]`), Second (`[BP+0Ah]`),
Minute (`[BP+0Eh]`) and Hour (`[BP+12h]`). It ends `POP BP; RETF 10h`. Each LES rereads its pointer after
the previous store, so a store that lands on a later argument's pointer redirects that store. STOSW
honours DF.

## Contract and stops

- Exactly 1 MiB of A20-off RAM; IP = 00A2h. Entry SP must be in `[0002h,FFECh]`, so the pushed BP and the
  14h-byte frame never cross SS:FFFF. LES pairs therefore never wrap. Outside that range the call returns
  ARGUMENT and changes nothing.
- **Declared environment input: `INT 21h` AH=2Ch.** A host service receives the CPU at the INT (IP 00A7,
  AH 2Ch) and returns the complete CPU after DOS returns at 00A9. The model takes that state as given. It
  refuses with **SERVICE**, stopped at the INT, if the service declines, or if its result's CS, IP, SS, SP or
  BP is not that continuation.
- **Stated limit:** the service must not write RAM. DOS's own interrupt frame and stack use below SP are
  outside this model, and the native comparison excludes them (below).
- Flags: `XOR AH,AH` sets ZF and PF and clears CF, OF, SF and AF. AF is architecturally undefined after XOR;
  both oracles give 0. TF, IF, DF and the reserved bits pass through.
- Stops: RETURNED (`RETF 10h`), ARGUMENT, SERVICE. The body has no conditional branch and no loop.

## Validation

Receipt: 2026-10-20 19:32:56–19:33:10 PDT, main `af9e15e0` plus this branch.

**Unicorn, 80 cases**, comparing unmasked CPU14 and all 1,048,576 RAM bytes. Unicorn's interrupt hook plays
the declared service: it supplies CX and DX at the INT, with IP already past it and nothing pushed. Starts
**19/19**; the body has no conditional outcomes. Cases:

- every hour 0–23 (with derived minute, second and hundredths), the edges (0,0,0,0), (23,59,59,99),
  (24,60,60,100) and (255,255,255,255);
- six flag profiles, including DF set;
- SP 0002, 0003, 8001, FFEB, FFEC; SS = DS; an A20 stack;
- all four targets equal, targets overlapping byte by byte, targets straddling offset FFFF and targets across
  A20;
- 32 random cases (time, flags, registers, pointers in three segments);
- two in-contract aliases, each asserting its outcome:
  - `alias-sec100-onto-hour-ptr`: the Sec100 store rewrites the Hour pointer's offset word, and the Hour
    store follows the rewritten pointer;
  - `alias-onto-return`: the Sec100 store rewrites the return IP, and RETF goes elsewhere.

**Native DOSBox-X, 35 cases equal** (hours, edges, six flag profiles including DF set), unmasked. Each
compares CPU14, the 64-byte destination region and the stack from the INT-time SP up to F000.

- Declared stub: a COM installs an `INT 21h` hook that answers AH=2Ch with the case's CX/DX and returns by
  `IRET`, leaving flags as pushed. Other functions chain to DOS. The original GetTime bytes run at 6000:00A2.
- Interrupts are off from the window fill until the record is taken.
- The stub's IRET frame below the INT-time SP is DOS-side and is not compared. That is the stated limit
  above.
- RESULT.BIN SHA-256 `027d9d918a93002a56a9ba10649fe871f60183fdb7607bb4f04b31ec8a03aa2c`.

### Declared substitution: the DOSBox-X binary

The native oracle is `/usr/bin/dosbox-x`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, `DOSBox-X version 2026.01.02 SDL2`. It
is **not** the pinned source build `5a9c7c53`; the receipt's `dosbox_x` block records this. What could
differ from the pinned build:

- lazy-flag conventions; the only flag writer here is `XOR AH,AH`, whose AF both oracles give as 0;
- LES above offset FFFC, which the SP contract excludes;
- stack wrap at FFFF, which is also excluded.

The native sample does not repeat the alias, straddle, A20 or SP-bound cases.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **19 hand-derived checks** pass. They cover the stored words,
the final AX/CX/DX/ES/DI, the XOR flag result, the DF-set DI, SERVICE on refusal and on a non-continuation,
and ARGUMENT, including SP 0001 and FFED, with nothing changed. CTest 1/1.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0A32_00A2',)`. `--compile-only` builds the source with `-Werror` before any media
access; it was verified with the media absent.

    python3 tools/overlay/dos_gettime_00a2_cpu_audit.py --compile-only
    python3 tools/overlay/dos_gettime_00a2_cpu_audit.py --unicorn --native --output resources/gettime/final.json

## Proposed coverage row (text only; the acceptance lands it)

    strict_candidate_queue.RUNTIME_COVERED_ENDPOINTS:  (0x0A32, 0x00A2): "Dos.GetTime (dos_gettime_00a2_cpu, #<this PR>)"

No strict, identity, manifest or driver_internal row is proposed.
