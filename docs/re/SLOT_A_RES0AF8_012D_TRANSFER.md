# Loaded RES_0AF8:012D stack-built far-transfer boundary

The nonnull far-pointer fallthrough from `131C:011B` starts at loaded
`131C:012D`, physical `0x132ED`. It maps to original
`0AF8:012D`, physical `0xB0AD`, and START file `0x0ADA3`. This note
bounds 19 bytes through RETF at `131C:013F`. The RETF is a
stack-built transfer to ES:BX, not evidence of a return to the caller.

`deja "Poolrad 131C:0103 ES:0010 131C:012D far transfer"` found
no indexed session. PM assigned this source-only arm in inbox i5775.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START transfer span | `0x0ADA3..0x0ADB5`, 19 bytes | `f40fac73cf15f6a62c43395ce93c455dfda956e2d427db8477461658d9a01bb1` |
| Loaded transfer span | `0x132ED..0x132FF`, 19 bytes | `f40fac73cf15f6a62c43395ce93c455dfda956e2d427db8477461658d9a01bb1` |
| Sequential next instruction | START `0x0ADB6..0x0ADB8`, loaded `0x13300..0x13302`, 3 bytes | `05363f65c03f3827871b25a34a47bd5ddfc501c83e7513555d321949fb2779dc` |

The source and loaded spans match exactly, have one START occurrence,
and decode as 10 instructions. `XOR AX,AX` zeroes AX; three word stores
would clear `DS:0x4418`, `DS:0x441A`, and `DS:0x4426`. If this arm
follows the `00E2` prefix, DS is `14A0` and the stores start at
physical `0x18E18`, `0x18E1A`, and `0x18E26`. This conditional address
calculation is not a live write observation.

The sequence then sets AX to `0x0123` and pushes CS, AX, ES, BX in
that order. RETF pops BX as IP and ES as CS, so the transfer target
is the prior ES:BX far pointer. It leaves the pushed `0x0123` and CS
words on the stack; the four pushes followed by RETF change SP by
minus four. A future model needs actual ES, BX, SS:SP and stack RAM,
as well as the target effect. The sequential next instruction at
`131C:0140` is an address boundary, not a return site from this arm.

Reproduce with
`python3 tools/replay/slot_a_res0af8_012d_transfer_audit.py`.
No reached clear, target value, far transfer, C parity, or full-C
count is claimed. Original media and RAM stay ignored under
`resources/`.
