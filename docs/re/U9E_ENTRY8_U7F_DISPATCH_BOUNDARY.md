# U9E entry8 U7F caller/dispatch boundary

This source-only gate follows the merged
[U9E loaded-entry composition](U9E_ENTRY8_FULLSTATE_COMPOSITION_0DE3.md).
The targeted `deja` query for this boundary returned no indexed session.
The merged [U7F call adapter](U7F_ROLL_CALL_0FDE.md),
[callback map](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md), and
[resident loader map](RESIDENT_LOADER_MAP.md) supply prior frame and
loader behavior. This gate independently checks the specific original
U7F call and U9E entry-8 stub without executing the dispatcher.

The owner-supplied DOS v1.3 media have SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
for `START.EXE` and
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
for `GAME.OVR`. The static checker
`tests/recovered/check_overlay_u9e_entry8_dispatch_boundary.py`
pins these bytes and the loader-expanded image:

| Boundary | Original bytes and address | SHA-256 |
| --- | --- | --- |
| U7F +0FDE far call | `GAME.OVR` file `0x25C67`, `9A 48 00 B0 00`; segment fixup at +0FE1 | `e4551a2bc99d5c8ac9cc97784911c182c2470f56668e8cf8bdb3a895ea4d9115` |
| U9E public entry-8 stub | `START.EXE` file `0xC2B`, packed image `0xA2B`, expanded image `0xB48` = `B0:0048`; `CD 3F E3 0D 00` | `16e87d451726931b6e084c2a73e82f385b41e3886c5d7a9806ec0fd814f991f2` |
| U9E entry-8 body | `GAME.OVR` file `[0x2BCCD,0x2BD18)`, 75 bytes / 29 instruction starts | `7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba` |

U9E's descriptor stores public entry 8 at +0DE3, next entry 9 at
+0E2E, and expanded-image stub paragraph `0xB0` **before DOS load
relocation**. The existing entry-8 audit
independently counts 122 fixup-confirmed far callers and pins the
SYSTEM +11C0/+120F body. The new checker only proves that the expanded
stub bytes match the packed descriptor and that this U7F call targets
it. `CD 3F` is the cold `INT 3Fh` form; a live loader may patch a stub
to a far jump, as measured for **other** overlay units. No U9E entry-8
stub was captured here, so neither a cold path nor a patched path is
asserted for this call.

## Composition seam

The merged U7F +0FDE adapter stops at source-image `B0:0048` with the
modeled far-call return frame; native loaded calls use a relocated
segment that must be measured in that run. The merged U9E composition starts later at
**loaded U9E CS:+0DE3**, with that same frame and original loaded
instruction bytes. The missing seam includes the actual `INT 3Fh`
vector/handler or patched stub jump, resulting loaded CS, any RAM and
CPU14 effects, and possible transfer/nonreturn. A static descriptor
word cannot supply that live CS or all dispatch effects. The resident
INT 3Fh handler/loader already has a separate owner; this branch does
not change it. A U7F callback cannot call the loaded-entry composition
as if B0 were the U9E code segment.

When the host native hold is lifted, first capture a genuinely reached
U7F +0FDE call and `B0:0048` stop. Record all 14 CPU words, full 1 MiB
A20-off RAM, the actual stub bytes, vector, return IP/CS words and
arguments. Follow the actual dispatch to U9E +0DE3 or another observed
destination, recording every stack and RAM change. A later C adapter
must compare the entire dispatch transition and return against that
frame, including code aliases and 16-bit SS offset wrap. If no original
U7F frame is reached, keep any controlled callback tests provisional.
The existing loaded-entry composition does not establish natural reach,
original dispatch parity, or additional strict full-C credit; the
project count remains 82.

Static verification only:

```sh
python3 tests/recovered/check_overlay_u9e_entry8_dispatch_boundary.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR
```
