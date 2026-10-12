# Saved slot-A Up: live resident 0x08AE:0x00DC dispatch

This note records one native DOS v1.3 replay through the resident dispatch
called by unit `0x68:+0742`. It establishes the reached branch and caller
return for this saved Up. It does not establish C-native parity, complete
callee effects, or a game-specific identity for the resident routine.

## Prior evidence and private inputs

Before this replay, `deja "poolrad slot A one Up relocated RES_08AE 10D2
00DC live mode active argument branch native replay"` returned no matching
session. The ignored prior run-004 checkpoint at `2E60:0742` has RAM SHA-256
`e48859eafce9d1f3afd4cf8b9fc85eeedd66fc9929e8818bd34e85f755826bb7`.
The live call target was rechecked here. Reused public bounds and control-flow
maps: [resident dispatch](RESIDENT_8AE_00DC.md),
[local leaves](RESIDENT_8AE_LEAVES.md), and
[coordinate writer](U68_COORDINATE_WRITER_073B.md).

| Private DOS v1.3 input | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |

The private DOSBox-X heavy build has SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
Run 005 loaded slot A, displayed `(15,4,6)` at `10:50`, and received
exactly one Up after the breakpoints were armed. At the first reached caller
stop, `2E60:0742`, DS was `14A0`, the party tuple was `(15,4,6)`, window-0
words `+018E/+0190/+0192` were `(0,5,10)`, its far pointer was
`3996:0000`, and window-1 `+0594` bit 0 was 0.

## Runtime address and code check

The live far-call instruction at `2E60:0742` targets `10D2:00DC` and returns
to `2E60:0747`; its five-byte span has SHA-256
`b170afc35fa600f58afbe7778e009c3645d75f69a1582dd60711e2014cd6b9b8`.
The runtime resident segment is `0x0824` paragraphs above the source
paragraph `0x08AE`. The 159-byte dispatch body at `10D2:00DC` exactly equals
`START.EXE` file `0x08BD3..0x08C71`, SHA-256
`aae1673000ebd85e67c66df978d2b986de8e4d52cdd7994ade46f1ac5d50bb2a`.
The merged source note decodes this body as 77 instructions and ends it at
`RETF 2` at `+0178`.

The reached `+1742` and `+17AE` leaves each have 108-byte source bodies.
Their runtime bodies differ from the source only at `+0009..+000A`, where
the segment word is relocated from `08AE` to `10D2`:

| Leaf | START file offset | Source SHA-256 | Runtime SHA-256 |
| --- | ---: | --- | --- |
| `+1742` | `0x09F1F` | `6128c5126be22f692937ccbb5e5eb2b07c85081a04f086e9bb84579486258484` | `7ec58106cd94f6689c9aacfaafe0f771b604bb43eea59a7770288b7dec9dff63` |
| `+17AE` | `0x09F8B` | `8bb73768c1c77c46b1695935e45b9ab162b9c7863c94529cbbffa248001c52a8` | `c9b63cbffb4478d2e7af973f0d9578f51492a7b88b02d045e27c0c4ee32597f9` |

## Reached frame and branch

At `10D2:00DF`, after the prologue, `DS=14A0`, `SS:BP=1D16:3DB6`,
byte `DS:[5372]=1`, word `DS:[5374]=100`, and word `[BP+6]=10`.
The frame holds return `2E60:0747`. The complete observed stop order is:

| Order | Guest CS:IP | Numeric observation |
| ---: | --- | --- |
| 1 | `2E60:0742` | Caller stack argument 10; mode 1; active 100. |
| 2 | `10D2:00DC`, `+00DF`, `+00E9`, `+00F1` | Entry, mode read, active read, then first local call site. |
| 3 | `10D2:137C` | First reached callee; return word `+00F4`. |
| 4 | `10D2:0137`, `+0144` | Later mode read still 1; active word 100; argument 10; mode-1 call site selected. |
| 5 | `10D2:1742` | Far-call argument 9; return `10D2:0147`. |
| 6 | `10D2:0156`, `+0162` | Later active read still 100; positive-active tail selected. |
| 7 | `10D2:006C` | Far-call argument 10; return `10D2:0165`. |
| 8 | `10D2:0169`, `+17AE` | Leaf argument 0; return `10D2:016C`. |
| 9 | `10D2:0170`, `+1742` | Second `+1742` visit, argument 0; return `10D2:0173`. |
| 10 | `10D2:0174`, `+13B1`, `+0178` | Final local callee, then dispatch `RETF 2`. |
| 11 | `2E60:0747` | Caller resumed with `DS=14A0`, `SP=3DBE`; mode 1, active 100, argument 10. |

At `2E60:0747`, before the coordinate stores, party `(15,4,6)` and
window-0 words `(0,5,10)` remained unchanged. The separate native
[coordinate watch](SLOT_A_ONE_UP_NATIVE_COORDINATE_WATCH.md) measures those
later stores; this note does not extrapolate their effects from the resident
return. The live route reaches both local writer leaves, but their complete
effects here and the external effects of `+137C`, `+006C`, and `+13B1` were
not measured. The bounded C models are not an end-to-end Up execution.

## Private checkpoint hashes and replay recipe

RAM dumps, the save, emulator trace, and screen captures remain under ignored
`resources/oracle-runs/slot-a-resident-live-005/`. Selected 640 KiB dump
SHA-256 values identify the stops without publishing their contents:

| Stop | RAM SHA-256 |
| --- | --- |
| `2E60:0742` | `2c231a83bee1655a737ceb48cbc877bb66aff85e31b0f48cf5d1b1cd6d94309e` |
| `10D2:00DF` | `6379c6f8109ea0fd77bb5a4ef5d953a56ba6ea292edca798c1011c9bf0b378d9` |
| `10D2:137C` | `80f3699041573694efff2f1199b8ca6cb492c40f617c301189dde15de0e20aaa` |
| `10D2:0137` | `ba58db8f02658bdc950c440860de7f2ec3b9542c9d08324d63b88acc2d6b5a84` |
| `10D2:1742` argument 9 | `44d0f9c46494487907b3db3ee5aa1b5ee68799ba49e9807e7a9aacd18cfe198f` |
| `10D2:0156` | `0e20a37c03f209a4090c443699569da89452c8a5b4e3557f513141f3e9677dd0` |
| `10D2:006C` | `7587274616be62887c89d4b9f93555b3ccedc2669b67d441ed66935f3469feea` |
| `10D2:17AE` argument 0 | `a7229f7c9bf63549d8eb12b78d3a88002aa9d17b85b4b57b83f36e82a337a48c` |
| `10D2:1742` argument 0 | `c30135de53d709a888844d94d71015badf997568ecec14e7089f9fb3c5a3d9d5` |
| `2E60:0747` | `455f22ee6705bb6aaddeb6adb23bbc0d89ac97b4519165123739a21a92326e20` |

Reproduce only with the ignored hash-pinned DOS inputs and heavy debugger:

```sh
tools/oracle/prepare_run.sh slot-a-resident-live-005
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a tools/oracle/launch.sh resources/oracle-runs/slot-a-resident-live-005 -time-limit 1200 -c "AUTOTYPE -w 160 -p 8 enter l , a"
```

At the playable saved view, pause with
Alt+Pause, arm the guest addresses in the stop table, continue with F5, and
send one Up. `EV CS IP DS SS SP BP` and
`MEMDUMPBIN 0000 0000 A0000` provide the numeric samples and private dumps.
No GitHub or external network action was used for this local evidence run.
