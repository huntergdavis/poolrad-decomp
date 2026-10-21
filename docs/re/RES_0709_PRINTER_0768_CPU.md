# RES_0709_0768: the text-window printer, raw CPU (VP i20411)

`pr_r0709_printer_0768_cpu` (`src/recovered/resident_0709_printer_0768_cpu.c`, header
`include/poolrad/recovered/resident_0709_printer_0768_cpu.h`) is an instruction-faithful model of START.EXE resident
`0709:[0768,09E3)`: 635 bytes, 246 instructions, one `RETF 10h`, sha256 `baedd54c…76668`. It is the game's text-window
printer. It copies a Pascal string, word-wraps it into the window `(x1, y1)..(x2, y2)` at the cursor `DS:5E24/5E25`, and
prints each word through `06A7`. When the window fills, it shows the `CS:074E` literal "Press any key to continue" through
`0BF5`, clears the window and continues.

**Identity: paid by par-a (#1346, `docs/re/NEAR_ONLY_GAME_SEGMENT_IDENTITY.md`).** 27 sites in 16 functions call 0768,
including the `game_verified` callers `001C_0A09` (x2), `001C_0F08`, `001C_1EF8` (x6) and `001C_2775`. The body reads and
writes the cursor bytes `DS:5E24/5E25`, which `ECL_CONTRACTS.md` (`0x33 PRINT RETURN`) and `FUNCTION_LEDGER.md` (function 10)
document. One line of that table is now stale: it says 0768 "is port-bound, and `0DD3`, `06A7` and `0BF5` are not yet
modelled". `0DD3` and `06A7` are modelled and composed here, the body itself issues no IN/OUT (pinned in the audit's
`source()`), and only `0BF5` remains (below). I am not editing par-a's document; the correction is proposed in the PR.

**Span.** The audit hashes `[0768,09E3)`, 635 bytes, from the relocated image, and checks that the decode covers it
exactly with one exit at `09E0`. The ledger rows (`resident_loader_ledger.csv`, `resident_callable_ledger.csv`, row 64) end
at `0x07831` = `0709:07A1`, a next-marker 57 bytes in. That end is short; the measured span is the one above.

## What the model does

The header carries the decode-faithful outline. The parts a reader needs:

- **Composition.** The far callees are the SYSTEM leaf models: StrStore `0B25`, set expansion `0D12`, set membership `0DC0`
  and string load `0B0B`, plus resident `0899:0104`. The near callees are `0DD3` (window clear, on the bus), `06A7` (word
  printer: glyph `01DF`, UpCase, CRT Delay, on the bus) and `0714` (skip spaces). Each far call's segment word is read live,
  and each callee must come back to the instruction after its call, or the model stops CALLEE.
- **Separators.** `0D12` expands the 4-byte set at `DS:0D16` (`02 70 00 8C`, chars 32..63), which is `!,-.:;?`. The audit
  derives that set from the media and prints it (`separators` in the receipt).
- **One named stop: `STOP_BF5`.** `0BF5` is not yet modelled; par-c's model will compose here. The model executes
  `PUSH CS; CALL 0BF5` (diagnostic site `0996`) with its two arguments on the stack: the word `0Dh` and the far pointer
  `SS:BP-11Fh` to the copied literal. The caller runs `0BF5` and resumes the model at `IP 0999h` with the post-callee
  state. The audit compares at the callee entry and resumes from the original's own post-`0BF5` snapshot.
- **Loop budgets (VP i20444: "a model loop is one of ours and it needs a budget").** Every loop-back JMP of this body counts
  against a declared budget that is fresh on each entry: `word_loopbacks` for `09C8 JMP 081A`, and `scan_loopbacks` for
  `0858`, `089F` and `08F0` together. When a JMP would run past its budget, the model stops LIMIT with IP at that JMP and none
  of it executed, and the diagnostic site is the JMP. The audit stops the original at the same arrival and compares
  everything.
- **The port contract, in four conditions:**
  1. This body issues no IN, OUT or INT itself (the audit asserts it in the decode).
  2. Every OUT inside `0DD3` and `06A7` goes through the bus's `pr_port_env`, then `pr_ega`, in order on both sides.
  3. Inside `0BF5` (original side only), ReadKey's driver ports `40h/43h/61h/C0h` and `0626`'s EGA SEQ/GC programming
     (`3C4/3C5/3CE/3CF`) are allowed and recorded apart.
  4. Any IN or INT outside `0BF5` is refused, so it fails the case.

## Finding: the original never returns when x1 > x2

The argument checks at `0783` bound each coordinate separately (`x1, x2 <= 27h`, `y1 <= 18h`, `y2 <= 27h`). They do not
require `x1 <= x2`. Suppose `x1 > x2` and a pass starts at a word with `i = n`, that is, a one-byte last word. Then
`j - i + [5E24] = x1 > x2`, so the word never fits. The pass wraps (`[5E25] += 1`), `0714` skips nothing (`i < n` is false),
and `096C` skips the window-full path (`i < n` is false again). `09BE` loops back, and nothing else changes. The row byte
cycles with period 256, and the original never reaches its `RETF`.

- **Catcher 1: the original's own state, not the model.** In the audit case `x1-gt-x2-hang` (window `(10,1,5,5)`, text
  `"A"`), the original's full state at the 300th and the 556th arrival at `09C8` is identical: CPU14 and all 1 MiB. Between
  them there are 0 OUTs and 0 VRAM writes, and any IN or INT outside `0BF5` refuses. A deterministic machine that repeats
  its whole state without reading input never returns.
- **Catcher 2: the independent statement.** With its own pass budget (4096), the statement reports "does not finish" for
  that case.
- **Catcher 3: the C test.** `run(3)` and `run(259)` leave byte-identical CPU and RAM, and 601 passes leave row
  `1 + 601 = 90 (mod 256)`.

The model reports the hang as LIMIT at `09C8`. This is the only input class I found where the word loop does not
terminate. Every other pass either calls `06A7`, which advances `i` to `j + 1`, or wraps a row, and at most `y2 - y1 + 1`
wraps reach the window-full path, which prints. I have not proved that the class is exhaustive, so the loop keeps its budget
whatever the class turns out to be. The scans are bounded by `n <= 255` per pass but carry a budget all the same.

Separately, `06A7`'s own index is a byte (its header: "Index255 wraps"). A word ending at `j = 255`, which needs a
255-byte string whose last word reaches the end, never ends inside `06A7`. `06A7`'s declared LIMIT covers it, and the
statement caps that loop at 256 draws.

## Dead code: 9 starts and 3 outcomes excluded, with the argument

`0914 JG` was taken to reach `0919`, so `AX > BX` (signed) there. `0919..0939` recomputes the same `AX = j - i + [5E24]`
and `BX = x2` from the same four bytes (`[BP+10h]`, `[5E24]`, `[BP-103h]`, `[BP-104h]`). No instruction from `08F2` to
`0939` writes memory, so the values cannot differ. Hence `0939 CMP AX,BX` gives "not equal", and `093B JNE 0955` is always
taken. The argument assumes no hardware interrupt changes those bytes in between; the model and the audit have no
interrupts.

- **Excluded starts (9):** `093D, 0941, 0943, 0945, 094A, 094C, 0950, 0951, 0952`. This is `MOV AL,[BP-104h]; XOR AH,AH;
  MOV DI,AX; CMP [BP+DI-100h],20h; JNE 0955; DEC [BP-104h]; PUSH BP; PUSH CS; CALL 06A7`.
- **Excluded outcomes (3):** `093B` falling through to `093D`; `094A` taken to `0955`; `094A` falling through to `094C`.
  The last two are reachable only through the first.

The exclusion fails closed both ways: the audit fails if an excluded start or edge is ever reached, and if any other start
or edge is missing. The model keeps the dead branch as decoded, so it is translated rather than deleted.

## CRT Delay: pinned by count, a visible CPU cost

When `DS:4A00 <> 0`, each glyph `06A7` draws runs CRT Delay `0A40:029C`, a pure CPU busy loop. Its loop-backs per glyph are
`4·cal·M − 1`, where `M = ([49E2]·10) & FFFFh` and `cal = [84FC]` (0 means 65536), and there are none when `M = 0`. The audit
counts the original's taken loop-backs at `0A40:02C1/02C6/02B8` and checks them against this formula in 3 cases
(`delay-on`, `delay-speed-3`, `delay-speed-0`). It also checks 0 loop-backs with the gate off. The delay has no observable
effect except elapsed CPU, so the count is how it is pinned.

**Performance note (VP i20413).** On a modern host this busy loop is a visible CPU cost. Its length is a fixed iteration
count, not a time, so a faithful port spends the same number of iterations per character. A port that wants 1991 speed has
to replace it with a timed wait. That would be a deliberate, declared deviation, not a translation.

## The cursor

`DS:5E24/5E25` is pinned per case by the independent statement:

- the home on entry when the cursor is outside the window;
- the home after a clear;
- the wrap `([5E24] := x1, [5E25] += 1)`;
- the home before `0BF5`;
- the final `09CB` wrap when the cursor ends past `x2`, which is the case `end-past-x2`.

The row is a byte: the x1 > x2 case shows it wrapping through 255 to 0.

## Evidence

Receipt `resources/0709-0768/final2.json` (ignored), from `res0709_printer_0768_cpu_audit.py --unicorn`, run at 07:43-07:50
PDT on 2026-10-21:

| Claim | Check that pins it |
|---|---|
| CPU14, all 1 MiB outside the A000 aperture, ordered OUT stream and ordered VRAM write operands equal at every stop, unmasked (`flag_masks: 0`) | Unicorn differential, per segment (two-phase across `0BF5`). It shares the declared environment with the model, so it cannot see a wrong environment. |
| 57 cases: RETURN 52 (9 of them through `STOP_BF5` and resume), LIMIT 5 | differential |
| Starts 237/246, plus the 9 excluded by the argument above; conditional outcomes 47/50, plus 3 excluded | coverage assertion in the audit, fail-closed both ways |
| Cursor, ordered glyph draws `(char, x, y)` seen at `01DF`'s entry, clear count, window-full count (34 cases) | `statement()`, an independent restatement from the decode. Its mechanism is not ours: it reads the original's RAM and its `01DF` entry frames. |
| CRT Delay loop-backs (3 cases plus gate-off) | the formula against the original's counted loop-backs |
| LIMIT sites `0858`, `089F`, `08F0`, `09C8` stop the original and the model at the same JMP arrival | differential, plus a diagnostic-site check |
| Budgets are fresh per entry | `word-budget-after-bf5`: `word_loopbacks = 1` and three `0BF5` seams. The case RETURNs equal on both sides, so each segment took at most one loop-back. The total across segments is not measured, so this does not show that a carried-over count would have stopped it. |
| x1 > x2 never returns | the original's state repeats after 256 passes, and the statement does not finish (above) |
| DIRTY flags `0202, 0602, 0A87, 0402, 0E57` on entry (DF and TF set, high bits) do not change the result | 5 differential cases, also statement-checked |
| Entry SP `0300, 8001, FFEB, FFEC` | 4 differential cases |
| `0BF5` path: literal copy, arguments, resume | differential across the seam; C test checks the stack layout |
| Model rejections change nothing; LIMIT, STOP_BF5 and resume behave as declared | `tests/recovered/test_resident_0709_printer_0768_cpu.c`, 27 checks, ASan and UBSan clean, no media |

**Mutation.** Two mutants run under a 900-second cap each:

- the row increment at `095B` removed (the mutant that hung the first attempt before the budget existed);
- the loop budget off by one.

A mutant that hits the cap is reported as "not terminated", never left running. Results, 2026-10-21 07:55-07:58 PDT:

- **Row increment at `095B` removed: CAUGHT in 38 s** at case 5 (`last-char-overflow`): `C LIMIT original STOP_BF5`. Before
  the budget, this mutant spun for 67 minutes at the same case.
- **`LOOPBACK` with `>` for `>=` (one pass too many): CAUGHT in 101 s** at case 29 (`word-budget`): `CPU word-budget#0/ax C
  000b original a20b`.

The source was restored byte-identical after each mutant (cmp with a saved copy).

**Native DOSBox-X corroboration: NOT DONE.** This is an open gap. The leaves `06A7`, `0DD3`, `01DF`, `0626` and `05EB`
have their own native runs, but 0768 composed does not. Adapting `res0709_text_06a7_cpu_native.py` means a guest harness
that plants the literal, the separator set and the `0BF5` ReadKey input. If a native run is required before credit, this
document is updated when it lands. The packaged-dosbox caveat applies to the leaves' runs as stated in their documents.

## Reproduction

```
poolrad-heavy.sh .venv/bin/python3 -u tools/overlay/res0709_printer_0768_cpu_audit.py --unicorn --output resources/0709-0768/final2.json
.venv/bin/python3 tools/overlay/res0709_printer_0768_cpu_audit.py --compile-only     # CHECKER_BUILD_OK, no media
ctest -R poolrad_resident_0709_printer_0768_cpu_tests
```

Each case prints a START line and a result line on stderr, with its stop sequence, the number of draws and the elapsed
time.
