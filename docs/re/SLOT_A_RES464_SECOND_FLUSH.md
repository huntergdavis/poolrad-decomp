# Saved first-view second flush: copied dirty rows, zero new EGA values

The saved slot-A route reaches `RES_0464:1B52` twice during the first
view. The second call follows the offscreen masked-byte path measured in
the [first-writer note](SLOT_A_RES464_POSTFLUSH_MASKED_BYTE.md) and the
separate +`0C6E` loop snapshot (PR #421). It processes 88 dirty rows,
but this fresh native run changes **zero** sampled EGA plane bytes in
the complete dirty-row union `[0x03C3,0x1166)` at both staging `A200`
and active page `A000`. The merged bounded
[all-dirty C transfer](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md) predicts
all **27,928/27,928** captured plane bytes and **168/168** post-flush
metadata rows. Thus this reached transfer performs copies and clears
dirty metadata without a new value change in the captured region.
This is not a 64,000-pixel frame comparison or a full translation of
the resident flush procedure.

Before investigation, `deja "Poolrad saved slot A first view RES_0464
offscreen to EGA transfer planes metadata"` was attempted with a
20-second bound and timed out. The merged all-dirty transfer model,
[display-transfer instruction trace](SLOT_A_U40C_DISPLAY_TRANSFER.md),
and first-writer note were reused; the new entry and return were
independently captured and checked.

## Native boundary and original code

Ignored DOS v1.3 `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its resident flush file span `[0x61CB,0x64D5)` is 778 bytes / 305
aligned instructions, SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
Runtime `Move` at file `[0xC2FB,0xC31E)` is 35 bytes / 16 aligned
instructions, SHA-256
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.
The four reached stage stores are at `+1C37`, `+1C7E`, `+1CC6`, and
`+1D0F`; `+1DA0` calls `Move`, whose forward `REP MOVSB` is `+16A1`.

The new ignored run `slot-a-res464-postflush-transfer-p5-031` used
the hash-verified saved-A inputs. Debugger stops were ordered:

| Stop | Native frame | RAM SHA-256 |
| --- | --- | --- |
| First flush entry | `0C88:1B52`, `SS:SP=1D16:3E94` | earlier first-flush audit |
| Second flush entry | `0C88:1B52`, `SS:SP=1D16:3EBE`, return `2D14:05FB`, outer `2D14:0DAE` | `bb6b86553477f8185a12153d2901ebafc3b907b32c436c6b01f1cb51e3b12324` |
| Immediate second return | `2D14:05FB`, `SS:SP=1D16:3EC2` | `227c649d491add1df356e405c6c0c635541f9ad91dad4a7c695fc76fecd95a` |

The private debugger transcript SHA-256 is
`e2e37a39dc8bd2feef3e57c8446d4452602c1b5c99e3eabb2f0f1b0c0bd4da30`.
At second entry, `DS=14A0`, video state 1 selects page `A000`,
`DS:[4A20]` resolves to offscreen object physical `0x3DE48`, and the
object SHA-256 is
`489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1`.
That hash matches the earlier independently captured view-four flush
object. The preceding +`0C3D` snapshot from PR #421 is a **different
native run**. Its measured writer address `0x3F0DD` has the same value
at this second-flush entry, but these captures do not prove an
uninterrupted same-run handoff from that exact instruction.

## Plane and metadata result

Entry row flags select exactly rows 16 through 103, with 11 byte
positions per row and plane. The union is `[0x03C3,0x1166)`; 968
positions per plane are dirty and the intervening captured positions
are clean. Entry Graphics Controller registers 0/1/3/4/5/8 read
`00/00/00/00/00/FF`. All four pre/post planes at `A200` and `A000`
were captured over the 3,491-byte union in the **same run**. Each
plane's entry and return SHA-256 is identical:

| Segment | Plane 0 | Plane 1 | Plane 2 | Plane 3 |
| --- | --- | --- | --- | --- |
| `A200` | `ad9c8e64…eaba24f` | `129fe74a…506b` | `ac0461dd…66bc` | `9b1c0382…de54` |
| `A000` | `a6a54c38…41c9eb` | `343503de…bc417` | `f18d65b1…df0d` | `a08fb3ca…f6aeb02` |

The audit pins the **complete** eight hashes for both phases. It
checks 3,872/3,872 interleaved source-to-stage values, 3,872/3,872
stage-to-page values, and the model's 27,928/27,928-byte full-union
output. At return, every one of the 168 dirty flags is zero, every
first extent is `FFFF`, and every last extent is zero. The model
matches all 168 metadata rows. The offscreen object itself is unchanged
by this flush.

The prior writer address is object data offset `0x127E`, plane 2 in
row 56. Its mapped staging/page offset is `0x0A08`. All four sampled
states at `A200:0A08` and `A000:0A08` (entry and return, plane 2)
already equal the offscreen byte at second-flush entry. The prior
first flush and later direct page printers are separate visible
effects; no whole-screen or RGB/palette claim follows from this
zero-delta transfer.

## Reproduction

```sh
cmake -S . -B resources/build-res464-second-flush \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-res464-second-flush --target \
  poolrad_slot_a_res464_second_flush_tests \
  poolrad_slot_a_ega_all_dirty_probe
ctest --test-dir resources/build-res464-second-flush \
  -R '^poolrad_slot_a_res464_second_flush_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/replay/slot_a_res464_second_flush_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-res464-postflush-transfer-p5-031 \
  --loop-run resources/oracle-runs/slot-a-res464-loop-c6e-p5-030 \
  --probe resources/build-res464-second-flush/poolrad_slot_a_ega_all_dirty_probe
```

The synthetic test distinguishes a dirty copy with no value changes
from the same path with one changed source plane byte. The audit reads
only ignored original executables, RAM, and EGA captures, and prints
addresses, counts, and parity. No game media, extracted bytes, pixels,
or screenshots are committed. No strict/full or game-identity count is
claimed.
