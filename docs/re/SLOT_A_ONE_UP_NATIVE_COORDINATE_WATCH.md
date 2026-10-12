# Saved slot-A one-Up native coordinate evidence

This is one DOS v1.3 native replay of exactly one Up. It measures the live
facing-indexed direction bytes, party position bytes, and window-1 branch
word while the coordinate writer executes. It adds no C translation or
game-function count.

Before this replay, deja "poolrad slot A one Up DS 2736 273F facing 6 party
6AAD 6AAF window 1 594 u68 0752 0761 native debugger" found no indexed
session. A shorter deja "slot A one Up" surfaced earlier Claude work on
unit 0x68 Up preflight. The merged
[preflight note](U68_UP_PREFLIGHT_068D.md),
[coordinate-writer note](U68_COORDINATE_WRITER_073B.md), and
[previous native minute watch](SLOT_A_ONE_UP_NATIVE_WRITE_WATCH.md) supplied
the address leads. The pointers, values, guest PCs, caller frame, and original
overlay hashes below were checked anew.

## Pinned private inputs

All original inputs, extracted overlay spans, memory dumps, emulator logs,
and captures stayed under ignored resources/ in this worker's isolated clone.
The new public note contains no original file content or captures; the
listed DS values are numeric live measurements.

| Input | Size | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65,231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206,105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| slot-A SAVGAMA.DAT | 13,137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |

The private DOSBox-X build was the same heavy-debug 2026.01.02 build used in
the previous native watch: upstream commit
5a9c7c53481c2e933948039c33256776653f4faf, executable SHA-256
8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92.
The coordinate-writer body, unit 0x68 +0x073B..+0x07F3 at GAME.OVR file
0x16E06..0x16EBE, is 185 bytes with SHA-256
342da33f91df7b31ba2aa2819f3e2fc6508f9d0b6634f7f22df616788d675fc5.
A separate 12-byte original overlay slice at unit 0x68
+0x1027..+0x1032 (file 0x176F2..0x176FD) has SHA-256
8bec83c0b10da8d4630d5ff4f7f3b27d17b1d5c7c0327e4043627d6608281613.
These hashes were recomputed from the ignored GAME.OVR in this replay.

## Live pre/post values

The private run loaded slot A to displayed 15,4 N at 10:50, then sent exactly
one Up. Its completed display was 14,4 N at 10:51. The heavy debugger showed
DS=14A0 before and after. Facing byte DS:6AAF was 6, so the two indexed
table addresses were DS:(0x2736+6)=273C and DS:(0x273F+6)=2745.
The actual window-1 far pointer at [DS:49D6] was 3A16:0000 both times;
the word was read at 3A16:0594.

| Guest address or field | Before Up | At y-store stop | After Up |
| --- | ---: | ---: | ---: |
| DS:273C, x step for facing 6 | 255 (signed -1) | not dumped | 255 |
| DS:2745, y step for facing 6 | 0 | not dumped | 0 |
| DS:6AAD, party x | 15 | 14 | 14 |
| DS:6AAE, party y | 4 | 4 | 4 |
| DS:6AAF, facing | 6 | 6 | 6 |
| [DS:49D6]+0x594, window-1 word | 0 | not dumped | 0 |
| window-1 +0x594 bit 0 | 0 | not dumped | 0 |

The direction bytes were dumped together from DS:273C..2745; the party
bytes from DS:6AAD..6AAF. The far pointer and word were dumped separately
before and after Up. The mid-step party dump occurred with the guest paused
at the y-store instruction, before that instruction executed.

## Coordinate-store PCs and caller

Two heavy-debug code breakpoints were set before Up and confirmed by BPLIST.
Both stopped during the single Up, before their respective instructions
executed. The loaded unit 0x68 code segment in this run was 2E60.

| Stop | Guest CS:IP | Live registers | Original-byte instruction at unit offset |
| --- | --- | --- | --- |
| x store | 2E60:0752 | DS=14A0, SS:BP=1D16:3DBE, AX=00FF, DI=0006 | +0x0752 adds AL to byte DS:6AAD |
| y store | 2E60:0761 | DS=14A0, SS:BP=1D16:3DBE, AX=0000, DI=0006 | +0x0761 adds AL to byte DS:6AAE |

At the x stop, the live frame at SS:BP=1D16:3DBE held return
offset [BP+2]=1031 and return segment [BP+4]=2E60.
Original-byte decoding of unit 0x68 +0x1027..+0x1032 shows a near call
to +0x073B at +0x102E with return address +0x1031. The frame therefore
identifies the caller of this coordinate-writer invocation. The value of
the caller's local movement flag and any earlier decision path were not
sampled. The breakpoints identify execution of these two stores in this
one saved route; they do not enumerate all possible writes to the party
bytes on other routes.

## Reproduction

Use the pinned private slot-A inputs only under ignored resources/. The
[previous native watch](SLOT_A_ONE_UP_NATIVE_WRITE_WATCH.md) gives the
heavy-debug build and Xvfb launch commands. Prepare a fresh run with
tools/oracle/prepare_run.sh and load slot A with the same delayed internal
sequence, AUTOTYPE -w 160 -p 8 enter l , a, omitting Up. Confirm the
15,4 N 10:50 screen before entering the debugger with Alt+Pause.

In the heavy debugger, use EV DS. In this run DS was 14A0. Dump each region
with MEMDUMPBIN and copy its private MEMDUMP.BIN before issuing the next dump:

    MEMDUMPBIN DS 273C 0A
    MEMDUMPBIN DS 6AAD 3
    MEMDUMPBIN DS 49D6 4
    MEMDUMPBIN 3A16 0594 2

Decode [DS:49D6] as a little-endian offset and segment and substitute the
resulting window-1 address; do not assume that 3A16 is fixed. For this run
the loaded unit 0x68 segment was 2E60. In another run, confirm the
candidate segment by privately dumping its +0x073B..+0x07F3 span and
comparing SHA-256 with the pinned 185-byte body before arming breakpoints.
For this run the commands were:

    BP 2E60:0752
    BP 2E60:0761
    BPLIST
    RUN

Send one Up to the focused private DOSBox-X window. At the first stop,
EV CS IP DS SS SP BP AX DI and MEMDUMPBIN SS BP 20 record the x store and
caller frame. RUN reaches the y-store breakpoint; dump DS:6AAD for the
mid-step party bytes. RUN again, wait for the completed 14,4 N 10:51 screen,
pause with Alt+Pause, and repeat the four pre-Up dumps for the post-state.
Keep all dumps and captures under ignored resources/.

This observation establishes the direction bytes, coordinate-store PCs,
caller return address, party-byte delta, and window-1 bit for this single
saved Up. It does not establish an unobserved entry-1 branch condition or
C-native save/load/one-Up parity.
