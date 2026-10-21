# Resident 0802:05CD, open the archive pair and position it: raw CPU, composed (t30)

`pr_r0802_05cd` (`src/recovered/resident_0802_05cd_cpu.c`) is the raw-CPU, full-state model of START.EXE resident
`0802:[05CD,06E7)`, the t30 cluster's archive-open leaf. 0716 calls it at +0775 on every block load. It is the fourth leaf
in the approved bottom-up order (VP i20316), and the first body in the cluster whose lower bodies are real. It composes 025D,
which composes 02EB.

**A body and its evidence; no credit is claimed here.** Identity follows the t30 chain ruling, through its role line below.

The earlier bounded model `pr_rec_res_0802_05cd` ([RESIDENT_DAX_FILE_OPEN.md](RESIDENT_DAX_FILE_OPEN.md), issue #83) stays on
main unchanged. It uses host callbacks and does not meet the raw-CPU bar. Its frame table and path table agree
with the decode below, and the audit's independent statement is written in the same terms. It called 04A9 "IOResult"; the
t29 declaration makes 04A9 **IOCheck**, which is terminal, and this model treats it so.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Archive open | `0802:[05CD,06E7)` | 282 / 118 | `d8067fb72bfede3fb56aa146e941d7bc9524cb23715f8f63e60f4918c65449b5` |

The audit asserts the following, each derived from the decode (the cluster's count rule, below):

- the decode covers the span exactly, in 118 instructions;
- the exact list of 17 far-call sites;
- the exact near-call set `{065Ch: 025D}`;
- the one conditional site, 065E.

## Behaviour

    05CD PUSH BP; MOV BP,SP; SUB SP,456h
    05D4 base [BP+16h] -> [BP-2Ah], extension [BP+12h] -> [BP-53h]   (StrStore, max 40 each)
    05FA name := base + extension at [BP-155h]                         (0B0B, 0B98; the pointer stays pushed)
    0614 message := table[DS:5376] + char(DS:547D) + CS:05CB literal   (DS:2614h + 1Eh x the byte; 0B0B, 0C3B, 0B98 x2)
    064F opened := CALL 025D(name, message, flag DS:557C as AX, f [BP+0Ah])
    065C OR AL,AL; JE 06DA                                             (not opened: AL := 0)
    0660 Assign(g [BP+6], base + extension); Reset(g, 1); IOCheck
    0697 BlockRead(g, header word [BP+0Eh], 2, [BP-55h]); IOCheck
    06B4 header word += 2                                              (16-bit wrap)
    06BB Seek(f, header word zero-extended); IOCheck; AL := 1
    06DE MOV SP,BP; POP BP; RETF 14h

- **f and g are the same file, opened twice** (measured in the original's DOS log). 025D opens f on the base + extension
  name and leaves it open with record size 1. This body then opens g on the same name, reads the first two bytes of the file
  through g into the caller's header word, and positions f at that word + 2.
  - The log for `present-flag1`: FileExists open/close on handle 5, Reset(f) on 5, Reset(g) on 6, a 2-byte read on 6, then
    LSEEK on 5 to 1236h.
- **The header word is a 16-bit value added in place.** FFFEh and FFFFh wrap to 0 and 1. The Seek gets the word
  zero-extended (`XOR DX,DX`), so the archive position is at most FFFFh.
- **A short file leaves part of the header word.** A 1-byte file overwrites only the low byte, and an empty file overwrites
  neither, because BlockRead with a result variable sets no error on a short read. The audit pins both.
- **Mixed I/O modes.** 025D under this body is `{$I-}`, so an absent file is data: AL = 0, and the body returns 0 without
  touching g or the header word. This body's own Reset, BlockRead and Seek are each followed by IOCheck, which is terminal.
  So any failure after 025D succeeds ends at **RUNERROR** with f left open.
- **The prompt message** is assembled here and shown by 0193 inside 025D: the table entry selected by `DS:5376`, then the
  character at `DS:547D`, then the one-byte literal at `CS:05CB`. Its contents are game data, so they stay in the ignored
  media.
- **The flag is a byte passed as a word with a stale AH.** It is the same fact recorded in RES_0802_025D_CPU.md, now
  exercised from its real caller.

## Composition and contract

- **Composed:**
  - the leaf 025D (`pr_r0802_025d`, #1326), with 02EB (#1324) under it;
  - the SYSTEM models StrStore `0B25`, string load `0B0B`, concat `0B98`, char-to-string `0C3B`, Assign `13E2`, Reset
    `1410` (its nested-Close stop composed), IOCheck `04A9`, BlockRead `14FB` and Seek `1563`.
- Each far-call segment word is read live from the code. Each callee must return to the instruction after its call; 025D
  must return to 065Ch. Otherwise the stop is **CALLEE**.
- **PROMPT is a named stop inside 025D**, unchanged from #1326:
  - The model stops at 0193's entry with 025D's frame on the stack.
  - The caller runs the prompt and resumes **this** model at IP = 02ADh.
  - The model runs 025D on, requires it to return to 065Ch, and finishes this body.
- **Declared environment:** INT 21h AH=3Dh/3Eh (through 025D's environment and Reset's nested Close), AH=3Fh under
  BlockRead's contract, and AH=42h under Seek's.
- Stops:
  - **RUNERROR:** IOCheck found InOutRes ≠ 0. Its JMP to `0AF8:00E2` has executed; this is terminal.
  - **DIVIDE:** BlockRead's recsize-0 DIV. It is unreachable here, because Reset sets recsize 1 and a failed Reset is
    RUNERROR first.
  - **SERVICE**, **CALLEE** and **ARGUMENT**, as for the earlier leaves.
- Exactly 1 MiB of RAM. Entry at IP = 05CDh with SP in `[066Ah,FFE8h]`, or resume at IP = 02ADh.
  - 066Ah keeps 025D's entry SP, 46Ah below this body's, at or above 025D's own 0200h bound.
  - FFE8h keeps the `RETF 14h` frame inside the segment.

**Role line (t30 side i):** opens the game's block archive named by 0716's caller (base + extension) as two file records,
reads its 2-byte header into 0716's variable, and positions the archive past it. When `[557C]` is 0, it asks for the disk
through 025D.

## Evidence

Receipt: 2026-10-21 01:19:44 PDT (`resources/0802-05cd/final.json`, sha `aa0f1a82…`), on a branch from main `6990130e`.

**Unicorn: 50 cases, 67 segments, 17 prompts; final stops RETURN 46 and RUNERROR 4.**

- The original runs from the whole relocated image with every callee real: 025D and 02EB, the SYSTEM code and, on the prompt
  path, 0193 with ReadKey `0899:0059`.
- Each segment is compared unmasked on CPU14, all 1,048,576 RAM bytes, the file-service state and the ordered DOS log slice.
- Starts **118/118**; conditional outcomes **2/2**; no flag masks.
- **36 independent checks** compare (final stop, prompts), the header word, AL and f's file position with the statement above.
- Cases:
  - present and absent, flag 1 and flag 0;
  - `[557C]` = 0 with one and two prompts; the flag high byte; type-ahead drained;
  - header words 1234h, FFFEh and FFFFh (both wrap), 0000h, a 1-byte file and an empty file;
  - the three terminal IOChecks: g's Reset failing (the disk gone from that open, directly and after a prompt), the read
    failing, and Seek failing;
  - table indices 3 and FFh; another disk character; a 60-character base; an empty extension; a 29-character table entry;
  - six flag profiles, each with and without the prompt;
  - SP 066A/0800/8001/FFE7/FFE8; SS = DS;
  - 8 random cases.
- **Declared, from the shared file service:**
  - the insert-disk event;
  - three injected faults (the disk gone from the Nth open, the Nth read failing, an LSEEK error answering DOS error 6);
  - the #1326 keyboard contract;
  - an INT 10h stub; INT 21h 2Ch/25h/35h; ports 40h/43h/61h/C0h.
- **Mutation check:** changing the model's `+ 2` to `+ 3` fails the first case on RAM (the header word). The source was then
  restored byte-identical.

**Native DOSBox-X: 20 cases equal, the C model against REAL DOS on real files, on the prompt-free paths.**

- Covered: HA.DAX present (header 1234h) with flag 1 and with flag 0, and HZ.DAX absent with flag 1, each under six flag
  profiles; HB.DAX (FFFEh, which wraps) and HC.DAX (a 1-byte file).
- After the return, the oracle asks DOS for f's position (`LSEEK 4201h, 0`) when f is open, and the audit compares it with
  the model's file service.
- Compared: CPU14, a 2 KiB stack window, `5800:0000+480` (the strings, both FileRecs and the header word) and `DS:4420+10`.
- **Declared:** DOSBox-X's INT 21h use of the 24 bytes below the caller's SP, masked; the oracle closes handles 6–19 between
  cases. There are no flag masks.
- RESULT.BIN `6c7f092cd558faec84f517c7744ee3fab8db756d696cc52b215f9308ea5d1042`. It is quoted per run: an earlier run the same
  night gave `5bbcd2c4…`.
- Binary: `/usr/bin/dosbox-x` 2026.01.02, `99d74601…`. It is **not** the pinned build, which is declared.

**C test.** Built with `-Werror` + ASan/UBSan: **24 hand-derived checks**. It uses no game bytes, and the literal is
synthetic. It covers:

- present → AL 1, both records open, header 1236h and Seek to 1236h;
- the FFFEh wrap;
- absent with flag 1 → AL 0, with nothing else touched;
- flag 0 → PROMPT with the assembled message, then resumed at 02ADh to AL 1 and RETF 14h;
- a failed read → RUNERROR at `0AF8:00E2` with no Seek;
- ARGUMENT, with nothing changed.

**Shared tool change.** `res0802_0716_cpu_audit.FileSystem` gains `gone_from` and `seek_fail`, both off by default. Re-run
with the defaults: 0716 gives 338 segments / 242 starts and 025D gives 57 segments / 24 checks, both unchanged.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0802_05CD',)`. `--compile-only` builds the composed sources with `-Werror` before any media access; it
was verified with the media absent.

    python3 tools/overlay/res0802_05cd_cpu_audit.py --compile-only
    python3 tools/overlay/res0802_05cd_cpu_audit.py --unicorn --native --output resources/0802-05cd/final.json
