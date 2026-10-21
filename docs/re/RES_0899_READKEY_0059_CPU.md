# RES_0899_0059: the game's ReadKey with hotkeys (raw CPU, composed closure) — strict-credit candidate

`pr_r0899_readkey_0059_cpu` (`src/recovered/resident_0899_readkey_0059_cpu.c`) models START.EXE resident
`0899:[0059,0104)`. This is the second half of the funded wall (VP i20005, task t28). It composes the merged models of
everything it calls, each through its real interface:

- Crt ReadKey and KeyPressed (#1285, #1268), with INT 16h as the declared service;
- the 08AE driver entries `00DC` and `017B`, with their members, through the composition file from
  [RES_0899_QUIT_0000_CPU.md](RES_0899_QUIT_0000_CPU.md);
- SYSTEM set IN and string compare (covered leaves);
- ParamStr (#1284), which runs its real bytes against a PSP;
- `RES_0899_0000` QUIT (#1291), with its Intr and its Halt EXIT.

This PR is stacked on #1291, which supplies the QUIT model and the driver composition.

**Necessary, not sufficient.** On a fresh queue export of main `07cc429a` (442 rows), **302 rows** have `RES_0899_0059` in
their closure, and 303 have `RES_0899_0000`. par-a measured the wall as **sufficient for exactly one row**
(`OVL_007F_0294`). The 302 counts rows this body blocks. It is not a promise that 302 rows become ready.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| ReadKey with hotkeys | `0899:[0059,0104)`, loader-expanded image `[089E9,08A94)` | 171 / 61 | `4a361032123f0bdc7fa2ff158dc2d8f690716ee716462f66bda9740f32e3362f` |

The audit asserts the structure of the span:

- **Far calls:** eight, at +0060 (ReadKey), +007F (00DC), +0094 (set IN), +00A1 (017B), +00B0 (ParamStr), +00BA (strcmp),
  +00E4 (KeyPressed) and +00ED (ReadKey). Their segment words are loader relocations, which the model reads live.
- **Near call:** one, `CALL 0000` at +00DB.
- **Conditional jumps:** nine.
- **Return:** a single `RETF` at +0103.

It also asserts two constants in CS:

- the set at CS:0033 is {0,1};
- the literal at CS:0053 is `'STING'`.

## Behaviour

    0059  PUSH BP ; MOV BP,SP ; SUB SP,102h        [BP-2] key, [BP-1] result, [BP-102h] string[255]
    0060  key := ReadKey
    0068  key = 13h (Ctrl-S): if [5372] <> 2 then [5373] := [5372]; 08AE:00DC([2612]); [5372] := 2     sound off
                              else if [5373] IN {0,1} then [5372] := [5373]; 08AE:017B                  sound on
    00A6  ParamStr(1) into [BP-102h]; compare with CS:'STING'
    00C1  only when equal: key = 04 toggles [49F7]; key = 03 does PUSH CS; CALL 0000 (QUIT, never returns)
    00DE  if key <> 0: while KeyPressed do key := ReadKey     drain, keeping the last key
    00F7  result := key ; AL := result ; MOV SP,BP ; POP BP ; RETF

**The STING gate and what "feasible" means.** The key 04 and key 03 branches are reachable only when the first
command-line word is exactly `STING`. The audit **exercises them from synthetic state**, a PSP tail that reads `STING`.
It does **not claim that they occur in normal play**. Feasible is what the coverage claims; occur is not (VP i20061).

## Identity (two-factor standard, #1195)

- **Factor 1 (reached from game code).** There are relocation-confirmed far fixups to `0899:0059` in the
  function-accounting callgraph, regenerated on this branch from main `07cc429a`. The callers are classified on the
  same fresh export:

  | Caller | Classification | Call site |
  | --- | --- | --- |
  | `OVL_03BD_0E08` | game_verified | +0EAF |
  | `OVL_03BD_10C7` | game_verified | +114E |

  There are five further game_evidenced sites in four functions (`OVL_0031_0FC1` ×2, `OVL_03B0_03BD` ×2, `OVL_040C_003B`,
  `OVL_040C_015D`) and one unresolved caller (`OVL_007F_0294`). The resident `0709:0BF5` also calls it, at +0C31.
- **Factor 2 (documented game state).** The body's subject is game state:
  - the sound-state pair DS:5372/5373, whose writers are in SLOT_A_RESIDENT_DISPATCH_GATE.md;
  - the driver argument DS:[2612];
  - the flag DS:[49F7].

## Contract and stops

- Exactly 1 MiB of A20-off RAM; IP = 0059h.
- Entry SP must lie in `[0112h,FFFEh]`. This body's deepest own push is entry SP − 112h, at the set-IN and ParamStr
  calls. Deeper frames are guarded by the composed models, whose refusal is a named stop.
- **Declared environment.** It is RES_0899_0000's (INT 21h 25h/35h/2Ch, ports, the INT 10h handler, 006C's limit), plus
  **BIOS INT 16h** for the Crt models: AH 00h and 01h, where any other AH is ENV. Nothing else is declared: ParamStr,
  set IN and strcmp are pure memory.
- Stops:
  - **RETURN**: AL is the key;
  - **EXIT**: STING plus key 03 reaches Halt through QUIT;
  - **BREAK**: a Crt ReadKey took its Ctrl-Break path; this is EXIT class;
  - **CRT**: Crt SERVICE, ENV or ARGUMENT;
  - **DRIVER**;
  - **LEAF**: set IN, strcmp or ParamStr refused;
  - **QUIT**: QUIT stopped without reaching Halt;
  - **LIMIT**: the drain loop ran `drain_limit` times. It stops at +00E4, before the next KeyPressed;
  - **TRANSFER**: a callee returned elsewhere. This is defensive and is reached in no case;
  - **ARGUMENT**.

## Validation

Receipt: 2026-10-20 21:53:10–21:53:53 PDT (`final.json`), on this branch stacked on #1291, from main `07cc429a`.

**The closure runs from the original bytes**, exactly as for RES_0899_0000. The whole loader-expanded image (`a42a57e6…`) is
loaded at a test base with the loader's own 561 relocations applied. No modelled byte is patched.

**Unicorn: 115 cases** (**103 RETURN, 10 EXIT, 2 BREAK**). Each compares unmasked CPU14 and all 1,048,576 RAM bytes, and
the stop, and requires an identical ordered environment event log: every `INT 16h`, `INT 21h`, `IN` and `OUT`.

- **Body: 61/61 starts and 18/18 conditional outcomes**, accounted by successor at all nine sites.
- **Every member's code executed.** Each count is executed over decoded starts on the original span:

  | Member | Executed / decoded |
  | --- | ---: |
  | Crt ReadKey | 14/14 |
  | Crt break check | 3/3 |
  | Crt KeyPressed | 8/8 |
  | ParamStr | 7/7 + 11/11 |
  | ParamStr word walk | 24/24 |
  | set IN | 13/13 |
  | strcmp | 20/20 |
  | QUIT | 17/17 |
  | 00DC | 77/77 |
  | 137C | 24/24 |
  | 13B1 | 27/27 |
  | 1742 | 43/43 |
  | 17AE | 43/43 |
  | 181A | 12/12 |
  | 006C | 46/46 |
  | RES_08AE_0000 | 48/48 |
  | Intr | 69/69 |
  | GetTime | 19/19 |
  | LongMul | 16/16 |
  | **017B** | **20/21** |

  The one 017B instruction not executed is `JMP 01A3` at +018B. It runs only when [5372] = 2 at 017B's entry. 0059's call
  site cannot produce that: at +009B it stores [5373], already tested IN {0,1}, into [5372] immediately before the call.
  The instruction is infeasible from this caller, and 017B's own evidence covers it.
- **Two kinds of fidelity** (VP i20175): each member's behaviour rests on that member's own evidence, and the
  composition rests on this run.
- **Independent semantics.** In every case that does not use the break flag or SS = DS, the audit checks the original's
  result against a separate Python statement of the body. That statement covers:
  - the Crt key protocol, including extended-key buffering and the synthesised AL = 3;
  - the Ctrl-S sound toggle;
  - the STING gate and the 49F7 toggle;
  - QUIT restoring [5372] from [5373];
  - the drain.

  It must agree on the stop, on [5372]/[5373]/[49F7], and on the returned AL.
- Cases:
  - Ctrl-S × [5372] 0/1/2 × [5373] 0/1/2/5;
  - sound-off × [5374] 0/1/100h × argument 0/1/FF/5;
  - sound-on × [5374];
  - eight PSP tails (`' STING'`, empty, `' STING foo'`, bare `STING`, `' sting'`, `' STINGX'`, tab, `' xSTING'`) × keys
    04/03/'a';
  - [49F7] 0/1/5/FF; QUIT with three modes and sound states;
  - drains, including an extended key mid-drain; an extended first key; a zero word (the synthesised 3); a buffered scan
    code;
  - Ctrl-S with a drain; Ctrl-S across midnight and with a 1-hundredth clock (006C's guard and long wait);
  - Ctrl-Break pending, alone and mid-drain;
  - six flag profiles × three keys; low and odd SP; SS = DS;
  - 24 random cases.

**Native DOSBox-X: 44 cases equal** (39 RETURN, 3 EXIT, 2 BREAK). Each compares:

- CPU14;
- the 1 KiB stack;
- DS:4A00+20h, 5370+8, 2610+4, 49F6+2 and 8500+4;
- `08AE:01D0+530h` and `08AE:1378+4`;
- the BIOS mode byte, the shadow vector, the GetTime call count and the number of keys taken.

RESULT.BIN `1a653f1c023dd1f2df9c2049977a14abb5a19b10076cced6b1d1f0ce4557d616`; IMAGE.BIN (at 3000h) `02bac6e8…`. The binary is
`/usr/bin/dosbox-x` 2026.01.02, `99d74601…`. It is **not** the pinned `5a9c7c53` build; this is declared.

The native substitutions are declared, each with its exact location:

1. **The RES_0899_0000 set:** the INT 21h hook for 25h/35h/2Ch, the real-byte INT 10h stub, the Halt capture
   (`33 C9 33 DB BA` → `EA …` at native `0AF8:00E9`), and the GetTime DOS frames.
2. **INT 16h hook.** It replays the case's key script: AH=00h takes the next word, and AH=01h sets ZF in the IRET frame
   when the script is empty. It uses no stack of its own. The real INT 16h frame below each Crt INT-time SP is outside
   the Crt models, as their contract states, and is masked at the recorded addresses.
3. **BREAK capture, a change to the native copy only.** At native `0A40:014B` the five bytes `C6 06 02 85 00`
   (`MOV BYTE [8502],0`, the first instruction of the Ctrl-Break path the model stops before) are replaced by
   `EA <capture> <oracle CS>`. The receipt records both the before and after bytes.
4. **IN 61h, live.** The masked bits are derived by two C runs and asserted to be SF/ZF/PF only, in at most two bytes,
   including Regs.FLAGS. That happens in the **3 cases** that reach 13B1 through QUIT. In RETURN cases 13B1's AND flags
   are dead: ParamStr's `OR DX,DX` overwrites them before anything stores flags. Those cases are compared with no mask.
5. **The PSP** is a synthetic one at 5800h: the count byte at :0080, the tail from :0081, then 0Dh.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **24 hand-derived checks** pass. CTest 1/1. The test uses no game bytes: it
places its own CS literal `"HELLO"`, which the model reads live like any literal. It covers:

- a plain key, with the return, the frame bytes, the ParamStr string in the frame and the final flags;
- a drain, and LIMIT;
- Ctrl-S off (00DC through 181A) and Ctrl-S muted with [5373] outside the set;
- Ctrl-S on reaching 017B's 137C, then a DOS refusal: a DRIVER stop;
- the literal gate: the 49F7 toggle both ways, EXIT through QUIT, and no QUIT without the literal;
- BREAK;
- ARGUMENT, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0899_0059',)`. `--compile-only` builds the 22 sources with `-Werror` before any media access; it was
verified with the media absent.

    python3 tools/overlay/res0899_readkey_0059_cpu_audit.py --compile-only
    python3 tools/overlay/res0899_readkey_0059_cpu_audit.py --unicorn --native --output resources/0899-readkey/final.json

## What composition readiness still waits on (not this body's evidence)

The same accounting lines as RES_0899_0000:

- the Intr coverage row;
- the Halt boundary line;
- the remaining 08AE membership rows;
- RES_0899_0000's own credit (#1291).

## Proposed rows (TEXT ONLY; the acceptance lands them)

    tools/overlay/resident_inventory.py  GAME_SPECIFIC_TARGETS  += (0x899, 0x0059)
    docs/re/strict_full_c_addresses.csv:  RES_0899_0059,docs/re/RES_0899_READKEY_0059_CPU.md
