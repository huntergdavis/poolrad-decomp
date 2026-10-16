# First-page `RES_0464:+16A1` exact-mode pre-byte bridge

The [C bridge](../../tools/replay/res464_16a1_uvga_exact_bridge.c) accepts
only the measured `P5FIRST` pre-byte mode at the first saved-slot-A page
transfer. It parses the full 32-bit VRAM mask, derives the two planar indices,
and projects the first write from an explicit four-plane source latch. Its
fixture plane values are invented. The private capture and binary stay under
ignored `resources/`; this note contains no captured pixels.

## Evidence and boundary

The bounded `deja "Poolrad RES0464 16A1 C bridge GC6 05 Seq4 06 uint32
memmask mode1 latch"` search returned no matching indexed session. This
work reuses the [loaded-PC bridge](RES464_16A1_CAPTURE_BRIDGE.md), the
[run015 caller preflight](RES464_16A1_RUN015_PREFLIGHT.md), and the
[one-byte projection](RES464_16A1_EXPLICIT_BYTE_PROJECTION.md). Those older
notes predate the present native pre-byte stop; their 16-bit/planar fixtures
remain separate and unchanged.

One caller-first private stop reached loaded `0C88:1DA0`, then
`131C:16A1` with saved far return `0C88:1DA5`, `DS:SI=A200:03C3`,
`ES:DI=A000:03C3`, `CX=000B`, and `DF=0`. The five-line private `P5FIRST`
record is SHA-256
`826c7dc3e0226ea0125cb13446ab9f97bdc666106f16c54a904a1bc89a1ef1f7`.
It reports `uvga` source/page handlers, GC6/GC4/GC5=`05/00/01`,
Seq4/Seq2=`06/0F`, compatible-chain4=`1`, odd/even override=`0`,
VRAM size/mask=`00200000/001FFFFF`, zero read/write banks, and A-window
page base/mask=`A0/0000FFFF`. Source/page physical addresses are
`A23C3/A03C3`; source/page planar indices are `000023C3/000003C3`.
The record includes four source and four old-page plane bytes, kept private.

The private DOSBox-X source fixes the interpretation:

| Source line | Bounded inference |
| --- | --- |
| `src/hardware/vga_seq.cpp:211-225`, `:175-181` | Seq4=`06` leaves chain-four clear, disables odd/even, selects extended memory; Seq2=`0F` enables all planes. |
| `src/hardware/vga_memory.cpp:2793-2832`, `:2842-2853`, `:577-606` | Unchained `uvga` handles both CPU windows; GC6=`05` selects the A0000 page with mask `FFFF`. |
| `src/hardware/vga.cpp:1116`; `src/hardware/vga_memory.cpp:283-325`, `:330-384` | The 2 MiB mask is `001FFFFF`; the compatible-chain4 index formula yields `23C3/03C3`. Read mode 0 loads all four source planes into the latch. |
| `src/hardware/vga_gfx.cpp:110-116`; `src/hardware/vga_memory.cpp:166-191`, `:376-384` | GC5=`01` selects write mode 1, which uses the latch rather than AL; all four enabled plane bytes are projected to the page index. |

The last step is a **source-derived projection**. There is no native
post-`MOVSB` physical-page capture for this call. The bridge therefore does
not claim native byte, plane, RGB, or full-frame parity. The next separate
gate is a PM-approved post-Move physical-page stop and comparison.

## Reproduction

Build the standalone bridge with strict C11 warnings and ASan/UBSan against
the existing EGA decoder. With no arguments it runs synthetic four-plane
copy and typed rejection tests. `--record` accepts only the exact private
mode/address tuple; it prints indices and a scope statement, not plane bytes.
The private record must be hash-checked before use. The legacy bridge and
its 16-bit/planar fixture are not reinterpreted by this model.

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude \
  tools/replay/res464_16a1_uvga_exact_bridge.c src/formats/ega.c \
  -o resources/res464-16a1-uvga-exact-asan
resources/res464-16a1-uvga-exact-asan
sha256sum resources/oracle-runs/slot-a-res464-first-page-store-p5-015/p5-first-state.txt
resources/res464-16a1-uvga-exact-asan --record \
  resources/oracle-runs/slot-a-res464-first-page-store-p5-015/p5-first-state.txt
```
