# Resident 0802:02EB, FileExists: raw CPU, composed (t30)

`pr_r0802_02eb` (`src/recovered/resident_0802_02eb_cpu.c`) is the raw-CPU, full-state model of START.EXE resident
`0802:[02EB,0356)`, the t30 cluster's FileExists leaf. It is called by 025D at +028E, which 05CD reaches on every block load.
It is the second leaf in the approved bottom-up order (VP i20316).

**A body and its evidence; no credit is claimed here.** Identity follows the t30 chain ruling, through its role line below.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| FileExists | `0802:[02EB,0356)`, image `[0830B,08376)` | 107 / 43 | `141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474` |

The audit asserts that the decode covers the span exactly, that there are five far calls, and that there are two conditional
sites (033C, 0342).

## Behaviour

    02EB PUSH BP; MOV BP,SP; SUB SP,0AAh
    02F2 name -> [BP-2Ah] (StrStore, max 40)
    0305 Assign(f = [BP-0AAh], name); 0315 Reset(f, 128); 0324 Close(f)
    032F DS:[49CE] := IOResult          (no IOCheck: the unit is compiled {$I-})
    0337 AL := 1 when [49CE] = 0 and the copied name is not empty, else 0
    0350 MOV SP,BP; POP BP; RETF 4

Three results the audit pins, each of which a plausible re-implementation would get wrong:

- **An absent file yields IOResult 103, not 2.** The open fails with DOS error 2, but the following Close finds a record that
  is not open and overwrites InOutRes with 103. Real DOS confirmed this natively.
- **An empty name yields 0 although IOResult is 0.** Reset of an empty name opens no file and uses handle 0, and Close of
  handle 0 issues no INT. The answer comes from the length test.
- **A pending InOutRes survives.** Reset still opens and Close still closes, and neither clears it. IOResult then returns the
  stale value, so FileExists answers 0 for a file that exists.

**The third result is a defect in the game, and the model keeps it on purpose.** With a stale InOutRes the model reproduces a
**false negative for a file that exists**, and that is the correct behaviour for this project. A model that "fixed" it would
pass every happy-path test and diverge exactly where the game misbehaves. A decomp reproduces the game's behaviour, not the
behaviour it should have had (VP i20322).

## Composition and contract

- The callees are StrStore `0B25`, Assign `13E2`, Reset `1410`, Close `1491` and IOResult `04A2`, each composed from its
  model on main.
- Each far-call segment word is read live from the code, so the loader relocations hold.
- Each callee must return to the instruction after its call; otherwise the stop is **CALLEE**.
- Reset's nested-Close stop is composed: Close, then Reset resumed at 1445h. It is unreachable here, because Assign leaves the
  magic at D7B0h.
- **Declared environment:** INT 21h AH=3Dh under Reset's contract, and AH=3Eh under Close's.
- Stops:
  - **SERVICE:** a declared service declined; CPU at that INT.
  - **CALLEE:** a composed model stopped otherwise, or returned elsewhere.
  - **ARGUMENT:** a precondition below failed; nothing is changed.
- There is no terminal stop: no IOCheck, so a failure flows on as data.
- Exactly 1 MiB of RAM; IP = 02EBh; entry SP in `[0100h,FFF8h]`.

**Role line (t30 side i):** tests whether the game's data file named by its caller (025D, on 05CD's archive path) exists,
storing the game's IOResult copy at `DS:49CE`.

## Evidence

Receipt: 2026-10-21 00:47:04–00:47:13 PDT (`resources/0802-02eb/final.json`), on a branch from main `5dfe4083`.

**Unicorn: 40 cases, all RETURN.**

- The original runs from the whole relocated image with the real StrStore/Assign/Reset/Close/IOResult bytes.
- Each case is compared unmasked on CPU14, all 1,048,576 RAM bytes and the ordered DOS event log.
- Starts **43/43**; conditional outcomes **4/4**.
- **20 independent checks** compare (AL, `[49CE]`) with the statement above.
- Cases:
  - present, absent, and empty names (with and without the file);
  - names longer than 40 and exactly 40;
  - FileMode 0/2/40h;
  - a pending InOutRes, with the file present and absent;
  - six flag profiles × {present, absent};
  - SP bounds; SS = DS;
  - the name straddling offset FFFF and across A20;
  - 10 random cases.

**Native DOSBox-X: 20 cases equal, the C model against REAL DOS on real files.**

- A present name opens a real file (handle 6). An absent name draws the real DOS error 2, which Close's 103 then overwrites,
  as in the model.
- Compared: CPU14, a 2 KiB stack window, the name area, `DS:4420–442F` and `DS:49CE`.
- **Declared:** DOSBox-X's INT 21h use of the 24 bytes below the caller's SP, masked; the oracle closes handles 6–19 after
  each case. There are no flag masks.
- RESULT.BIN (this run) `571d76b079a2038f0b456f199ad82137b78bf8a6f3a1822af67887ea8cc4ee70`, quoted per run.
- Binary: `/usr/bin/dosbox-x` 2026.01.02, `99d74601…`. It is **not** the pinned build, which is declared.

**C test.** Built with `-Werror` + ASan/UBSan: **19 hand-derived checks**, CTest 1/1. It uses no game bytes. It covers:

- present, absent (103) and the empty name;
- truncation to 40;
- the stale InOutRes (5);
- SERVICE;
- ARGUMENT, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0802_02EB',)`. `--compile-only` builds the composed sources with `-Werror` before any media access; it
was verified with the media absent.

    python3 tools/overlay/res0802_02eb_cpu_audit.py --compile-only
    python3 tools/overlay/res0802_02eb_cpu_audit.py --unicorn --native --output resources/0802-02eb/final.json
