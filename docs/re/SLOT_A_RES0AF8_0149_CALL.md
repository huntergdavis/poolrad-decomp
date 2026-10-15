# Loaded RES_0AF8:0149 second near-call site

Loaded `131C:0149` is the next instruction after the first near CALL
site at `131C:0140..0148`. It is a separate setup and CALL, not the
callee entry. Reaching it requires a return from the first call, which
has not been observed. This source-only audit bounds nine bytes
through the CALL at `131C:014F` and stops before its callee.

`deja "Poolrad RES_0AF8 131C:0149 callee return boundary 131C:0615 XOR first write source"`
found no indexed session. PM corrected the call-site classification
in inbox i6025; p5 and pF cleared these addresses. The original
image address is `0AF8:0149`, physical `0xB0C9`; START file source
is `0x0ADBF`; loaded physical address is `0x13309`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START call site | `0x0ADBF..0x0ADC7`, 9 bytes | `c1360ea94ea3d257f20ac327fb5a724ac16bae7fa39e0a54ffa51d0ef0c65d94` |
| Loaded call site | `0x13309..0x13311`, 9 bytes | `c1360ea94ea3d257f20ac327fb5a724ac16bae7fa39e0a54ffa51d0ef0c65d94` |
| Near CALL | START `0x0ADC5..0x0ADC7`, loaded `0x1330F..0x13311`, 3 bytes | `f6a163a014846f6c10b6715675a78a06faeae2c012f9c87f90f2cf67a1e51e56` |
| Target prefix | START `0x0B237..0x0B23E`, loaded `0x13781..0x13788`, 8 bytes | `de4d9b12622b144b6dbcf41df6bf0af33507fcae98d15161a9ee6c2bdcb0ee1f` |

The nine-byte source and loaded spans match exactly at one START
position. They decode as `MOV AX,0x8608`, `PUSH DS`, `PUSH AX`,
`PUSH CS`, and a near CALL to `131C:05C1` (original `0AF8:05C1`).
Given a hypothetical reached call and readable stack, its callee
entry would have return IP `0152` on top, then the pushed CS,
AX=`8608`, and DS words. The three PUSH instructions and CALL's
return-IP push decrease SP by eight.
The first eight target bytes also match the pinned START source,
but this audit does not enter that callee or supply its effects.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0149_call_audit.py`.
No first-call return, second-call reach, native execution, C parity,
or full-C count is claimed. Original media and RAM stay ignored
under `resources/`.
