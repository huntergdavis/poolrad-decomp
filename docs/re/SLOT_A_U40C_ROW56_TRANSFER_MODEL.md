# Saved slot-A row-56 EGA transfer: bounded four-plane C model

At the reached post-wall flush, a portable C model takes the offscreen
object bytes, pre-flush `A200` staging planes, pre-flush `A000` page
planes, row metadata and EGA register state. It predicts the eleven-byte
`A200:0A03..0A0D` staging span and the `A000:0A03..0A0D` page span
in all four planes. Against a **same-run immediate post-flush** native
capture, the model matches **44/44 staging bytes and 44/44 page bytes**.
The native dirty flag and extent words also reset as decoded.

This models the reached **row-56 video-state-1 path** within a larger
778-byte flush. It is neither a full translation of that flush nor a
new strict full-C/game-identity count. It does not predict a screenshot
or palette colour.

Before debugging, `deja "Poolrad RES_0464 1B52 row56 A200 A000 write
mode 1 Move 11 bytes latch"` found no exact indexed session. Reused
and independently checked the [post-wall transfer](SLOT_A_U40C_DISPLAY_TRANSFER.md)
(PR #319), [first EGA byte](SLOT_A_FIRST_EGA_INDEXED_BYTE.md)
(PR #322), [row-64 four-plane parity](SLOT_A_FIRST_EGA_ROW64.md)
(prior local work), and [video page setup](RESIDENT_VIDEO_INIT_4426.md).

## Original bounds and reached frame

Ignored `START.EXE` is SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Capstone 5.0.7 reaches the exact ends of both spans:

| Original | START file range, exclusive end | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| `RES_0464:1B52..1E5B` | `[0x61CB,0x64D5)` | 778 / 305 | `7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c` |
| runtime `0AF8:1685..16A8` Move | `[0xC2FB,0xC31E)` | 35 / 16 | `1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972` |

The private run stopped at `0C88:1B52` with return
`2D14:05FB`, outer return `2D14:0DAE`, then at the immediate
return `2D14:05FB`. The pre-flush 640 KiB RAM SHA-256 is
`d102de8a62ef0895916263d20da1fd3b58e45ccc6ed15eedb22bb5514f292fc0`;
the immediate post-flush RAM SHA-256 is
`87799350e1bec55a5e6b9b3c283f7c2cd11e8309ccd336f0f535fc64205d4466`.
The live video-state byte `DS:[4A1E]` is 1; the selected active and
draw page is `A000`. Row 56 is dirty, with first/last interleaved
object indices 4712/4755, row base `0A00`, low offset 2, and
`DS:[5060]=0140`. The length is
`((4755-4712)>>2)+1=11`; staging and page offsets both equal
`0A00+2+1=0140+1+56*40+2=0A03`.

## Write and latch sequence

| Offset | Decoded effect |
| --- | --- |
| `+1BD1..+1BD6` | Select Sequencer map-mask register 2 at `3C4/3C5`. |
| `+1C09,+1C4F,+1C96,+1CDE` | Select the four map masks from `DS:[25F6..25F9]`; the live values are 1, 2, 4, 8. |
| `+1C37,+1C7E,+1CC6,+1D0F` | For each plane loop, write CPU byte `object[0x17+first+plane+4*j]` to `A200:0A03+j`, `j=0..10`. |
| `+1D31..+1D49` | Set map mask `0F`, then Graphics Controller register 5 write mode 1. |
| `+1DA0` | Call runtime `0AF8:1685` with `A200:0A03` source, `A000:0A03` destination, length 11. |
| `+1DB0..+1DB5` | Restore Graphics Controller write mode 0. |
| `+1E25..+1E53` | Three FillChar calls reset dirty flags to 0, first extents to `FFFF`, last extents to 0. |

Before the native flush, Graphics Controller registers 0, 1, 3, 4 and 5
read 0, and register 8 read `FF`. The debugger changed only read-map
register 4 to sample planes, then restored it to 0 and selector 5
before resuming. Thus mode-0 staging writes replace the enabled plane
bytes with CPU source bytes. The model rejects any other register
configuration. At stop `0C88:1B52`, the exact debugger sequence was
`OUTP 3CE 00; INP 3CF`, then the same pair for selectors `01`,
`03`, `04`, `05` and `08`, followed by `OUTP 3CE 05`.
The six `INP 3CF` results were `00/00/00/00/00/FF`. Those results
were observed **interactively**, not captured by a persistent native
debugger log. The ignored numeric transcription
`resources/oracle-runs/slot-a-u40c-row56-transfer-p5-001/gc-flush-entry.txt`
is SHA-256
`9cbcf4528ea3c3300faccfd2d40776d61f1d4a093a09d8a38464a7f5c2506ed3`.
The read-only checker pins that record and passes its values into the
C probe. Its hash protects the documented input from drift; it does
not independently reproduce the debugger read.

The runtime Move body loads far source and destination from its stack
frame at `+1689/+168D`, length at `+1691`, executes `CLD`,
compares source and destination offsets at `+1696`, and reaches
forward `REP MOVSB` at `+16A1` because both offsets are `0A03`.
Each source read at `A200` loads all four EGA latches. In write mode 1
the `A000` write uses those latch bytes, while map mask `0F` enables
all four planes. This is a four-plane transfer even though the CPU
receives only its selected read-map byte. The documented EGA behavior
matches the [DOSBox-X VGA memory implementation](https://github.com/joncampbell123/dosbox-x/blob/master/src/hardware/vga_memory.cpp):
its read handler loads all planes into the latch, its write-mode-1
operation returns the latch value, and its write handler applies the
map mask. The C model performs that latch transfer explicitly.

## Private parity and scope

The ignored replay is
`resources/oracle-runs/slot-a-u40c-row56-transfer-p5-001/`.
The read-only checker pins both RAM hashes, all 16 pre/post 11-byte
plane-capture hashes, the private GC numeric-record hash, both original
code hashes, exact instruction sites and the caller frame. It takes
the **pre-flush** RAM/object, pre-flush plane files, and GC numeric
record as model inputs. The immediate post-flush plane files are used
only for comparison; no expected game byte array is embedded in C or
Python.

| Comparison | Match |
| --- | ---: |
| Offscreen interleaved source to native post-flush `A200` | 44/44 |
| Native post-flush `A200` to native post-flush `A000` | 44/44 |
| C-model `A200` to native post-flush `A200` | 44/44 |
| C-model `A000` to native post-flush `A000` | 44/44 |

Native `DS:[4A28+56]` changes 1→0; first/last extent words change
4712/4755→`FFFF`/0. The C model applies this row-56 reset after
the transfer. Synthetic tests use nontrivial four-plane object, staging
and page values; they cover all-plane latch transfer, map-mask overlap,
retaining an unselected staging plane, a short host object, and an
unsupported EGA state. The original does not perform those host bounds
checks. Other dirty rows, other register configurations, other EGA
timing effects and full-flush parity remain outside this gate.

```sh
cmake -S . -B resources/build-u40c-row56 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u40c-row56 \
  --target poolrad_resident_464_row56_transfer_tests \
           poolrad_slot_a_ega_row56_transfer_probe
ctest --test-dir resources/build-u40c-row56 \
  -R '^poolrad_resident_464_row56_transfer_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/replay/slot_a_ega_row56_transfer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-u40c-row56-transfer-p5-001 \
  --probe resources/build-u40c-row56/poolrad_slot_a_ega_row56_transfer_probe
```

All DOS files, RAM, plane captures and screenshot artifacts stay in
ignored `resources/`. This branch is local under the owner's no-GitHub-write
instruction.
