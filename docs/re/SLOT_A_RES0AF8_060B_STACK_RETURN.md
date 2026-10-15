# Loaded RES_0AF8:060B stack-return tail

Both source paths from loaded `131C:0604` reach `131C:060B`: a zero
post-far-call AX jumps here, and a nonzero AX first reaches the
`DS:4426` store at `131C:0608`. This audit bounds only the three
instructions at `060B..060D`. The loaded start is physical
`0x137CB`; original `0AF8:060B` is physical `0xB58B`; START file
source is `0x0B281`.

The `deja` lookup for `131C:012D` and `131C:060B` was attempted before
this audit but the local recall service did not return. PM assigned
the source-only successor in inbox i5926. Prior PM clearance for the
containing `0604` return side, a tracked-file screen, and p5/pF
ownership queries found no reported collision.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START tail | `0x0B281..0x0B283`, 3 bytes | `5e4a22596185d7238317b5a9ab7ed66b7baae2941db391e08cd8778fd944e889` |
| Loaded tail | `0x137CB..0x137CD`, 3 bytes | `5e4a22596185d7238317b5a9ab7ed66b7baae2941db391e08cd8778fd944e889` |
| Unique return-suffix anchor | START `0x0B27A..0x0B283`, loaded `0x137C4..0x137CD`, 10 bytes | `89ccf1ac7c0aad9c03d35681e115f8b2953d425add72c23a830b90f3f0a2043b` |

The tail occurs once in START and matches loaded RAM exactly. It
decodes to `POP DI`, `POP ES`, and near `RET`. If an independent frame
supplies SS:SP and three readable words, the first word becomes DI,
the second becomes ES, and the third becomes the return IP in the
current CS. SP advances by six modulo 65536; these instructions do
not change FLAGS.

The preceding indirect far callee's target, stack cleanup, and
post-call stack contents are unmeasured. Therefore the three popped
words cannot be identified as original saved registers or a valid
caller return address. Without a supplied SS:SP frame, this gate
stops before the first POP and asserts no actual return. The next
sequential instruction is at `131C:060E`, outside this tail. Original
and loaded physical addresses differ by `0x08240`; original and
START file addresses differ by `0x030A`.

Reproduce with
`python3 tools/replay/slot_a_res0af8_060b_stack_return_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
