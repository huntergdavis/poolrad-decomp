# Loaded RES_0AF8:0608 nonzero return word store

The source-only `131C:0604` decision falls through to loaded
`131C:0608` when supplied post-far-call AX is
nonzero. This instruction is at physical `0x137C8`; original
`0AF8:0608` is physical `0xB588`, and START file source is `0x0B27E`.
The audit bounds this one word store and stops at the next instruction,
`POP DI` at `131C:060B`.

`deja "Poolrad 131C:0123 DS:4418 far pointer consumer 131C:0608 DS:4426 store source"`
found no indexed session. PM assigned this source-only follow-on in
inbox i5860. PM's i5847 clearance for the containing `0604`
return side and a tracked-file screen found no overlap.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| Unique return suffix anchor | START `0x0B27A..0x0B283`, loaded `0x137C4..0x137CD`, 10 bytes | `89ccf1ac7c0aad9c03d35681e115f8b2953d425add72c23a830b90f3f0a2043b` |
| Nonzero store | START `0x0B27E..0x0B280`, loaded `0x137C8..0x137CA`, 3 bytes | `bf754db91e10c283bf2b78bd69d4d97ccf871861a819899ad2648cdd9ba5722e` |
| Stack-return successor | START `0x0B281..0x0B283`, loaded `0x137CB..0x137CD`, 3 bytes | `5e4a22596185d7238317b5a9ab7ed66b7baae2941db391e08cd8778fd944e889` |

The three-byte instruction matches loaded RAM and stores AX as a
`uint16_t` at `DS:4426`. It has 13 START occurrences by itself; the
unique ten-byte return suffix and the preceding `OR AX,AX` / `JE`
anchor this site. The source `JE` is not taken only if supplied AX is
nonzero. A zero AX skips this store and enters `131C:060B` directly.

The far callee's return AX and DS state are unsupported inputs. If
AX or the writable DS memory at `4426..4427` is unavailable, a model
must stop at the reached decision or store, preserving prior effects.
This audit supplies no actual AX, DS word, observed write, or stack
frame. The following `POP DI`, `POP ES`, and RET need independent
SS:SP and stack evidence; their effects are outside this gate.
Original and loaded physical addresses differ by `0x08240`;
original and START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_0608_error_store_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
