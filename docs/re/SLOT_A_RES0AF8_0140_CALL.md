# Loaded RES_0AF8:0140 first near-call boundary

The zero-pointer target of the [merged `0123` decision](SLOT_A_RES0AF8_0123_POINTER_CONSUMER.md)
starts at loaded `131C:0140`, physical `0x13300`. The other arm reaches
`131C:012D`, whose far return does not fall through here. Its original image
address is `0AF8:0140`, physical `0xB0C0`, and its START file source
is `0x0ADB6`. This note bounds nine bytes through the first near CALL
at loaded `131C:0146`; it stops before the callee.

`deja "Poolrad 131C:010C ES:0014 131C:0140 near CALL"` and the
follow-on `deja "Poolrad 131C:0140 instruction boundary 131C:060E next function source"`
found no indexed session. PM assigned this source-only span in inbox
i5788 and renewed it in i5961.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START call-site span | `0x0ADB6..0x0ADBE`, 9 bytes | `a41aff4f3ba30e8e833b6c51abd3543579617c9e091633e4ca341addeada47d1` |
| Loaded call-site span | `0x13300..0x13308`, 9 bytes | `a41aff4f3ba30e8e833b6c51abd3543579617c9e091633e4ca341addeada47d1` |
| Near CALL | START `0x0ADBC..0x0ADBE`, loaded `0x13306..0x13308`, 3 bytes | `4dccbdd62f52ff0c0b8d4012eba527eb3ece3258acc7697486231b8031e03498` |
| Target prefix | START `0x0B237..0x0B23E`, loaded `0x13781..0x13788`, 8 bytes | `de4d9b12622b144b6dbcf41df6bf0af33507fcae98d15161a9ee6c2bdcb0ee1f` |

The nine-byte source and loaded spans match exactly at one START
position. They decode as `MOV AX,0x8508`, `PUSH DS`, `PUSH AX`,
`PUSH CS`, and a near CALL to `131C:05C1` (original `0AF8:05C1`).
At a hypothetical callee entry, the actual `SS:SP` stack would hold
return IP `0149` on top, then the pushed CS, AX=`8508`, and DS words.
The near CALL itself pushes only the return IP; these four words
decrease SP by eight. The target's first eight loaded bytes match
START file `0x0B237`, but this audit does not enter it or supply
its effects. Original image and loaded addresses differ by `0x08240`;
original image and START file differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0140_call_audit.py`.
No callee execution, return, native reach, C parity, or full-C count
is claimed. Original media and RAM stay ignored under `resources/`.
