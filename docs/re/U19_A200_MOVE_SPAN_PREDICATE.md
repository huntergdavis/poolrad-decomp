# U19 witness versus a later mode-1 Move row span

This source-only predicate asks whether a **supplied, complete**
`RES_0464:+1B52` video-state-1 row table could place the destination of
runtime `Move` over guest `A000:118F`. The saved slot-A row table excludes
that address. A separate synthetic row-104 table shows the minimum extent
needed to include it. Neither table is a reached U19 `+1B52` frame.
An intersection is a conditional guest-address result, not a reached CPU
store, host backing assignment, changed EGA plane, first writer, or RGB
frame.

The earlier bounded `deja` attempt for the U19/RES0464 work could not
obtain the local index lock. PM declined a retry under host load; no
indexed finding is reused. The checked-in [saved transfer](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md),
[original display route](SLOT_A_U40C_DISPLAY_TRANSFER.md), and
[prior U19 exclusion](U19_A200_MODE1_LATCH_BOUNDARY.md) supply the separate
run boundaries. This fixture checks their row arithmetic without replaying
native code.

## Original source pins

Ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The original `RES_0464:+1B52..+1E5B` file span `[0x61CB,0x64D5)` is
778 bytes, SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
Its mode-1 register setup is file `[0x63AA,0x63CA)`, 32 bytes,
SHA-256 `79fe951cb779d3f656e78693b928c652483f415b8c8272792f7b2eb4b74d7851`.
Runtime `Move` at `0AF8:+1685..+16A8` is file `[0xC2FB,0xC31E)`,
35 bytes, SHA-256
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.
The original mode-1 path stages CPU bytes into `A200`, sets Sequencer mask
`0F` and Graphics Controller write mode 1, then calls `Move` at `+1DA0`;
its forward `REP MOVSB` is `+16A1`. Those source instructions alone do not
measure the live controller or host handler at that instruction.

## Fail-closed guest span predicate

The [executable fixture](../../tools/replay/u19_a200_move_span_predicate.py)
requires all 168 dirty flags, a supported extent and low offset for every
dirty row, `video_state=1`, source segment `A200`, destination segment
`A000`, and page base. For row `r`, the original row-table addresses are
`DS:[4A28+r]` (dirty), `DS:[4C20+2r]` (first),
`DS:[4D70+2r]` (last), `DS:[4AD0+2r]` (low), and `DS:[5060]`
(page base). Its destination span is:

```text
P = page_base + 1 + 40*r + low
L = ((last - first) >> 2) + 1
intersection iff dirty and P <= 0x118F < P + L
```

The predicate rejects incomplete rows, unknown live metadata, unsupported
interleaved extents, and 16-bit span wrap as `UNKNOWN_METADATA`. A complete
nonintersecting table yields `NOT_MATCH` **for this direct mode-1 Move
span**. An intersecting table yields `CONDITIONAL_SPAN_INTERSECTION`; it
still needs the same-run `+1B52` caller, live `+1DA0` arguments and a
reached `+16A1` source/destination pair before even a CPU-store claim.
Other drawing routes remain outside this predicate.

| Supplied geometry | Result | Exact boundary |
| --- | --- | --- |
| Checked-in saved slot-A pattern: dirty rows 16..103, `L=11` | `NOT_MATCH` | Last exclusive page span `[0x115B,0x1166)` is `0x29` before target. |
| Synthetic row 104, same `page_base=0140`, `low=2`, `L=11` | `NOT_MATCH` | Row span `[0x1183,0x118E)` ends one byte before target. |
| Synthetic row 104, same base and low, `L=13` | `CONDITIONAL_SPAN_INTERSECTION` | Span `[0x1183,0x1190)` contains target at index 12. |
| Missing live row metadata | `UNKNOWN_METADATA` | No inferred U19 row, page, or write. |

The two row-104 cases are intentionally synthetic. Extending the saved
row stride does not assert that row 104 was dirty, that its page base and
extent matched, or that U19 reached this flush. The saved second flush is
also a negative effect control: it executed row copies yet changed zero
sampled EGA plane bytes in the complete captured dirty-row union.

The separate [first-byte projection](RES464_16A1_EXPLICIT_BYTE_PROJECTION.md)
requires live GC6, Seq4, map mask, handler, bank, memory mask, source
planes, prior page planes and latch state before a value-change model can
run. Its saved first-byte route stops unsupported on missing inputs. The
predicate here does not supply them or overlap the selected U19 handler,
host display/page, or event-clock lanes. Original media, RAM, EGA planes,
and screenshots remain under ignored `resources/`.

Run the small source-only fixture with:

```sh
python3 -B tools/replay/u19_a200_move_span_predicate.py
```

A future complete 168-row JSON frame can be supplied with `--frame PATH`;
unknown or malformed input prints `UNKNOWN_METADATA` and exits nonzero.
No native, compiler, first actual EGA writer, page/RGB, or game-identity
credit follows from this geometry check.
