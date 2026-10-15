# Saved slot A: first monitored UI printer after one Up

On the hash-pinned DOS v1.3 slot-A replay, one Up from the visible
`(15,4,6)` prompt reaches the resident printer at `0F2D:0626`. It was
the first hit among three armed UI entry breakpoints: `0F2D:0626`
(printer), `0C88:1B52` (flush), and `2116:01DA` (prompt-row drawer).
This ordering does not claim that no earlier hardware or other page
write occurred. The printer returns to `08DE:2B5A`, and the existing
bounded C printer-page model matches **32,000/32,000 bytes** of the
immediate native EGA page at that return.

Before this investigation, `deja "Poolrad slot A exactly one Up first
reached native post-Up printer page writer A000 immediate page frame
RES_0709 RES_0464"` timed out without a matching result. The merged
[first-view printer-page audit](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md) and
[post-row-nine sequence](SLOT_A_AFTER_ROW9_FULL_PAGE_SEQUENCE.md)
provided the `RES_00BA:2B55` call and C probe as leads. This run
independently checks the caller and input frame after the Up replay.

## Input and call proof

Private run: `resources/oracle-runs/slot-a-post-up-first-ui-writer-p9-111/`.
`START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the slot-A save is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private patched DOSBox-X binary hashes to
`c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`.
Entry and return conventional-RAM captures, each 640 KiB, hash to
`5c3aa481884391318a14a9cfe4e76b1446027b72b368065df606c2a37c5c0c23`
and
`24b7c6335e01ec918c823935a52342278ddb81c3150e06d67d07b10dcc63d1e8`.

The original `RES_00BA:2B55` five-byte far call at expanded image
offset `0x36F5` hashes to
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`
and targets `0709:0626`. Its loaded counterpart at `08DE:2B55`
hashes to
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`
and targets `0F2D:0626`. The original 129-byte printer body at
expanded `[0x76B6,0x7737)` hashes to
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`;
the loaded `0F2D:0626..06A7` body hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.

At the printer entry, `CS:IP=0F2D:0626`, `DS=14A0`, `ES=A200`, and
`SS:SP=1D16:3C6E`. Seven stack words are
`2B5A,08DE,3E7D,1D16,A00A,A00F,A011`: far return, far Pascal-string
source, color, row, and column. The source length is 19 and its
length-plus-content SHA-256 is
`e767d5c8ef552436c5e411af72d8eca9581f3feacbb988c67c16bf2b701bc2fe`.
The live font pointer at `DS:5E20` is `3CB6:0000`; its 1,416-byte
buffer hashes to
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:4A1E=1` selects EGA, and the page segments at `DS:4A16` and
`DS:4A18` are both `A000`.

## Immediate page result

Each capture concatenates four 8,000-byte EGA planes. The entry page
SHA-256 is
`b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44`;
the immediate return page is
`a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac`.
Exactly 210 byte values differ: 105 in plane 1, 105 in plane 3, none
in planes 0 or 2. All differences lie at global scanlines 120–127,
columns 17–35, matching row 15 and the 19-cell input frame.

The C probe uses the live font, the stack-derived `length=19`,
`column=17`, `row=15`, `color=10`, the private Pascal source, and the
native entry page. Its private input SHA-256 is
`53fd15433e954797e4b2ef165d7eac15c0accea3db040d956bfe9dedf2042318`.
The existing model reports **19 cells, 608 plane stores, and 210
changed values**. Its output is identical to the immediate native
return page over all 32,000 bytes. Snapshot equality does not exclude
transient or same-value native stores. This is one printer-page
transition; it does not establish joined C one-Up key-dispatch parity
or a new full-routine count.

## Reproduce locally

Prepare the ignored run with `tools/oracle/prepare_run.sh`. In the
private patched DOSBox-X, load slot A and wait for the initial prompt.
Arm `BP 0F2D:0626`, `BP 0C88:1B52`, and `BP 2116:01DA`; verify the
breakpoints, resume, and send exactly one Up. Capture the first
printer-entry RAM and all four EGA planes. Arm `BP 08DE:2B5A`, resume,
and capture that immediate return. Use `MEMDUMPBIN 0000 0000 A0000`
for RAM. For each plane, select EGA read map via `OUTP 3CE 04`,
`OUTP 3CF <plane>`, then `MEMDUMPBIN A000 0000 1F40`. Restore read
map 0 and Graphics Controller selector 5 before resuming. Keep all
captures, screenshots, and source bytes under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-post-up-first-writer-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-post-up-first-writer-p9 --target \
  poolrad_slot_a_full_ega_printer_page_probe
python3 -m unittest tests/replay/test_slot_a_post_up_first_printer_page_audit.py
python3 tools/replay/slot_a_post_up_first_printer_page_audit.py \
  --exe resources/oracle-runs/slot-a-post-up-first-ui-writer-p9-111/POOLRAD/START.EXE \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-post-up-first-ui-writer-p9-111 \
  --probe resources/build-slot-a-post-up-first-writer-p9/poolrad_slot_a_full_ega_printer_page_probe
```

The audit hash-locks media, emulator, RAM, original and loaded
call/body spans, source, font, each EGA plane, and the C probe input;
it checks the exact 32,000-byte C/native output equality. The
synthetic test rejects changed frames, off-row or off-plane deltas,
and a wrong plane distribution. No game bytes, save, text, font,
screenshots, RAM, or EGA payload is tracked.
