# Second-key resident `+006B` return and caller successor

This bounded model starts at the naturally reached `RES_08AE:+006B` far
return, resumes the immediate caller at `+008D`, and stops before the
conditional gate at `+00AA`. It does not model the intervening callee body
that produced the incoming `+006B` state, the `+00AA` branch, or a complete
second-key route. The required
`deja "Poolrad second key RES_006B +008D caller successor +00AA native C parity"`
query found no indexed prior session. The merged `+001F..+0033` caller
composition and the separate, reviewed local `+006B` dependency note at
`9983f093` supplied the earlier return-frame context. The separate
reviewed `+00AA` gate at `2ca13ac` begins at this model's stopping point.

## Reached bytes and state

The controlled DOS v1.3 replay used START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and the 13,137-byte owner slot-A save SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Private captures remain ignored in
`resources/oracle-runs/slot-a-resident-137c-second-004/`.

Loaded `10D2:006B` is `CB` (`RETF`). The loaded `+008D..+00A9`
29-byte span has SHA-256
`0da185379137239136d28227fef8d44da782d13b0326fed0030f153d03da98ad`.
It matches START.EXE file `0x8B84..0x8BA0` byte for byte.

CPU words below use debugger order
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`:

| Same-run stop | CPU words | Full RAM SHA-256 |
| --- | --- | --- |
| before `RETF`, `10D2:006B` | `10D2 006B 14A0 1D16 1D16 3DA8 3DB4 8D94 0005 8D7C 0005 0001 0000 7206` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| after `RETF`, `10D2:008D` | `10D2 008D 14A0 1D16 1D16 3DAC 3DB4 8D94 0005 8D7C 0005 0001 0000 7206` | no direct RAM capture |
| before `+00AA` gate, `10D2:00AA` | `10D2 00AA 14A0 1D16 1D16 3DAC 3DB4 000A 0000 000A 0000 0001 0000 7246` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |

The `+006B` RAM has far return words `SS:3DA8=008D` and
`SS:3DAA=10D2`. The return advances SP by four and leaves the other
CPU words unchanged. At `+008D`, two MOVs store `AX=8D94` and
`DX=0005` into `SS:[BP-8]=SS:3DAC` and `SS:[BP-6]=SS:3DAE`.
Those words already have the same values at the incoming stop.
The caller then reads `SS:[BP+6]=000A`, zeroes DX, and copies the count
to CX. It subtracts the saved return pair `0005:8D94` from the prior
pair `0005:8D9E`, yielding `DX:AX=0000:000A`. The final
`CMP DX,BX` sees zero against zero and leaves FLAGS=`7246`.

## Bounded C comparison

`pr_rec_res_08ae_006b_return` models the one-byte far return using the
saved frame. `pr_rec_res_08ae_008d_to_00aa` models the two local stores,
load, XOR, copies, 32-bit subtract, and high-word compare. It stops at
`+00AA`; it does not select the following branch. A focused CTest checks
the reached CPU state and uses deliberately different synthetic local
values to verify that both MOV stores execute. The same test, run with
the ignored native `+006B` and `+00AA` RAM files, reports **0/655,360
RAM-byte mismatches** and exact **14/14 CPU words** at `+00AA`.
Both entry and exit native RAM images are byte identical; the synthetic
case is what distinguishes the modeled same-value stores from skipped
stores. The model compiled with `-Wall -Wextra -Wpedantic -Werror`, and
the registered focused CTest passed. There is no direct full-RAM
comparison at the intermediate `+008D` stop.

## Parity review

- **target_function:** natural `RES_08AE:+006B` return and reached caller
  `+008D..+00A9`, stopping before `+00AA`.
- **source_candidates:** the two new bounded C entry points above.
- **signals:** original and loaded byte identity, saved far-return frame,
  direct same-run CPU stops, two hash-locked full RAM checkpoints, exact
  native-seeded C CPU/RAM exit, and a synthetic store check.
- **score and verdict:** 4/5 corroborating signal groups, **match** for
  this reached instruction slice. No independent replay or direct
  `+008D` RAM capture was available.
- **review_notes:** the earlier `+003F..+006A` callee span remains an
  incoming native dependency. The later `+00AA` branch is separately
  bounded and not composed here. This earns zero strict full-function
  or playable-game credit.
