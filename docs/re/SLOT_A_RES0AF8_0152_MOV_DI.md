# Loaded RES_0AF8:0152 first instruction after a hypothetical return

Loaded `131C:0152` is the return IP pushed by the second near CALL
at `131C:014F`. This source-only audit bounds just the three-byte
instruction there. It does not establish that the call returns or
that this instruction executes. The next instruction starts at
`131C:0155`.

`deja "Poolrad RES_0AF8 loaded 131C:0152 next caller instruction 131C:0619 MOV AX 3D00 control boundary"`
found no indexed session. PM assigned this follow-on in inbox
i6083; p5, pF, and pC were screened for ownership before edits.
Original `0AF8:0152` is physical `0xB0D2`; START file source
is `0x0ADC8`; loaded physical is `0x13312`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START instruction | `0x0ADC8..0x0ADCA`, 3 bytes | `b3f5bef0ba1235ee7a8c96ffe9c6707b94165ee875eef6699514f29ac2789539` |
| Loaded instruction | `0x13312..0x13314`, 3 bytes | `b3f5bef0ba1235ee7a8c96ffe9c6707b94165ee875eef6699514f29ac2789539` |
| Prior CALL plus instruction anchor | START `0x0ADBF..0x0ADCA`, loaded `0x13309..0x13314`, 12 bytes | `931086f6f0c8ddef51ef0100a3351230e14311f95f8f75e7265a4a38934e22d0` |

The three-byte instruction matches loaded RAM and appears twice
in START, at `0x0ACB7` and `0x0ADC8`. The 12-byte contiguous
span joining the pinned preceding call site to this instruction
occurs once and identifies this location. The instruction decodes
to `MOV DI,0x8708`. If reached, it sets DI to `0x8708` and
leaves FLAGS unchanged. It does not itself read or write memory.
The prior call's return, stack cleanup, and register effects are
unmeasured. Original and loaded physical addresses differ by
`0x08240`; original and START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0152_mov_di_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
