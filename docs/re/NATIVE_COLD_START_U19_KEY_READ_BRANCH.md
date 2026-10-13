# Native cold-start unit 0x19 keyboard read and Return branch

One DOS v1.3 no-argument cold start sent exactly one Return while the credits
frame was visible. The key remained queued at resident `0824:0122`, reached
unit `0x19` entry 1, and was read by BIOS keyboard service `INT 16h`,
`AH=0`, at live `1264:031D`. The unit's reached Return comparison was at live
`0F2D:0AEC..0AF2`. This is a **native route observation** to a runtime/BIOS
boundary, not a code-word validation model, C-native parity result, or new
game-function count.

Before debugging, `deja "Pool of Radiance unit 0x19 code word keyboard read
Return branch"` found no focused match. The earlier [key timing
experiment](NATIVE_COLD_START_CODEWORD_KEY_TIMING.md) established that a
queued Return disappears inside unit `0x19`, and the [resident loader
map](RESIDENT_LOADER_MAP.md) supplied the original-to-loaded segment mapping.
The static [entry-4 note](U3CE_ENTRY4_01C8.md) labels resident
`0x899:0104` as a key-flush hook. The live path below checks that call
independently: its jump goes directly to its epilogue and the queue does not
change there. That older label is therefore not evidence of a flush on this
path.

## Inputs and code identity

| Private input or tool | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| copied-input manifest | `adf69e4405ea60fe45037c018efe0b3bcdd72c357ff16927dae9a1d879735b55` |
| private DOSBox-X 2026.01.02 heavy executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The ignored run is `resources/oracle-runs/native-codeword-read-005`; its
copied-input manifest passed after the run. Unit `0x19` has an original
`GAME.OVR` code descriptor at file `0x09AC`: 862 code bytes, SHA-256
`f93ae8c41b823421115bf6d882588ac3577095d1f9ddace6f487a3bcae33291b`.
Entry-1's descriptor alias `+0x08FD` resolves to loaded `+0x00FD` at
`38D3:00FD`. The stub `0840:0025` jumped there. The MZ runtime load base was
`0x0824`, so original resident segments `0x709`, `0x899`, and `0xA40`
appeared live at `0x0F2D`, `0x10BD`, and `0x1264`.

These hashes are over **original** code, before live segment relocation:

| Original span | Length | SHA-256 | Reached role |
| --- | ---: | --- | --- |
| unit `0x19:+0x02DD..+0x02E1` | 5 | `ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356` | far call to `0x899:0104` |
| unit `0x19:+0x02FD..+0x0301` | 5 | `32ca391ccf9a6a85b6399ab782aa4a24a56c5030ffca617a8a31265fee3c8541` | far call to `0x709:09E3` |
| `0x709:0A2B..0A2F` | 5 | `78ac1ee627344be09af407d3f8b6810a574e97e3ca9dd2bdffd75607c19d45ef` | far call to `0x899:0059` |
| `0x899:0060..0064` | 5 | `949e6d5d0e9be04acf10f23a8d0e859bd37068b8a9e49e4f473a7cfc5256de57` | far call to `0xA40:030F` |
| `0xA40:031D..031E` | 2 | `9c8a394d3384eea8df986985eba6e46e9c15c9434c700ed1da9c063cbb418c4b` | BIOS keyboard interrupt |
| `0x709:0A38..0A3B` | 4 | `8b479bce5244e2ae2ffa03d092b879494053b81cb6a3498e5756f6da38030358` | compare with `0x20`; below branch |
| `0x709:0AA1..0AA4` | 4 | `34db97d32827745fa4c229a3a32f847cdc1d22408acd1aadeae40191a189ed82` | compare with `0x08`; unequal branch |
| `0x709:0AEC..0AF2` | 7 | `86e5180ce7a3340f964582c93e889e57374b2c2b50b366b15dc31493b672e2e4` | compare local key with `0x0D`; equal branch |
| `0x899:010A..010B` | 2 | `6d228d2c7763c1f1c7688ad2d6c49aa341db030ad604ad3d25c61a600072d39c` | direct jump to `+0x011E` epilogue |

The original expanded-image offsets for these resident spans are the
original segment times 16 plus the listed offset. The BIOS interrupt spans
image `+0xA71D..+0xA71E`; the Return comparison spans image
`+0x7B7C..+0x7B82`.

## Live caller chain and keyboard handoff

The BIOS queue head/tail words are at physical `0x041A/0x041C`; slot
`0x001E` is at physical `0x041E`. At `0824:0122`, the head/tail were
`0x001E/0x0020`, with one queued `0x1C0D` scan/ASCII word. The live stops
below occurred after that single Return; no second key was sent. Guest BIOS
tick was 583570 at every listed call and branch stop.

| Order | Guest CS:IP | Reached operation and caller evidence | Queue head/tail |
| ---: | --- | --- | --- |
| 1 | `38D3:02DD` | call `10BD:0104`; `SS:SP=1D16:3DB6`, `BP=3EF8` | `001E/0020` |
| 2 | `38D3:02E2` | return from `10BD:0104`; its `+010A` jump skipped to `+011E` epilogue | `001E/0020` |
| 3 | `38D3:02FD` | call `0F2D:09E3`; `SS:SP=1D16:3DAA`, `BP=3EF8` | `001E/0020` |
| 4 | `0F2D:0A2B` | call `10BD:0059`; `SS:SP=1D16:3A78`, `BP=3DA4`; frame at `BP`: `3EF8,0302,38D3` | `001E/0020` |
| 5 | `10BD:0060` | call `1264:030F`; `SS:SP=1D16:3970`, `BP=3A72`; frame at `BP`: `3DA4,0A30,0F2D` | `001E/0020` |
| 6 | `1264:031D` | `INT 16h`, `AX=0000`; `SS:SP=1D16:396C`; far return words `0065,10BD` | `001E/0020` |
| 7 | `F000:CFC5` | post BIOS keyboard callback `002E`; `AX=1C0D`; interrupt return words `031F,1264,7246` at `SS:SP=1D16:3966` | `0020/0020` |
| 8 | `1264:031F` | returned from BIOS read; `AX=1C0D` | `0020/0020` |

The debugger's physical `BPM 0040:001A` reported an initial baseline change
to `0x001E` at `F000:CFC0`; that was not a write. The first subsequent
value-changing event was `0x001E → 0x0020` inside DOSBox-X's BIOS keyboard
callback `002E`, whose post-event stop was `F000:CFC5`. The guest instruction
that requested the read is `1264:031D INT 16h`, `AH=0`. This emulator
callback is the observed consumption boundary; no ordinary guest x86 store
instruction for the queue-head update was observed.

The `1264:031D` private RAM dump hashes
`cae84bbffb1edc8f7905b050417bcd4d707ff1056ebec9c8aec30301b46242de`;
the `F000:CFC5` and `1264:031F` dumps both hash
`0f43aa9f686fbabe348de432a6e6ceba0751f5acf82697f4eb01f655ae56b649`.
The old `0x1C0D` slot word remains after head/tail become equal.

## Reached branch after the read

At `0F2D:0A2B`, the line-input callee called the wrapper above. It then
stored returned `AL=0x0D` at local `[BP-0x012B]`. The same numeric local
value appears in the RAM dumps at both later branch stops.

| Guest branch | Measured inputs | Observed next stop |
| --- | --- | --- |
| `0F2D:0A38` compare `AL,0x20`; `+0A3A` below branch | `AX=010D`, `FLAGS=7287`, carry set | `0F2D:0AA1` |
| `0F2D:0AA1` compare `AL,0x08`; `+0AA3` unequal branch | `AL=0D` | `0F2D:0AEC` |
| `0F2D:0AEC` compare local key with `0x0D`; `+0AF1` equal branch | `AX=010D`, `FLAGS=7246`, zero set, local key `0D` | `0F2D:0AFD` |

At the `+0AF1` branch, queue head/tail remained `0x0020/0x0020` and the
private RAM dump hashed
`5e3a151d57d1efbe04460da67b13de4463cec7291177ca2fffe132b5a69fd1d3`.
The callee returned to unit `38D3:0302` with `AX=0000`, whose stack frame
still links to resident `0824:0127`. The final resident stop had the same
empty queue. This proves the Return-specific branch for this run; it does
not determine how a nonempty code word is judged or what later screen path
all possible inputs take.

## Reproduction boundary

Prepare a fresh ignored run with `tools/oracle/prepare_run.sh`; use the
hash-pinned heavy DOSBox-X and no program arguments as in the [native route
procedure](NATIVE_COLD_START_U17_PICTURE_ROUTE.md). Launch `DEBUGBOX START`.
At MZ entry, arm `BP 0824:0122` and `BP 0824:0127`. Send one Return at the
credits RGB hash
`624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698`.
At `+0122`, confirm `0x001E/0x0020` at physical `0x041A/0x041C` and
`0x1C0D` at `0x041E`; then arm successive breakpoints at the guest addresses
in the caller and branch tables. At `1264:031D`, arm `BPM 0040:001A` and
resume through the BIOS callback. Use `EV CS IP DS ES SS SP BP AX FLAGS`
and `MEMDUMPBIN 0000 0000 A0000` at each stop; move each dump before
resuming. Check caller frames through little-endian words at `SS:BP`, the
queue through the BIOS data area, and the original span hashes with
`tools/overlay/resident_loader_map.py`'s `unpack` helper plus the pinned
overlay descriptor. Screenshots, RAM images, debugger trace, game files,
and executable bytes stay only under ignored `resources/`.
