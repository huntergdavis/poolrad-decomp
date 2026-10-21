# RES_0899_0000: the game's QUIT routine (raw CPU, composed closure) — strict-credit candidate

`pr_r0899_quit_0000_cpu` (`src/recovered/resident_0899_quit_0000_cpu.c`) models START.EXE resident `0899:[0000,0033)`.
It composes the merged models of everything it calls:

- the 08AE driver dispatch `00DC` with its members, through `src/recovered/resident_08ae_driver_cpu.c`, a composition
  file with no body of its own;
- `Dos.Intr` (#1259);
- the SYSTEM `Halt` boundary (#1260), as the EXIT stop.

This is the first half of the funded wall (VP i20005, task t28). `0899:0059` follows in its own PR.

**Necessary, not sufficient.** On a fresh queue export of main `07cc429a` (442 rows), **303 rows** have `RES_0899_0000` in
their closure, and 302 have `RES_0899_0059`. par-a measured the wall as **sufficient for exactly one row**
(`OVL_007F_0294`). The 303 is therefore the number of rows this body blocks. It is not a promise that 303 rows become ready.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| QUIT | `0899:[0000,0033)`, loader-expanded image `[08990,089C3)` | 51 / 19 | `f4d6f25f38c01ad8730fc424ad38214c037804afe497ff125794d8e008465d7d` |

The loader-expanded image SHA-256 is `a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e`, with 561 loader
relocations. The audit asserts these facts about the span:

- it holds exactly three far calls: `08AE:00DC` at +000D, `0A32:000B` at +0025 and `0AF8:00E9` at +002C;
- their segment words at +0010, +0028 and +002F are loader relocations, and the model reads them live;
- there is no branch, near call or INT;
- the only return is `RETF` at +0032.

## Behaviour

    0000  PUSH BP ; MOV BP,SP
    0003  MOV AL,[5373] ; MOV [5372],AL          restore the saved sound state
    0009  PUSH WORD [2612] ; LCALL 08AE:00DC      the driver dispatch with argument [2612]
    0012  MOV BYTE [4A03],0 ; MOV AL,[4A1C] ; MOV [4A02],AL     Regs.AX = (AH 0 = set mode, AL = saved video mode)
    001D  MOV AL,10h ; PUSH AX ; MOV DI,4A02h ; PUSH DS ; PUSH DI ; LCALL 0A32:000B     Intr($10, Regs)
    002A  XOR AX,AX ; LCALL 0AF8:00E9              Halt(0)
    0031  POP BP ; RETF                            DEAD CODE

**Every path ends at EXIT.** Halt runs the ExitProc chain, restores the vectors and issues `INT 21h` AH=4Ch; it never
returns (DECLARED_BOUNDARY_0AF8_00E9.md). So `POP BP; RETF` at +0031/+0032 is **dead code**, not
"unreachable but possible". The audit asserts that neither instruction is ever reached.

The model stops with the Halt call executed:

- the return `0899:0031` and the body CS are pushed;
- CS:IP is the live Halt target;
- AX = 0, the exit code;
- the flags are those of `XOR AX,AX`.

The model never returns. Key 03 in `0899:0059` reaches this routine by `PUSH CS; CALL 0000`, which therefore never
returns either.

## Identity (two-factor standard, #1195)

- **Factor 1 (reached from game code).** There are relocation-confirmed far fixups to `0899:0000`
  (`far_fixup_confirmed` in the function-accounting callgraph regenerated on this branch from main `07cc429a`). The
  callers are classified on that same fresh export:

  | Caller | Classification | Call site |
  | --- | --- | --- |
  | `OVL_001C_19B0` | game_verified | unit 1Ch, +1A7B |
  | `OVL_03BD_0E08` | game_verified | unit 3BDh, +0EBC |
  | `OVL_03BD_10C7` | game_verified | unit 3BDh, +115B: the wallset entry 7, U3BD_WALLSET_ENTRY7.md, which already records this call as the exit routine |

  There are nine further game_evidenced callers (`OVL_0019_00FD`, `OVL_006A_18E5`, `OVL_006A_1E6F`, `OVL_006C_0161`,
  `OVL_006F_0EC4`, `OVL_006F_22D5`, `OVL_03B9_0708`, `OVL_040C_003B`, `OVL_040C_015D`), one unresolved (`OVL_03CE_0022`),
  and the near call from `0899:0059` +00DA.
- **Factor 2 (documented game state).** The body's subject is game state:
  - the sound-state pair DS:5372/5373, whose writers are documented in SLOT_A_RESIDENT_DISPATCH_GATE.md;
  - the driver argument DS:[2612];
  - the saved video mode DS:[4A1C];
  - the game's `Registers` record at DS:4A02.

## Contract and stops

- Exactly 1 MiB of A20-off RAM; IP = 0000h.
- Entry SP must lie in `[000Ch,FFFEh]`, so that this body's own pushes (deepest at entry SP − 0Ch) do not wrap. Deeper
  frames are guarded by the composed models, whose refusal is a named stop.
- **Declared environment.** It is the union of the composed models' own declarations and nothing more:

  | Input | Used by | Contract |
  | --- | --- | --- |
  | `INT 21h` AH=25h/35h, vector-table service | 137C / 13B1 | hardware frame written, flags returned |
  | `OUT` 40h/43h/61h/C0h and `IN` 61h | 137C / 13B1 | — |
  | `INT 21h` AH=2Ch | Dos.GetTime, via `RES_08AE_0000` inside 006C's wait | no frame; must not write RAM |
  | 006C's step limit | 006C | — |
  | the `INT 10h` handler that Intr transfers to | Intr | may write RAM; continuation checked |

- Stops:
  - **EXIT**: the only completing outcome;
  - **ARGUMENT**: nothing changed;
  - **DRIVER**: a composed driver member stopped. The diagnostic names the member (00DC, 137C, 13B1, 1742, 17AE, 181A or
    006C) and its status; a 006C TIME stop also carries `RES_08AE_0000`'s status;
  - **INTR**: Intr did not return; its SERVICE status means the INT 10h handler;
  - **TRANSFER**: a callee returned somewhere other than the next instruction. It is a defensive stop, reached in no
    audit case.

## Validation

Receipt: 2026-10-20 21:33:16–21:33:57 PDT (`final.json`), on this branch from main `07cc429a`. The branch then merged
main `7cf02cd7` (membership rows only), with no change to any composed source.

**The closure runs from the original bytes.** The whole loader-expanded image is loaded at a test base (1000h for
Unicorn, 3000h natively) with the **loader's own relocation table applied**, which is what DOS does. No byte of any
modelled body is patched.

**Unicorn: 112 cases, all EXIT.** Each compares unmasked CPU14 and all 1,048,576 RAM bytes, and the environment event log
(every `INT 21h`, `IN` and `OUT`, in order) must be identical between the original run and the C model's callbacks.

- **Body starts: 17/17 live.** The two dead starts (+0031, +0032) are asserted never reached. There are no conditional
  outcomes in the body.
- **Every member's code actually executed.** Each count is the instruction starts executed in any case over the
  instructions decoded from that member's original span (receipt 21:40:10–21:40:32 PDT):

  | Member | Span | Executed / decoded |
  | --- | --- | ---: |
  | dispatch | `08AE:[00DC,017B)` | 77/77 |
  | vector save/install | `08AE:[137C,13B1)` | 24/24 |
  | vector restore/PIT/PSG | `08AE:[13B1,13EA)` | 27/27 |
  | table writer | `08AE:[1742,17AE)` | 43/43 |
  | table writer | `08AE:[17AE,181A)` | 43/43 |
  | byte | `08AE:[181A,1830)` | 12/12 |
  | delay | `08AE:[006C,00DC)` | 46/46 |
  | time | `08AE:[0000,006C)` (RES_08AE_0000) | 48/48 |
  | Dos.Intr | `0A32:[000B,006C)` | 69/69 |
  | Dos.GetTime | `0A32:[00A2,00C7)` | 19/19 |
  | SYSTEM LongMul | `0AF8:[0A5B,0A76)` | 16/16 |
  | INT 10h stub | declared | 8/8 |

- **Two kinds of fidelity, kept apart** (VP i20175):
  - **Each member's own behaviour** rests on that member's own evidence at the bar: its PR, its doc, its unmasked
    differential.
  - **The composition** rests on this run. The members are chained through their real interfaces (each CALL stop,
    each resume, each environment callback), and the result is compared against the original image executing the
    real bodies, with nothing patched.

  This run is the composition proof for the set. It does not replace any member's evidence.
- Driver member calls per case: 0 (muted: 00DC returns at once) in 29 cases, and 1 to 8 in the rest.
- Cases:
  - saved sound state 0/1/2/3/FF × [5374] 0/1/100h × argument 0/1/FF/5;
  - six video modes;
  - six flag profiles × {muted, on};
  - IN 61h values 00/03/30/FF;
  - a clock crossing midnight and a 1-hundredth clock (006C's wrap guard and long wait);
  - a zero saved vector; low and odd SP; SS = the game DS;
  - 24 random cases.
- **Independent expectations**, from the body's bytes and not from either model, are checked in every case:
  - [5372] = the saved [5373];
  - Regs.AX at DS:4A02 = (0, [4A1C]);
  - the INT 10h stub saw AL = [4A1C], visible as 0040:0049;
  - AX = 0 at Halt;
  - the return frame `0899:0031` is on the stack.

**Native DOSBox-X: 42 cases equal.** The cases are the sound states × [5374] × argument, six modes, and six flags ×
{muted, on}. Each compares:

- CPU14;
- the 1 KiB stack window;
- the game fields DS:4A00+20h, 5370+8 and 2610+4;
- the driver tables `08AE:01D0+530h`;
- `08AE:1378+4`, the saved timer vector;
- the shadow vector entry 8;
- the BIOS mode byte;
- the number of GetTime calls.

RESULT.BIN SHA-256 `01182f30ace85c9018e4e257337382dfcc5251218ee07919a60f9637d3d69935` is **a per-run receipt, not a
fixed value to compare across runs**. RESULT.BIN records the live IN 61h port byte that DOSBox-X supplies, and that
byte varies from run to run: par-e's reproduction got `74957ab9…`. The comparison passes on every run. Only the 20
IN-61h-masked cases are affected, and they are otherwise fully unmasked. **The variation is in a value the environment
supplies, not in the result.** IMAGE.BIN (relocated to 3000h)
`02bac6e8cd3c3e6450f1f67d87ebc450bdaaf506261fcfca6537082e1507b2fe`. The binary is `/usr/bin/dosbox-x`, SHA-256 `99d74601…`,
2026.01.02. It is **not** the pinned `5a9c7c53` build; the receipt's `dosbox_x` block records this.

The native substitutions are declared, each with its exact location:

1. **INT 21h hook.** The oracle program serves AH=25h/35h over a shadow vector table and AH=2Ch from the case's scripted
   clock. It returns by `IRET` with no stack use of its own; other functions chain to DOS. The real IVT[08h] is never
   written.
2. **INT 10h.** IVT[10h] points at the same 13-byte real stub the Unicorn run uses. The vector and the BIOS mode byte
   are restored after each case.
3. **Halt capture, a change to the native copy only.** At native `0AF8:00E9` the five bytes `33 C9 33 DB BA` are replaced
   by `EA <capture> <oracle CS>`, a far JMP. The receipt records both the before and after bytes. The model stops at this
   address without executing it, so no modelled byte changes.
4. **IN 61h is live emulated hardware.** The C model is run with two IN values whose `AND AL,FCh` flags differ. Exactly
   the bytes that differ are masked, and those are **asserted to be SF/ZF/PF bits only, in at most two bytes**: Regs.FLAGS
   at DS:4A14, and Intr's pushed flags word at entry SP − 18. 13B1's flags reach these through Intr's `PUSHF`. That
   happens in the **20 cases that run 13B1**; the 22 muted cases are compared with no mask at all.
5. **GetTime frames.** The 6-byte DOS frame of each real `INT 21h` AH=2Ch lies below GetTime's INT-time SP. It is
   outside the GetTime model, as #1245 states, and is masked at its recorded addresses.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **18 hand-derived checks** pass. CTest 1/1. The test uses synthetic RAM:
only the three segment words, the IVT entry and the game fields are placed. It covers:

- the muted path to EXIT, with every register, the flags, the frame, Regs and the handler's view;
- a DRIVER stop naming 137C's ENV_DOS;
- an INTR stop on a refusing handler;
- ARGUMENT, with nothing changed.

## What composition readiness still waits on (not this body's evidence)

- The `RUNTIME_COVERED_ENDPOINTS` row for `0A32:000B`, proposed in #1259 and not yet on main.
- The `DECLARED_BOUNDARY_ENDPOINTS` line for `RES_0AF8_00E9`, proposed in #1260 and not yet on main.
- The remaining 08AE membership rows: 006C, 137C/13B1, 017B/00DC and 01A5. On main, 181A, 0000, 1742 and 17AE are in.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0899_0000',)`. `--compile-only` builds the 16 sources with `-Werror` before any media access; it was
verified with the media absent.

    python3 tools/overlay/res0899_quit_0000_cpu_audit.py --compile-only
    python3 tools/overlay/res0899_quit_0000_cpu_audit.py --unicorn --native --output resources/0899-quit/final.json

## Proposed rows (TEXT ONLY; the acceptance lands them)

    tools/overlay/resident_inventory.py  GAME_SPECIFIC_TARGETS  += (0x899, 0x0000)
    docs/re/strict_full_c_addresses.csv:  RES_0899_0000,docs/re/RES_0899_QUIT_0000_CPU.md

Resident identities live in `GAME_SPECIFIC_TARGETS` plus the strict table, and not in `reviewed_function_identities.csv`
(handoff condition 9).

## Reviewer acceptance record (added by the review lane, not the author)

Resident strict credit, following the RES_00BA_1693 precedent (VP i20209). Reproduced by par-e on main `ab790918` and
`e37aab8d` (2026-10-20):
- **Unicorn:** 112 cases, all EXIT, body 17/17 starts (the two dead starts asserted), 14 CPU words and all 1,048,576 RAM
  bytes, flag masks 0. Composed members are fully executed: 00DC 77/77, 006C 46/46, 0000 48/48, 137C 24/24, 13B1 27/27,
  1742 43/43, 17AE 43/43, 181A 12/12. Intr 69/69, GetTime 19/19 and LongMul 16/16.
- **Native:** 42 cases equal. RESULT.BIN reproduced as `74957ab9…` against `01182f30…` above. It is a per-run receipt,
  because the live `IN 61h` byte enters it in the 20 masked cases. The comparison passes every run.
- **C:** 18 checks under strict `-Werror` ASan/UBSan. The checker gate passes on current main.
- **Identity, factor 1, checked from the media:** 13 relocated `LCALL 0899:0000` sites, all at instruction boundaries.
  Three are inside **game_verified** functions: `001C_19B0 +1A7B`, `03BD_0E08 +0EBC` and `03BD_10C7 +115B`.
- **Identity, factor 2:** the only subject with a cited document is the sound-state pair `DS:5372/5373`.
  SLOT_A_RESIDENT_DISPATCH_GATE.md tabulates its game-code writers. `[2612]`, `[4A1C]` and `4A02` are named in this
  document without a cited definition, and the identity does not rest on them.
