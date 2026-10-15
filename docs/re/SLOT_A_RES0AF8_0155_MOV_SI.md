# Loaded RES_0AF8:0155 second post-call MOV

The [merged `+0152` audit](SLOT_A_RES0AF8_0152_MOV_DI.md) stops
before loaded `131C:0155`. This source-only gate bounds the
three-byte instruction there, after a hypothetical return from
the `+014F` near CALL. It stops before `131C:0158`; no return
or native reach has been observed.

`deja "Poolrad 131C:0155 MOV SI 01EE 131C:0623 0630 source branch audit"`
found no indexed session. PM assigned this follow-on in inbox i6155.
p5, pF, and pC received exact-address ownership screens before edits.
Original `0AF8:0155` is physical `0xB0D5`; START file source
is `0x0ADCB`; loaded physical is `0x13315`.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START instruction | `0x0ADCB..0x0ADCD`, 3 bytes | `6a9d0beb81be4f631820c683e61cfb1734964c19ae6e4d6504596e5b5e74dc8d` |
| Loaded instruction | `0x13315..0x13317`, 3 bytes | `6a9d0beb81be4f631820c683e61cfb1734964c19ae6e4d6504596e5b5e74dc8d` |
| Prior call plus two MOVs | START `0x0ADBF..0x0ADCD`, loaded `0x13309..0x13317`, 15 bytes | `69f3c75744af408ea7740c12131cf78eb49f55db934c3d1f5cb408e198ba717b` |

The three-byte instruction matches loaded RAM and appears twice
in START, at `0x0ACBA` and `0x0ADCB`. The 15-byte contiguous
anchor including the preceding near CALL and both MOV instructions
occurs once. The instruction decodes to `MOV SI,0x01EE`. If
reached, it sets SI to `0x01EE`, leaves FLAGS unchanged, and does
not itself access memory. The preceding call's return, stack
cleanup, and register effects are unmeasured. Original and loaded
physical addresses differ by `0x08240`; original and START file
addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0155_mov_si_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
