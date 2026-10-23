# Resident `0709:+0DBE` calibrated-delay wrapper

This recovers the complete resident far routine `0709:+0DBE..+0DD3`, from
`PUSH BP` through `RETF`, with the A40 timer as an explicit callback. The C
model includes the guest stack writes, register effects, far-call frame,
callee `RETF 2` cleanup, and wrapper `POP BP; RETF` cleanup. It does not model
the A40 elapsed time or establish a natural native U1C DELAY execution.

Before implementation, bounded `deja "Poolrad RES_0709 0DBE delay wrapper
A40 029C full RAM CPU parity U1C saved game native DELAY"` timed out after
20 seconds. The merged [U1C DELAY note](U1C_DELAY_271F.md) supplied the
candidate call chain, and the merged [A40 note](RES_A40_DELAY_029C.md)
supplied its separately bounded callee. Both spans and loaded bytes were
checked independently for this wrapper. The supplied-state
[APPROACH note](U1C_APPROACH_0817.md) supplied the `0x9C27 → 0x9C28`
encounter-script lead; it is not a native reachability result.

## Original and loaded bound

Ignored DOS v1.3 `START.EXE`: 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its hash-locked expanded image `[0x7E4E,0x7E63)` is **21 bytes and 10
instructions**, SHA-256
`a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d`.
The loaded private RAM copy at `0F2D:+0DBE..+0DD3` has SHA-256
`7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be`.
The RAM dump is 655,360 bytes, SHA-256
`107f1f0b4dbcecffc8fed3744b4d7f55b26e65186a2628401d586e354a19ab32`.
The changed loaded words are the relocated far-call segment; the loaded call
at `+0DCC` targets `1264:029C`. `+0DD1` is the far-call return IP, `+0DD2`
is the final `RETF`, and `+0DD3` starts the next routine.

| Offset | Checked effect |
| --- | --- |
| `+0DBE..+0DC1` | Save BP and make the two-byte frame. |
| `+0DC1..+0DC6` | Read DS byte `49E2` into AL and clear AH. |
| `+0DC6..+0DCB` | Multiply AX by 225 via DX, leaving the low word in AX and zero in DX for every byte input. CF/OF are zero. |
| `+0DCB..+0DD1` | Push AX; far-call loaded A40 `1264:029C`. At callee entry SS:SP holds return IP `0DD1`, return CS `0F2D`, then the word argument. |
| `+0DD1..+0DD3` | After A40 `RETF 2` consumes the argument, restore BP and far-return to the original caller. |

The caller is U1C `+271F..+272D` (GAME.OVR file `[0x3465,0x3473)`,
SHA-256 `aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1`).
It advances ECL PC before its far call to the wrapper. The wrapper's factor
is in `[0,255]`; its maximum product is 57,375, so the low-word result loses
no high bits. The saved slot-A numeric factor is 4, making the argument 900
if this handler is reached.

## C model and original-byte differential

`pr_r0709_delay_wrapper_0dbe` uses a 14-field guest CPU state and exactly
one MiB of guest RAM for 20-bit real-mode addresses. A callback sees the
fully pushed A40 frame. A missing callback stops at that reached call with
the frame and previous register effects intact. A successful callback must
restore SP to its entry value; the wrapper then performs both returns.
Callback failure or malformed callee SP preserves the partial state and
returns an explicit status.

The focused C test uses synthetic factors 0, 4, and 255, stack wrap,
callback-visible word order, guest register mutation, complete return
cleanup, and missing/failing/malformed callback boundaries.
The separate optional Python differential executes the **hash-verified
loaded original wrapper instructions** under Unicorn 2.1.4. A newly written
synthetic A40 callee sets all flags and returns with `RETF 2`; the C callback
has the same external effect. Across 16 factor/stack/initial-flags cases,
the callee-entry frame, all **14 returned CPU fields**, and the complete
**1 MiB RAM image** match. MUL's undefined flags at the callee boundary are
excluded from that entry comparison; the synthetic callee defines them
before the returned full-CPU comparison. This proves the wrapper with a
controlled A40 boundary, not native timer behavior or elapsed-time parity.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_delay_wrapper_0dbe.py
```

## Bounded natural-route check

A fresh ignored DOSBox-X heavy-debug run used the same START/GAME hashes
and slot-A save (13,137 bytes, SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`).
Before slot-A load, breakpoints were armed at loaded U1C `3582:271F`, this
wrapper `0F2D:0DBE`, and A40 `1264:029C`; `BPLIST` confirmed all three.
The `2116:0566` menu checkpoint was removed before loading slot A. The
native screen started at 15,4 N 10:50. The bounded input sequence was one
Search, one Up, and one Search. The settled frame after the Up was 14,4 N
11:00; the final settled frame remained 14,4 N 11:00. **No breakpoint stop
was observed at any of the three entries** during this sequence. Heavy-debug
screenshots, save, and emulator state stay under ignored `resources/`.
The first Search's time effect was not separately measured at a settled
prompt, so no per-key clock attribution is made.

This is a nonhit for that bounded sequence, not a general nonreach proof.
The next targeted native route is an actual encounter/script branch that
reaches APPROACH at ECL PC `0x9C27` and then DELAY at `0x9C28`; the existing
supplied-state probe can reach it with chosen RNG/menu inputs, but native
RNG, menu choice, and script reachability remain to be measured. Arm U1C
`3582:271F`, resident `0F2D:0DBE`, and A40 `1264:029C`, then capture entry
and return frames only on a naturally reached branch.

No full-C count or saved-game DELAY parity is claimed before PM review.

## Pinned MULW flags: model fixed (2026-10-23, census #1464 family 3)

Pinned DOSBox-X `MULW` (`instructions.h:605-619`) keeps SF and AF (`FillFlagsNoCFOF`), sets ZF from AX **alone**, PF to the even parity of **all four bytes of DX:AX** (`FLAG_PF` is the constant mask 4), and CF = OF = (DX != 0). This plain model modelled neither XOR AH,AH's flags nor MUL's ZF/PF/SF; it only cleared CF/OF. Fix: XOR AH,AH sets ZF and PF and clears the rest, and the MUL follows pinned MULW (here AL*E1h <= DB1Fh, so DX = 0 and CF = OF = 0).

- **Did the evidence depend on these flags?** No. The credit rests on the `_cpu` audit, and this plain model's C test passes with asserts on.
- **Caller-observable?** No. The `MUL` is followed immediately by `LCALL 0A40:029C`, whose `OR DX,DX` at `+02A2` overwrites ZF, PF, SF and AF before any instruction reads them (walked over the original bytes). At every real return these flags are already gone.
- **Evidence for the verdict (par-e decides):** we expect the credit to stand.
