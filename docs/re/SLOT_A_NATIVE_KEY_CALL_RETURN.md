# Saved slot-A Up: live gameplay input call and return

This address/hash/numeric-only note captures **one invocation** of the gameplay
loop's input call in native DOS v1.3: unit `0x1C:+3395` into unit `0x68` entry
2, one Up, its return to `+339A`, the first window-1 gate at `+33BD`, and the
later pre-RANDOM stop. It does not add C, a routine count, or a C-native parity
claim. There was no debugger memory intervention in this run.

Before tracing, `deja "poolrad slot A one Up key route 1c 32f8 u68 entry2
window1 594 post key pre RANDOM"` returned no indexed match. The previous
[loop model](U1C_LOOP_3387.md), [entry-2 Up slice](U68_ENTRY2_UP_SLICE.md),
[native prompt ABI](SLOT_A_ONE_UP_NATIVE_PROMPT_ABI.md), and
[native AL10 continuation](SLOT_A_ONE_UP_NATIVE_AL10_BRANCH.md) supplied
address and replay leads. All stops and deltas below were measured anew.

## Pinned ignored inputs and loaded code

| Private input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The original unit `0x1C:+32F8..+3507` span, file `0x403E..0x424D`, hashes
`a6f44cd9d3397ea0d96b03f45dfb82fa10c44071a9023a57d8e4979e2260ea79`.
Its five-byte far call at `+3395`, file `0x40DB..0x40DF`, hashes
`b588dcf500a335a6dc95ae0ca430a5ec17d6f1af71a4c94ca6ccf97f6915ae4f`.
The loaded five bytes at `3582:3395` hash
`b280d21502c60f0f32bbfeea9e8ff2d36c2b21a1c7465b1052adee93e2ed7a2d`
and call relocated stub `0896:002A`. The five-byte instruction's return IP
is `+339A`; the debugger observed that return, rather than inferring it from
the decode alone.

The original unit `0x68` entry-2 code, file `0x17054..0x17416`, hashes
`6b79d88f95710c9ceb1f9d6d05a669d9125efd1c0ca34ec9f0911c35dc9e00f8`.
Its first 12 bytes match the loaded body at `2E60:0989` with SHA-256
`019e539c500db7ef9d0577b53126791ef021e8077cdf3f410c8c4f7b520b26ce`;
the full loaded 963-byte code hashes
`56aba6420ada0435e3ab07387823f1468ec4b18e9f10603e9c29e6d10627bc30`.
Relocated call operands explain why the loaded full-span hash differs from the
file span. The stub's own entry was not trapped in this run.

## One-Up call/return checkpoints

The private run was prepared from the pinned inputs and launched **with a
PTY** for the heavy debugger. Visible code-word prompt, main menu, and load
prompt gated Enter and `l`. At the load prompt, before selecting `a`,
`BP 3582:3395` was armed. After `a`, the breakpoint fired before the first
gameplay input call. Resuming displayed the saved `(15,4,N)` view at 10:50;
its RGB HUD crop `(580,540,780,575)` hashed
`bf1e6e8f2804c8317f61029670ef899f5a2c79e77df306d464b8b2551c4fe1a2`
over Pillow's crop byte array. **Exactly one Up** was then sent; no second
game key was used.

| Stop | Live register/frame evidence | Relevant state |
| --- | --- | --- |
| `3582:3395` before far call | `DS=14A0`, `SS:SP=1D16:3EF6`, `BP=3EF8`, `AX=0014`; call target `0896:002A` | party `(15,4,6)`; window-0 words `+018E/+0190/+0192=(0,5,10)`; window-1 `+0594=0`; script PC `9AA5` |
| `2E60:09D5` after input call | `SS:SP=1D16:3EB4`, `BP=3EF0`, `AX=0048`; output flag `SS:[BP-4]=1` | prompt returned Up scan byte `AL=0x48` |
| `2E60:0D4B` before `RETF` | `SS:SP=1D16:3EF2`, `BP=3EF8`, `AX=A048`; stack words at `SS:3EF2/3EF4` are `339A/3582` | window-1 `+0594=0`; party `(15,4,6)` |
| `3582:339A` after `RETF` | `SS:SP=1D16:3EF6`, `BP=3EF8`, `AX=A048` | RAM byte-identical to the preceding `RETF` stop |
| `3582:33BD` before gate comparison | `DS=14A0`, `ES:DI=3A16:0000`; the compared word `ES:[DI+0594]=0` | party `(15,4,6)`; window-1 `+0594=0` |
| `3582:026C` before first RANDOM call | `DS=14A0`, `SS:SP=1D16:3EDC`, `BP=3EE2`, `AX=000E` | party `(14,4,6)`; time `(1,5,10)`; window-1 `+0594=0`; script PC `9B40` |

At all six stops, `DS:[49D2]=3996:0000`, `DS:[49D6]=3A16:0000`, and
`DS:[49DA]=3A96:0000`. Window 2 `+021E` remained 16 through pre-RANDOM.
The original `DS:4428` seed in this run was 304079422 at both the pre-call
and pre-RANDOM stops. This run stopped before its RNG result; no AL10 branch
or post-RANDOM outcome is claimed here.

Each checkpoint is a private 640 KiB RAM dump under ignored
`resources/oracle-runs/slot-a-key-route-live-013/`:

| Stop | RAM SHA-256 |
| --- | --- |
| `+3395` | `4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc` |
| `2E60:09D5` | `ecadcd2bb53e0827c38e509a6b20992033dc5c7a473bfe5ae1886990a713bf52` |
| `2E60:0D4B` | `5ab1d3fa57949ce7c3e2b4dbd0ff1d6d2f96787f0dd13bc8cbc10b44713bda37` |
| `+339A` | `5ab1d3fa57949ce7c3e2b4dbd0ff1d6d2f96787f0dd13bc8cbc10b44713bda37` |
| `+33BD` | `38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3` |
| `+026C` | `d5332052a2367728a88215e36ff182a4f4e7a2bbdde22a7ef2c74497e2f0cf7c` |

## Numeric memory changes at bounded regions

The comparison covers the **entire 64 KiB DS segment** and **all 0x800 bytes
of each of the three far world frames** at the pointers above. It does not
classify changes elsewhere in guest RAM, including stack and display memory.
Differences identify net changes between stops, not the first writer or every
intermediate write.

From `+3395` to u68 `RETF`, DS had 436 changed bytes in 43 contiguous runs.
Seven were below `DS:9000`:

| DS offset | Before → `RETF` |
| --- | --- |
| `43E0`, `43E2` | `44→45` each |
| `43F6`, `43F7`, `43FA` | `96→107`, `46→47`, `89→79` |
| `6B2C`, `84E4` | `0→1` each |

The other 429 DS changes lie in 37 disjoint runs within `C288..C652`. Their
role is **unclassified**. The exact upper-DS address runs are:

```text
C288-C2A1 C2A3 C2A5 C2A7 C2AA C2B2-C2D8 C396-C3A5 C3A7 C3A9
C3AB C3AD-C3AE C3B0-C3C1 C3CC-C3D8 C3DA-C3DB C3DD-C3DE
C450-C454 C456-C45C C45E-C45F C4DD-C4E9 C520-C567 C569-C576
C578-C57B C57D-C57F C582 C584-C58B C58D-C590 C592-C595
C598-C5A3 C5A5-C5C6 C5D2-C5DC C5DE-C5E6 C5E8-C5FC
C5FE-C61A C61C-C63D C63F-C648 C64A-C64F C652
```

The sorted **full-DS** changed-offset list hashes
`c948fdee0e171f88b9ad0ede93f45da1510bf75a108353940a34dc9c4ef03d1a`
when each offset is encoded as a little-endian 16-bit word. During that call,
window 0 changed only at `+03FE:3→0`; windows 1 and 2 were byte-identical.

From u68 `RETF` to the caller's `+33BD` gate, DS words `4436/4438` changed
from `(1455,16384)` to `(8,17562)`, and upper-DS byte `C657` changed
(value not classified). All three 0x800-byte frames were
byte-identical across those two stops. The four-byte DS change follows the
checked-in loop's copies from `DS:5D92..5D95`; this trace alone does not
identify the producer of those source words.

From `+33BD` to pre-RANDOM `+026C`, DS had 866 net changed bytes (293 below
`9000`, 573 above), with sorted changed-offset SHA-256
`1d4bf1dcf778e971b3e7e350beb045a8136770c4611fda64f4cc01d4a3527c24`.
The lower DS runs were `43E0`, `43E2`, `43F6..43F7`, `43FA`,
`471E..4765`, `49ED..49EE`, `4A00`, `6AAD`, `6AB0`, `6F78..6F7A`,
`6F7D..6F7F`, `7006..701A`, `7046..705A`, `7086..709A`,
`70C8..7153`, `84E1`, and `84E4`. Window 0 changed at `+018E:0→1`
and `+01E0:1→15`; window 2 at `+0210:6→10`; **all 0x800 bytes of window 1
remained unchanged**. These later deltas span the movement and script setup
route, so no single callee is assigned all of them.

## Reproduction boundary

Prepare the pinned ignored save in an isolated checkout, start the private
heavy debugger from a terminal/PTY, and keep all captures under `resources/`:

```sh
tools/oracle/prepare_run.sh slot-a-key-route-live-013
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-key-route-live-013 \
  -time-limit 1200
```

Wait for visible code-word prompt, send Enter, wait for menu, send `l`, and
**pause at the visible load prompt before `a`**. Arm `BP 3582:3395`, resume,
send `a`, and capture that pre-call stop. Arm `BP 2E60:09D5`,
`BP 2E60:0D4B`, `BP 3582:339A`, `BP 3582:33BD`, and `BP 3582:026C`.
Resume to the playable view, verify its HUD crop, then send exactly one Up.
At each stop, `EV CS IP DS ES DI SS SP BP AX` and
`MEMDUMPBIN 0000 0000 A0000` provide the private register and RAM samples.
The original media, save, dumps, debugger output, and screenshots remain
ignored; none is tracked in this note.
