# Resident `A40:029C` calibrated delay boundary

This recovers the complete 45-byte far delay routine and its near spin helper,
`A40:+029C..+02C9`, as a bounded C state model. It is the static callee of
[`U1C DELAY`](U1C_DELAY_271F.md) through resident `0709:+0DBE`. Its observable
inputs are a BIOS data byte and a calibration word. Wall-clock duration and
native execution of the U1C opcode on the saved one-Up route remain unmeasured.

Before implementation, `deja "Poolrad U1C_DELAY_271F RES_0709 0DBE timer
callback saved slot A native parity"` timed out after 20 seconds. The prior
U1C delay note above supplied the candidate chain and save-derived factor;
the source spans, loaded addresses, and loop target were checked again here.

## Source and loaded proof

The inputs are ignored DOS v1.3 `START.EXE` (65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`)
and `GAME.OVR` (206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`).
The START image is expanded by the hash-locked loader map. The separate
private RAM dump is 655,360 bytes, SHA-256
`107f1f0b4dbcecffc8fed3744b4d7f55b26e65186a2628401d586e354a19ab32`.

| Executable span | Original location and SHA-256 | Loaded location and SHA-256 |
| --- | --- | --- |
| U1C `+271F..+272D`, 14 bytes, 6 instructions | GAME file `[0x3465,0x3473)`, `aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1` | `3582:+271F`, `ecb6e7a981de47539c4809d0338832f0c107bf96a9e83d65e2847bf5a2f14ec2` |
| RES_0709 `+0DBE..+0DD3`, 21 bytes, 10 instructions | expanded START `[0x7E4E,0x7E63)`, `a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d` | `0F2D:+0DBE`, `7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be` |
| A40 far body `+029C..+02BD`, 33 bytes, 13 instructions | expanded START `[0xA69C,0xA6BD)`, `7ee3c156a50e06d1f23e4da738158d3cb48179223d3135c55b7976eae426e75e` | `1264:+029C`, same hash |
| A40 near helper `+02BD..+02C9`, 12 bytes, 6 instructions | expanded START `[0xA6BD,0xA6C9)`, `d2df9bf05a361b9f27a4fe2591ea9938c31fb9d319ea1640d73bfbc52d93a2ea` | `1264:+02BD`, same hash |

The combined loaded A40 span at physical `0x128DC..0x12909` hashes
`d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb`.
U1C `+2726` far-calls resident `0709:+0DBE`; that resident reads DS:`49E2`,
multiplies the byte by 225, pushes the low word and far-calls A40 `+029C`.
The factor is at most 255, so the full product fits in 16 bits. A40 returns
with `RETF 2` and consumes that word argument.

`A40:+029C` reads the word argument from its far-call frame. Zero returns
without an external read. For a nonzero argument, it sets ES to `0x0040` and
DI to zero, reads one baseline byte from `ES:[DI]` at `+02AD`, and **does not
reload that baseline** on the DX loop back-edge. Each unit reloads CX from
DS:`84FC` at `+02B0`, calls near `+02BD`, then decrements DX. The helper loads
BX=4, performs four decrements, compares the original baseline with the
current BIOS byte, and `LOOPE` repeats while the bytes match and decremented
CX is nonzero. Initial CX=0 wraps, permitting 65,536 polls in one unit. The
original body has one far return and one near return, no game-memory store.

## C boundary and tests

`pr_a40_delay_029c_run` requests the BIOS byte and DS calibration word through
typed callbacks only at their reached instructions. The callback order is one
baseline BIOS read, then for each unit one calibration read and one or more
spin BIOS reads. A caller-supplied operation budget stops before a further
read while preserving the completed counts. The U1C adapter fits the existing
`delay_a40_029c` hook; a missing or failing callback remains an explicit
external status. Neither interface estimates elapsed time or drives a real
timer.

The synthetic test covers zero units, the one-time baseline across two units,
calibration reload, changed BIOS byte, CX=0 wrap, read ordering, exhausted
budget, absent callbacks, callback failure, and U1C PC-advance integration.
The private RAM snapshot from a separate PRINTCLEAR stop has DS=`14A0`,
DS:`49E2`=4, DS:`84FC`=269, BIOS `0040:0000`=248. The factor would pass 900
to A40 *if U1C DELAY were reached*. These are numeric snapshot values only,
not a delay-entry trace or a duration measurement.

## Native one-Up reachability check

In a fresh ignored heavy-debug DOSBox-X run using the same hash-pinned slot-A
save (13,137 bytes, SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`),
`BP 1264:029C` was armed before loading slot A. The title transition stopped
at the separate menu breakpoint `2116:0566`; after clearing that breakpoint,
the private run loaded slot A and showed `15,4 N 10:50`. Exactly one Up reached
the completed `14,4 N 10:51` frame without an observed A40 breakpoint stop.
This is an observed nonhit in one run, not a universal unreachability proof.
The private screenshots, save, emulator state, and any binary slices remain
under ignored `resources/`.

Reproduce the hash and numeric audit with:

```sh
python3 tools/replay/slot_a_u1c_delay_a40_audit.py \
  resources/DOS/POOLRAD/START.EXE \
  resources/DOS/POOLRAD/GAME.OVR \
  resources/oracle-runs/slot-a-u1c-print-rect-p9-096/preprint-ram-private.bin \
  resources/oracle-runs/slot-a-u1c-delay-a40-p9-097/POOLRAD/SAVGAMA.DAT
```

The model establishes bounded instruction semantics for this pair. It does
not establish elapsed-time parity, native U1C DELAY reachability on one Up,
or a full playable-C count.
