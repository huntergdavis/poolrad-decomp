# Unit 0x9b +0x01AF: record counter increments (the call `0xAC:0x25`)

Unit `0x9b` has memory segment `0xAC` (overlay inventory), so the far call `0xAC:0x25` made at the
end of the party insertion (unit `0x6f` `+0x1574`) is the **overlay thunk** for this unit's function
at `+0x01AF`; it is not a resident routine. Span: `+0x01AF..+0x0254` (the `retf 4` at `+0x0252`),
file `175,158..175,323`, 166 bytes, 49 instructions, no calls. SHA-256
`5dd90fb8796e7aa15a770e4868aaceeee0a0886e0251a9fc7c10f3f32935652c` (GAME.OVR, 206,105 bytes,
`872964a3…7928614`; unit file offset 174,727). `deja` found no earlier note.

**Classification: full translation** of all 49 instructions (no calls; the only state is the record
view). No count credit, identity or DOS/native parity is claimed here; accounting is the PM's after review.

## Behavior

One far-pointer argument (offset `[bp+6]`, segment `[bp+8]`, `retf 4`) addresses a character record. With
`t` the byte at `+0x12`, five steps run in order, each an unsigned byte compare against `t` and, when taken, an
unsigned compare of the counter with zero and a byte `inc` (mod 256). The counter is re-read before each step, so a
step that wraps it to 0 stops the next one:

| Steps | Condition | Effect |
| --- | --- | --- |
| `+0x01C1..+0x01D6`, `+0x01DE..+0x01F3` | `t > 0x0C`, then `t > 0x0D`; counter `+0xB2` nonzero | `+0xB2 += 1` each |
| `+0x01FB..+0x0210`, `+0x0218..+0x022D` | `t > 0x0E`, then `t > 0x0F`; counter `+0xB3` nonzero | `+0xB3 += 1` each |
| `+0x0235..+0x024A` | `t > 0x10`; counter `+0xB4` nonzero | `+0xB4 += 1` |

No other record byte is read or written. Every access is `es:[di + k]` (`di` the far offset), so the
effective offset wraps modulo 65,536. The record is explicit host state (a view of its ES segment plus the
far offset); an access outside the view is a host guard, stopping with the increments already done kept.

`pr_u9b_record_incr` (`overlay_u9b_record_incr_01af.{h,c}`; test
`test_overlay_u9b_record_incr_01af.c`): all 256 values of `t` against eight boundary values for each of
the three counters (a wrap-point restatement instead of stepwise increments), the mid-pair wrap
(`0xFF -> 0` skips the second step), unsigned `t = 0xFF`, the 16-bit offset wrap in a 64 KiB view,
untouched bytes, and the host-guard paths with retained partial effects.

## Slot-A party use (private, aggregate only)

`pr_u6f_party_build` now applies this routine to each member after the append, as the insertion does. For
the six slot-A records, the steps perform **4 increments in total**; none of them is one of the five
PARTYSTRENGTH offsets (`+0x96`, `+0x9B`, `+0x110`, `+0x111`, `+0x11B`), so the party strength remains
**10**. The private script `tests/replay/check_slot_a_party_input.sh` output is unchanged.
