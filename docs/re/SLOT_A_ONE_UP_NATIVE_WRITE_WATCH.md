# Saved slot-A one-Up native window-0 write watch

This is one native DOS v1.3 replay, not a new C translation or a game-function
count claim. It closes the memory-observation gap in
[SLOT_A_UP_ECL_GATE.md](SLOT_A_UP_ECL_GATE.md) and the native-watch blocker in
[SLOT_A_MINUTE_WRITER_BOUNDARY.md](SLOT_A_MINUTE_WRITER_BOUNDARY.md): one Up changes window-0 word
`+0x18E` from 0 to 1, while word `+0x190` stays 5. The first observed
**value-changing** write to either word is unit `0x7D:+0x045D`, reached from
unit `0x68:+0x07ED`. The debugger stops at the following guest instruction,
`0x7D:+0x0462`.

Before debugging, `deja "poolrad slot A one Up native DOS replay DOSBox-X window 0 49D2 18E 190 write watch"`
returned only the current assignment, with no prior replay finding to reuse.
The saved-frame layout in [DOS_SAVGAM.md](../formats/DOS_SAVGAM.md), the
original-byte bounds in [U7D_ENTRY1.md](U7D_ENTRY1.md) and
[U68_COORDINATE_WRITER_073B.md](U68_COORDINATE_WRITER_073B.md), and the oracle
launcher in [docs/oracle/README.md](../oracle/README.md) supplied the starting
addresses and run procedure. The pointer, word values, watch stop, and stack
below were checked anew in the native guest.

## Private input and emulator identity

All original inputs, the emulator build, memory dumps, logs, and screen
captures were kept under ignored `resources/` in this worker's isolated copy.
No original bytes or images are in this note.

| Input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |

The private DOSBox-X build came from upstream tag `dosbox-x-v2026.01.02`,
commit `5a9c7c53481c2e933948039c33256776653f4faf`, configured with
`--enable-debug=heavy --enable-sdl2 --disable-dynamic-core`. Its executable
SHA-256 was `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The installed Ubuntu DOSBox-X 2026.01.02 binary did not include the heavy
debugger's `BPM` memory-change command. The private binary included `BPM`,
and its live `BPLIST` confirmed the four watches. DOSBox-X's
[build guide](https://github.com/joncampbell123/dosbox-x/blob/master/BUILD.md)
documents `--enable-debug=heavy`; its
[debugger source](https://dosbox-x.com/doxygen/html/debug_8cpp_source.html)
shows that `BPM` compares a watched byte at instruction boundaries.

## One-Up observation

The private run loaded slot A and displayed `(x,y,facing)=(15,4,N)` at
`10:50`. The debugger's DS was `14A0`; a guest dump of `DS:49D2..49D5`
gave the live window-0 far pointer `3996:0000`. A guest dump at
`3996:018E..0191` gave these pre-Up words. Exactly one Up key was then sent.

| Window-0 offset | Before Up | After Up | Value-changing watch stops during Up |
| --- | ---: | ---: | ---: |
| `+0x18E` | 0 | 1 | one, low byte `00→01` |
| `+0x190` | 5 | 5 | none |

The completed native screen displayed `(14,4,N)` at `10:51`. A second guest
dump confirmed that `DS:49D2` still held `3996:0000`, and the two post-Up
words were 1 and 5. The final debugger watch baselines were `01,00,05,00`
at `3996:018E..0191`.

`BPM` initially listed `00` as the baseline for **every** newly added watch,
including `3996:0190`, whose pre-existing value was `05`. A `RUN` before Up
therefore produced an initialization stop `3996:0190 00→05`. That stop was
absorbed before the key and is excluded from the Up result. All four watches
then remained armed through the movement and final screen.

## First observed value-changing writer and caller frame

After the one Up, `BPM` reported `3996:018E 00→01` and stopped at guest
`CS:IP = 2F6B:0462`. At that stop the live registers included
`DS=14A0`, `ES=3996`, `SS=1D16`, `SP=3DA2`, `BP=3DB4`, `AX=938E`, and
`DI=938E`. The actual far pointer's offset was zero. The source instruction
at unit `0x7D:+0x045D` stores a word to `ES:[DI+0x6E00]`; the next
instruction is `+0x0462`. The watched change was therefore caused by the
store at guest `2F6B:045D`, rather than by the instruction shown at the
debugger stop.

| Original overlay span | File offsets | Bound | SHA-256 |
| --- | --- | ---: | --- |
| unit `0x7D` entry 1, `+0x03D5..+0x0479` | `0x241A1..0x24245` | 165 bytes, 67 instructions | `7bd8c6d35073b0d7101c4a3fbe2154e916a1f72424636636fcf0b48718f148a4` |
| unit `0x68` coordinate writer, `+0x073B..+0x07F3` | `0x16E06..0x16EBE` | 185 bytes, 66 instructions | `342da33f91df7b31ba2aa2819f3e2fc6508f9d0b6634f7f22df616788d675fc5` |

The bounded original-byte decode around the stop and far caller is:

| Unit offset | Instruction/evidence |
| --- | --- |
| `0x7D:+0x0457` | load window-0 far pointer from `DS:49D2` |
| `0x7D:+0x045B` | add the indexed word offset to DI |
| `0x7D:+0x045D` | `mov word es:[di+0x6E00],dx` — value-changing store |
| `0x7D:+0x0462` | compare local loop byte with 6 — breakpoint reports this next IP |
| `0x68:+0x07E7..+0x07EC` | push word 1, then word 1 |
| `0x68:+0x07ED` | far call via stub `0x8C:0x25` into unit `0x7D` entry 1 |
| `0x68:+0x07F2` | far-call return address observed in the live stack frame |

The stopped callee frame at `SS:BP = 1D16:3DB4` held return offset
`[BP+2]=07F2`, return segment `[BP+4]=2E60`, and arguments
`[BP+6]=1`, `[BP+8]=1`. Thus the live caller was `2E60:07ED`, the bit-clear
arm documented in the coordinate-writer note. The loop's destination
calculation and `ES=3996` connect that call to the watched window-0 word.

This trace identifies the first **value-changing** store observed by four
byte watches after the Up key. A `BPM` comparison cannot detect a store that
writes the same value, so this observation does not rule out earlier or later
same-value stores. It does not establish behavior for other saves, keys, or
window fields.

## Reproduction commands

Run from a terminal with the original inputs only in ignored `resources/`.
The tag and heavy-debug configure switch are required for `BPM`. On the
tested Ubuntu host, Autotools, SDL2 networking headers, and PNG headers were
needed. The private build used:

```sh
sudo apt-get install -y autoconf automake libtool libsdl2-net-dev libpng-dev
git clone --depth 1 --single-branch --branch dosbox-x-v2026.01.02 \
  https://github.com/joncampbell123/dosbox-x.git resources/dosbox-x-heavy-src
(
  cd resources/dosbox-x-heavy-src
  ./autogen.sh
  ./configure --enable-debug=heavy --enable-sdl2 --disable-dynamic-core
  make -j6 CFLAGS='-O0 -g1 -msse -Wno-error=incompatible-pointer-types' \
    CXXFLAGS='-O0 -g1 -std=gnu++14 -msse -Wno-error=incompatible-pointer-types'
)
```

The replay run was a fresh copy made by `tools/oracle/prepare_run.sh`. The
delayed internal key sequence loaded slot A and deliberately omitted Up:

```sh
run_dir=$(tools/oracle/prepare_run.sh slot-a-heavy-repro)
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a bash -c '
  printf "%s\\n" "$DISPLAY" > "$1/display"
  printf "%s\\n" "$XAUTHORITY" > "$1/xauth_path"
  exec tools/oracle/launch.sh "$1" -time-limit 600 \
    -c "AUTOTYPE -w 160 -p 8 enter l , a"
' bash "$run_dir"
```

From another shell, set `DISPLAY` and `XAUTHORITY` from those two private
files, focus the DOSBox-X window, and send `Alt+Pause` at the `15,4 N` screen:

```sh
run_dir=resources/oracle-runs/slot-a-heavy-repro
export DISPLAY=$(cat "$run_dir/display")
export XAUTHORITY=$(cat "$run_dir/xauth_path")
window_id=$(xdotool search --name 'DOSBox-X' | head -1)
xdotool windowfocus --sync "$window_id"
xdotool key --clearmodifiers Alt_L+Pause
```

In the debugger, `EV DS` and `MEMDUMPBIN DS 49D2 4` reveal the actual far
pointer; decode the private `MEMDUMP.BIN` as little-endian offset and segment.
In this run it was `3996:0000`. Enter these commands one at a time, reading
or copying each `MEMDUMP.BIN` before the next dump overwrites it:

```text
EV DS
MEMDUMPBIN DS 49D2 4
MEMDUMPBIN 3996 018E 4
BPM 3996:018E
BPM 3996:018F
BPM 3996:0190
BPM 3996:0191
BPLIST
RUN
```

Substitute the pointer found in a fresh run. `BPM` initializes its listed
baseline to `00`; in this run, the first `RUN` stopped on the pre-existing
`+0x190` byte `05`. Verify and absorb any such stop **before** Up, then
`RUN` again. Send exactly one `xdotool key --clearmodifiers Up` to the focused
private game window. At a watch stop, `EV CS IP DS ES SS SP BP AX DI` and
`MEMDUMPBIN SS BP 30` capture the guest instruction and caller frame. Resume,
wait for `14,4 N`, enter the debugger again, and dump the same four window
bytes and `DS:49D2` for the post-state. Keep all dumps and screen captures
inside ignored `resources/`.
