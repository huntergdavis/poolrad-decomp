# First-page `0AF8:+16A1`: explicit-input one-byte EGA projection

The [standalone C fixture](../../tools/replay/res464_16a1_explicit_byte_fixture.c)
models **one** source read and destination write at the first conditional
`REP MOVSB` iteration. It requires every graphics register, handler, bank,
memory-mask, and four-plane source/page input used by this small model.
Missing inputs cause an unsupported stop without changing the output.
Its test bytes are synthetic; none is a claimed saved-route register,
VRAM byte, pixel, or palette value.

## Original boundary

The [read-only audit](../../tools/replay/res464_16a1_explicit_byte_audit.py)
pins ignored DOS v1.3 `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
first-flush RAM SHA-256
`25ae4cdbceb249e04ec413b49e6cbfc6f14ff4210378bbdd34e19d37389266d1`,
and entry GC record SHA-256
`4a474c34200525498a5a223630e9a858db3902bb713d4191c70b7214d5bd6ce5`.
It also pins the flush `[0x61CB,0x64D5)` SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`,
register setup `[0x63AA,0x63CA)` SHA-256
`79fe951cb779d3f656e78693b928c652483f415b8c8272792f7b2eb4b74d7851`,
and runtime Move `[0xC2FB,0xC31E)` SHA-256
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.
Nine selected original operands are checked, including `+1698 JAE` and
`+16A1 REP MOVSB`. First dirty row 16 derives host source
`A200:03C3`, host destination `A000:03C3`, and a conditional 11-byte
Move count. This fixture models only the **first** byte.

The earlier static contract `f27862b` proved that source-local code writes
Sequencer mask `0F` and GC5 value `01`, but it did not measure their values
at the native `+16A1` stop. The entry record does not include GC6 or Seq4.
Handler selection, banks, memory mask, source planes, and page planes are
also unmeasured there. The fixture accordingly requires explicit values
for GC6, Seq4, Seq2, GC4, GC5, handler/odd-even flags, banks, memory mask,
all four source-plane bytes, and all four old page-plane bytes. It accepts
only the narrow unchained, bank-zero, `memmask=FFFF`, write-mode-1,
read-mode-0 cases that the local model implements.

## Four-plane address and mask table

For accepted `GC6=04`, the source planar byte offset is `23C3` and the page
planar byte offset is `03C3`. For accepted `GC6=06`, GC6 bit 1 evenizes
those offsets to `23C2` and `03C2`. Those offsets label planar byte
addresses within the separate source/page regions; the plane number is a
separate selector. The odd **host** offset remains `03C3` in both cases.

| Plane | Map bit | GC6=04 source/page | GC6=06 source/page | Seq2=0F, Seq4=04 | Seq2=05, Seq4=04 | Seq2=0F, Seq4=00 at odd host address |
| ---: | ---: | --- | --- | --- | --- | --- |
| 0 | `01` | `23C3 / 03C3` | `23C2 / 03C2` | write source latch | write source latch | keep old page byte |
| 1 | `02` | `23C3 / 03C3` | `23C2 / 03C2` | write source latch | keep old page byte | write source latch |
| 2 | `04` | `23C3 / 03C3` | `23C2 / 03C2` | write source latch | write source latch | keep old page byte |
| 3 | `08` | `23C3 / 03C3` | `23C2 / 03C2` | write source latch | keep old page byte | write source latch |

With `Seq4=00`, odd host address bit 0 selects read plane 1 when `GC4=0`
and masks the write to planes 1 and 3. The effective mask comes from
`Seq2` intersected with that odd/even mask. Write mode 1 copies each
source-read latch into its enabled page plane. The fixture reports each
plane's selector, map bit, source/page planar offsets, source byte, old
page byte, enabled flag, and resulting byte.

The merged portable [`pr_ega_planar_byte`](../../src/formats/ega.c)
then projects the four resulting page bytes to eight **palette indices**.
Pixel positions 0..7 use bit masks `80,40,20,10,08,04,02,01`; plane 0
contributes index bit 0, plane 3 index bit 3. This does not choose a
palette or RGB value. The five synthetic tests use source planes
`80/40/20/10` and explicit old page planes to check full mask, GC6
evenization, odd/even plane selection, and partial mask cases. They check
all four per-plane table rows and all eight resulting indices. Ten
individual missing-input flags and two unsupported hardware modes stop
without changing output.

**Boundary:** the saved route is unsupported **before its first `+16A1`
read** because the native hardware and plane inputs above are missing.
For a fully supplied synthetic case, this fixture stops after that one
modeled write, before the next `+16A1` iteration and before the eventual
`+16A3` DS restore / `+16A5` far return. It does not duplicate or claim
native parity for the separate 11-byte synthetic composition `8e280f6`.

The bounded `deja` query for row 32 and one-byte GC6/Seq4 projection returned
no matches. This fixture reuses local address selection `fa217de`, static
input contract `f27862b`, and the merged EGA byte decoder. No emulator,
GitHub action, native EGA/page comparison, RGB, or full-frame claim is involved.

```sh
python3 -B tools/replay/res464_16a1_explicit_byte_audit.py
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude \
  tools/replay/res464_16a1_explicit_byte_fixture.c src/formats/ega.c \
  -o resources/res464-16a1-explicit-byte-fixture
resources/res464-16a1-explicit-byte-fixture
```
