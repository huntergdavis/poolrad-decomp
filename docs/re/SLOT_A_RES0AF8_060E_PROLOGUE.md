# Loaded RES_0AF8:060E next prologue boundary

The [merged `060B` tail](SLOT_A_RES0AF8_060B_STACK_RETURN.md)
ends in a near `RET` at loaded `131C:060D`. Its next sequential
instruction at `131C:060E` starts a distinct prologue; the return
does not fall through to it. This source-only audit stops after the
three instructions at `060E..0614`, before the `XOR CX,CX` at
`0615` and before the first data write at `0617`.

`deja "Poolrad 131C:0140 instruction boundary 131C:060E next function source"`
found no indexed session. PM assigned this boundary in inbox i5961.
The original image address is `0AF8:060E`, physical `0xB58E`;
the START file source is `0x0B284`; the loaded physical address
is `0x137CE`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START prologue | `0x0B284..0x0B28A`, 7 bytes | `de93a15f335abe117a5810f160b92897d3c8d8c069d01ccb16db4d862e148220` |
| Loaded prologue | `0x137CE..0x137D4`, 7 bytes | `de93a15f335abe117a5810f160b92897d3c8d8c069d01ccb16db4d862e148220` |
| Return-tail plus prologue anchor | START `0x0B27A..0x0B28A`, loaded `0x137C4..0x137D4`, 17 bytes | `543a9a9eb8255640058d16408842539adc8e66ec548e6b693a18122e4b4de9ba` |

The seven-byte prologue matches loaded RAM but occurs at three START
offsets: `0x0A42A`, `0x0A455`, and `0x0B284`. The 17-byte
contiguous span including the already pinned return tail occurs once
and identifies this site. The prologue decodes to `MOV BX,SP`,
`PUSH DS`, and `LDS DI,SS:[BX+4]`.

Given an independent readable caller frame with entry SP `S`,
the first instruction sets BX to `S`; `PUSH DS` saves the prior DS
at `SS:S-2` and changes SP to `S-2`. The `LDS` then reads an
offset word from `SS:S+4` into DI and a segment word from
`SS:S+6` into DS. These instructions leave FLAGS unchanged.
No actual caller frame or far-pointer value has been measured here.
The next instruction and first data write are outside this gate.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_060e_prologue_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
