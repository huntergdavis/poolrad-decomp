# OVL_007F_0294: the game's numeric entry line editor (raw CPU, composed closure) — strict-credit candidate

`pr_u7f_number_0294_cpu` (`src/recovered/overlay_u7f_number_0294_cpu.c`) models GAME.OVR unit 7Fh `+[0294,0441)`, which is
the routine the game uses to ask "how much?" and read an amount up to a maximum.

**Price.** VP i20226 approved this row **above** the 132 B-per-row guide. The record of that decision:

- it is 429 B for one row;
- the ceiling is a heuristic for choosing between candidates, not a prohibition;
- this is real game logic whose subject is documented money state;
- it composes the credited wall.

## Media span (and the 440 vs 429 correction)

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| numeric entry | unit 7Fh `+[0294,0441)`, GAME.OVR `0x24F1D…` | **429** / 188 | `66d7b9df95a5aad97b6d084354e56f72b70122a796018be24e48ab21605aafe8` |

**The queue's 440 B is not the body.** The executable body ends with `RETF 8` at +043E, at 429 B. The 11 bytes after it
are the next function's length-prefixed string `"\nOverloaded"` (0Ah, then `Overloaded`); the audit asserts those bytes.

The audit also asserts:

- 188 instructions and a single `RETF 8`;
- the ten conditional sites;
- **17 far calls, each carrying exactly one of the unit's 17 fixups (its segment word)**. These are loader relocations,
  read live.

## Behaviour

The arguments are `[BP+6]` colour, `[BP+8]` the far prompt string and `[BP+0Ch]` max (a word). The frame is 244h bytes.

    prompt := prompt[1..40]           StrStore 40              -> [BP-2Bh]
    clear row 24, columns 0..39       0709:05EB (0, 18h, 0, 28h)
    print prompt at row 24            0709:0626 (colour [BP+6])
    column := length(prompt)          [BP-2Eh] promptlen, [BP-2Ch] column
    maxtext := Str(max:5)             0AF8:1366                -> [BP-34h] (right-aligned in 5)
    text := ''                        [BP-3Bh] string[6]
    repeat
      key := RES_0899_0059            [BP-3Ch]
      '0'..'9': text := text + key (string load 0B0B, char-to-string 0C3B, concat 0B98, StrStore 6);
                v := Val(text) (0AF8:13B1);
                if v > max then text := maxtext, column := promptlen + length(maxtext)   (the clamp)
                           else column := column + 1;
                print text at row 24, column promptlen, colour 0Fh (0709:0626)
      08 with text <> '': text := Copy(text, 1, length-1) (0B57; StrStore 6); clear one cell at column (05EB); column -= 1
    until key = 0Dh (Enter) or 1Bh (Esc)
    clear row 24 (05EB); AX := 0 for Esc, else Val(text)                                   RETF 8

`RES_0899_0059` drains every pending key and returns the last one. So one pass of this loop consumes everything typed
since the previous pass; see `drain-group` below.

## Identity (two-factor, #1195): factor 2 is argument-borne

**Factor 1.** Three game_evidenced callers, on a fresh export of main:

- `OVL_0077_2156`, relocation fixup at +23F5;
- `OVL_0077_24A7`, fixup at +26F3;
- `OVL_007F_0CEB`, a near `PUSH CS; CALL` at +0F0E.

**Factor 2 — argument-borne, and why this is not 06FB.** This body has **no game-global memory operand**: its own operands
are its arguments, its locals and the screen (through the 0709 routines). That is the same shape as `OVL_03B0_06FB`, which
was **refused** on factor 2 as a display service with no memory operand.

The difference is the argument. #1195 counts state passed as an argument when the body **uses** it, and every caller
passes `max` from documented game state:

- **`OVL_0077_2156` (+23E1…23E7) and `OVL_0077_24A7` (+26DF…26E5):** `LES DI,[5D92h]` loads the far pointer to the active
  character record (ECL_LOAD_HOOK_6BA0_SIGNED, FUNCTION_LEDGER). The caller pushes `ES:[DI+88h+2i]`, one of the record's
  money words at +88h, +8Ah … +94h (U7F_DIV5_ENTRY10, U77_WEIGHTED_28BE, U7F_RECORD_SUBTRACT_0020).
- **`OVL_007F_0CEB` (+0F00):** it pushes the word at `DS:[67F4h+4i]`. U7F_RECORD_SUBTRACT_0020 documents those same record
  words being added into the DS:67F4/67F6 dwords.

The body **uses** `max`, not merely forwards it: it formats `max` with `Str` for the clamp text, and compares against it
(`CMP DX,[BP-3Eh]` / `CMP AX,[BP-40h]`) to clamp the entry. The routine is therefore "enter an amount of money up to what
the character (or the pool) has". A reader comparing it with 06FB should see that only the argument makes them different.

## Contract and stops

- Exactly 1 MiB through the bus **with an EGA attached**, as the printer and space contracts require; IP = 0294h.
- Entry SP must lie in `[025Ah,FFF4h]`: the body's frame and pushes reach entry SP − 25Ah. Deeper frames are guarded by
  the composed models.
- **The key loop is a named LIMIT stop.** `key_limit` counts iterations of the +03FF `JMP 02FB` loop. The model stops at
  +02FB before the next key call when the budget is spent. The audit's budget is 64.
- Stops:
  - **RETURN**: AX is the value, or 0 on Esc;
  - **EXIT**: STING + key 03 inside RES_0899_0059 ends the process;
  - **BREAK**: RES_0899_0059's ReadKey took the Ctrl-Break path;
  - **CALLEE**: a composed callee stopped otherwise;
  - **LIMIT**;
  - **TRANSFER**: defensive, reached in no case;
  - **ARGUMENT**.
- **The chain is all credited or covered, not a mix with models** (VP i20226 condition 4). `RES_0899_0059` and
  `RES_0899_0000` are now **credited** (#1302). Under them, the Crt, ParamStr, Intr, Halt, GetTime and 08AE members are
  covered, declared or driver_internal. The SYSTEM string routines, Str and Val (#1303) are covered. 0709:05EB and 0626 are
  resident_covered.
- **One argued infeasible outcome.** `JG` at +0363 taken would need `0 > hi(Val(text))`, and `text` only ever holds digits
  and blanks:
  - appended digits;
  - `Str(max:5)`'s right-aligned clamp text;
  - `Copy` of those.

  So `Val` yields 0…999999 or the error result 0, and its high word is never negative. The fall-through is covered.

## Validation

Receipts: Unicorn 2026-10-20 22:29:36–22:34:10 PDT; native 22:36:49–22:37:44 PDT. Both ran on this branch from main
`4850dac6`.

**The closure runs from the original bytes.** The loader-expanded image is loaded at a test base with the loader's 561
relocations. The overlay body is placed at a test segment, with its 17 unit fixups relocated the same way. Nothing is
patched.

**Unicorn: 49 cases** (48 RETURN, 1 EXIT). Each compares unmasked CPU14, all 1,048,576 RAM bytes, the stop, the ordered
INT 16h / INT 21h / IN / OUT event log, **every EGA OUT, all 294,528 VRAM write operands, and the full EGA state**: four
planes, latches and registers. The EGA is an independent Python device (`res00ba_display_1693_cpu_audit.PythonEga`) mapped
as MMIO at A0000h; the C side uses the merged `pr_ega`.

- **Body: 188/188 starts; 19/19 feasible conditional outcomes by successor**, plus the one argued infeasible above.
- **48 independent result checks.** Each compares AX against a separate Python statement of the editor: the clamp to
  `Str(max:5)`, the 6-character text, Backspace, and Esc versus Enter.
- **Key delivery.** Keys arrive in **typing groups**. KeyPressed reports a key only within the current group, so each
  RES_0899_0059 call returns the last key of one group, as a person typing between calls produces. `drain-group`
  (`1 2 3` typed together) shows only `3` arriving.
- Cases:
  - Enter or Esc on empty input; digits; Backspace, including on empty text;
  - the clamp, the exact maximum, and a digit after the clamp; max 0 and max 65535; seven digits (truncated to six);
  - leading zeros; letters ignored; Ctrl-S (the sound toggle) mid-entry; Enter inside a drain group;
  - an empty prompt and a 45-character prompt (cut to 40);
  - STING + Ctrl-C (EXIT) and STING + Ctrl-D;
  - six flag profiles; low and odd SP;
  - 20 random cases.

**Native DOSBox-X, `machine=ega`, video mode 0Dh: 21 cases equal** (20 RETURN, 1 EXIT). Each compares:

- CPU14;
- a 4 KiB stack window;
- **the whole 64 KiB game data segment**;
- **all four EGA planes (256 KiB)**, read back from real emulated video memory through read-map select;
- the shadow vector and the GetTime call count.

RESULT.BIN `00484664…` is a **per-run receipt**: it carries the live IN 61h byte. The binary is `/usr/bin/dosbox-x`
2026.01.02, `99d74601…`; it is **not** the pinned `5a9c7c53` build, which is declared.

The native substitutions are declared, each with its location:

1. **RES_0899_0059's set:**
   - the INT 21h hook for 25h/35h/2Ch;
   - the INT 10h stub;
   - the Halt capture, replacing `33 C9 33 DB BA` at native `0AF8:00E9`;
   - the derived IN 61h mask, needed in **1** case;
   - the GetTime and INT 16h frame masks.
2. **The INT 16h hook** replays the same key groups, with FFFFh separating groups.
3. **EGA.** Mode 0Dh is set once. Before every case the harness resets the graphics-controller registers, fills each plane
   with 31h + 27h·plane and sets the map mask back to 0Fh, matching the C reset plus fill.
4. **What native does not check.** EGA registers and latches are compared by the Unicorn run only (they are not read back
   natively). The EGA OUTs are not traced natively.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **13 hand-derived checks** pass. CTest 1/1. It uses no game bytes, so the
first screen call stops on the space model's own CODE refusal, which the test pins. It covers:

- the frame, the saved BP and the StrStore copy, including the cut to 40 characters;
- the CALLEE stop at +02BA;
- ARGUMENT, including no EGA attached and both SP bounds, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('OVL_007F_0294',)`. `--compile-only` builds the sources with `-Werror` before any media access; it was
verified with the media absent.

    python3 tools/overlay/u7f_number_0294_cpu_audit.py --compile-only
    python3 tools/overlay/u7f_number_0294_cpu_audit.py --unicorn --native --output resources/u7f-0294/final.json

## Proposed rows (TEXT ONLY; par-e accepts)

    docs/re/reviewed_function_identities.csv:  OVL_007F_0294,docs/re/U7F_NUMBER_0294_CPU.md,<this PR>,<reviewed head>,66d7b9df95a5aad97b6d084354e56f72b70122a796018be24e48ab21605aafe8,429
    docs/re/strict_full_c_addresses.csv:       OVL_007F_0294,docs/re/U7F_NUMBER_0294_CPU.md

The identity is argument-borne, as stated above. The row's extent is **429**, not the queue's 440.
