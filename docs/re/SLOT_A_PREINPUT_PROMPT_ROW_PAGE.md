# Saved slot-A first view: bounded prompt-row page before input

This note stops at the first repeated keyboard poll after unit
`0x1C:+3387` returns from the saved slot-A first-view draw. No gameplay key was
sent. `pr_slot_a_u3b0_prompt_row_apply` models **only the EGA page effect** of
the reached unit `0x3B0:+01DA` helper. It is not a full helper translation, an
input dispatcher, a complete static C frame, or a full-C routine count.

Before tracing, `deja "slot A 3582:3387 first view return next keyboard wait pre
input UI page"` and then `deja "Poolrad unit 3B0 +01DA prompt row 24 glyph draw
key poll 0566 slot A"` each timed out without a result. The checked-in
[gameplay path](GAMEPLAY_PATH.md), [unit `0x1C` loop](U1C_LOOP_3387.md),
[first-view page stop](SLOT_A_CALL1_AFTER_ROW9_COL34_STOP.md),
[font/printer recovery](FONT_8X8D_201.md), and
[printer page model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) supplied address and C
leads. All live page stops below were measured in fresh private replays.

## Pinned source and reached call

| Ignored input or original span | Size / count | SHA-256 |
| --- | ---: | --- |
| DOS v1.3 `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A world save | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X | 47,259,176 bytes | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |
| Unit `0x3B0:+01DA..+037C`, GAME file `[0x2CAA9,0x2CC4C)` | 419 bytes / 172 contiguous 16-bit instructions, ending `RETF 4` | `8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce` |
| Unit `0x3B0:+048B` near call to `+01DA`, GAME file `[0x2CD5A,0x2CD5D)` | 3 bytes / 1 instruction | `a47ae6e9d1136bdf24bdde8a222cffe3c2c5df0d98fec86529b0650bd2bdf4f3` |

The loaded helper at `2116:01DA..037C` hashes
`a6a772f3cf170d69c3e73cac96056ab2689f25811ae148c5e4026c78ac0f02b8`;
the loaded `2116:048B` call hash equals its original hash. The caller's
earlier `+040C` near helper returned with zero page-byte changes. Its
conditional printer at `+0475` was skipped by the saved input's branch to
`+047A`; the page there remained byte-identical to the first-view return.
The next reached call, `2116:048B → 2116:01DA`, accounts for the entire page
delta before the keyboard poll.

## Live frame and branch boundary

At `2116:048B`, the parent frame used `DS=14A0`, `SS=1D16`, `BP=3E98` and
selection index `DS:6B2B=1`. Its copied Pascal text has length 33 and starts
at column 0. The selected character span is one-based positions 1–4; the
selected and ordinary colors are 15 and 10. The character-classification
operand is the 32-byte set at loaded `CS:01BA`; SYSTEM `+0DC0` tests the
character bit. The live font far pointer is `3CB6:0000`. Hashes of the
copied Pascal input, set, and 1,416-byte font are respectively
`1aade9e35370a1f2ee8d8ab9f41906f03d4732644424c627089ba51397db19b4`,
`2d66b9ae33311a4e813f8a5263360a0aaeb0e64d86f53bdeeb2f0849d5e2cbe0`,
and `fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
No text or font bytes are tracked.

The original helper's conditions applied to that frame select four
characters through the selected-span branch, five more through set membership,
and 24 through the ordinary-color branch. Its final call fills six cells
through column 38 with zero-color spaces. These are branch results derived
from the original instructions and the hash-locked live frame; no separate
per-glyph PC trace is claimed. The C model accepts these values as arguments,
uses the merged glyph printer for each character, and refuses cells beyond
column 38 before writing. A zero-length string returns without clearing,
matching the helper's entry branch.

## Page parity and remaining state

| Native stop | Full four-plane page SHA-256 | Difference from preceding stop |
| --- | --- | ---: |
| `3582:3387` first-view return | `3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738` | prior accepted page |
| `3582:3395` before input far call | same | 0 / 32,000 |
| `2E60:09D0` before prompt far call | same | 0 / 32,000 |
| `2116:040C` helper entry and `+040F` return | same | 0 / 32,000 |
| `2116:047A` after skipped printer | same | 0 / 32,000 |
| `2116:048B` drawer entry | same | 0 / 32,000 |
| `2116:048E` drawer return | `09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a` | 518 / 32,000 changed |
| `2116:0566` poll, then same PC again with no key | same as drawer return | 0 / 32,000 |

The native drawer changed 63, 196, 63, and 196 byte values in planes 0–3.
The C model reports 39 cells, 1,248 plane-byte stores, and 518 changed values;
its entire 32,000-byte output equals both the native `+048E` return and the
first and repeated `+0566` wait pages. The repeated no-key wait RAM snapshots
are also byte-identical. The poll call at `2116:0566` targets loaded resident
`1264:02FD`; the observed repeat, with no key sent, bounds this UI interval
before key dispatch. A filtered debugger `INT 16h` breakpoint did not stop in
this run; Alt+Pause supplied the first poll stop, then a code breakpoint
verified the repeat. The exact BIOS interrupt instruction is not claimed.

The drawer entry and return 640-KiB RAM hashes are
`31aaf5f659370812d52f25aa37f0dd1a97c19e5ab645417c69a15b8be602d26d`
and `456a2246da93bfa6d0b5bd32b161bec8eea3d0340d6497d5b05b8a112db1f1d6`.
They differ at 43 guest stack bytes and the BIOS tick byte at physical
`0x0046C`; the C page model does not reproduce those writes. Across entry,
return, and wait, party `(15,4,6)`, window-0 words `(0,5,10)`, map byte 20,
script PC `0x9AA5`, and selection 1 are unchanged. No input dispatch or Up
effect is included.

## Reproduce and scope

Build with strict C11 warnings and run
`poolrad_slot_a_u3b0_prompt_row_tests`. The synthetic test covers selected,
set-member, and ordinary colors; trailing clear; repeat; zero length; and
rejected bounds/arguments. For the private replay, `tools/replay/slot_a_u3b0_prompt_row_audit.py`
hash-locks original and loaded code, ignored RAM and page captures, live frame
values, native deltas, and the complete C output. Pass the hash-pinned ignored
inputs and the `poolrad_slot_a_u3b0_prompt_row_probe` path; the audit emits
only addresses, hashes, and counts. Screenshots, saves, game files, RAM, EGA
planes, and probe data remain under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-u3b0-row \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-u3b0-row -j 4
ctest --test-dir resources/build-slot-a-u3b0-row --output-on-failure
python3 tools/replay/slot_a_u3b0_prompt_row_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --game resources/DOS/POOLRAD/GAME.OVR \
  --save resources/oracle-runs/slot-a-ui-second-cycle-p9-023/POOLRAD/SAVGAMA.DAT \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --writer-run resources/oracle-runs/slot-a-ui-preinput-writer-p9-053 \
  --wait-run resources/oracle-runs/slot-a-ui-preinput-replay-p9-052 \
  --probe resources/build-slot-a-u3b0-row/poolrad_slot_a_u3b0_prompt_row_probe
```

To recapture: launch the pinned heavy debugger with `DEBUGBOX START`, load
slot A, break at `3582:3387`, then at `2116:040C`, `+040F`, `+047A`, `+048B`,
`+048E`, and `+0566`. Dump 640 KiB RAM with `MEMDUMPBIN 0000 0000 A0000`;
for each EGA read plane select Graphics Controller register 4 and read map
0–3 before `MEMDUMPBIN A000 0000 1F40`. Restore read map 0 before resuming.
At the no-key poll, repeat a `+0566` breakpoint without sending a key.
