# Copied-suffix overlay parents through SYSTEM B0B copy

This gate enters SYSTEM `0AF8:0B0B` on the two measured unit-`077`
parent frames after the copied-suffix display returns. A replay-only C
bridge advances one original helper instruction at a time by composing
the existing `pr_slot_a_saved_copy_1965_step` model with the actual
nine-word frame. It stops after `RETF 4` at parent `+11F2` or `+2ADE`.
A further step returns a preserving boundary status at that first
unsupported overlay PC. Later parent instructions are separate work.

## Original sources and live frame

This branch is stacked on PM-approved, published parent call-entry head
`f74388112894c1774ddb5c42acf823ba85e95bbb`. The ignored
`START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its SYSTEM B0B body `[0xB781,0xB79B)` is 26 bytes and 12 instructions,
SHA-256 `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f`.
The ignored `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

| Parent return | Private source address / OVR file offset | Pascal length | Length-plus-content SHA-256 |
| --- | --- | ---: | --- |
| `077:+11F2` | `CS:0EB2` / `0x21EEC` | 5 | `bfccb462af8b61ee8bf8229cfd4e656d3bd0de0494320f5878228356b8142e46` |
| `077:+2ADE` | `CS:2994` / `0x239CE` | 7 | `eb27091422364253f75ff44586270aa2f3b3683e3898409592d999a57243ac97` |

The test loads those exact source spans into ignored synthetic RAM after
checking their hashes; it does not publish their content or construct a
copy result. The reached parent frame supplies return IP/CS, source
offset/segment, destination offset/segment, color, row, and length-plus-four
in nine consecutive stack words. The bridge validates the return,
source, and destination words and both complete copy spans before its
first step. It maps only the byte-identical helper's execution label
from original `0AF8` to the existing stepper's loaded `131C`; the real
parent return frame remains on the stack.

## Per-instruction parity

The original-byte fixture reaches each parent through the display's
first printer, SYSTEM suffix copy, second printer, two copied glyphs,
printer return, display epilogue, and parent argument setup. The copied
glyphs use a synthetic font byte at the prior measured boundary; the
parent B0B source bytes are the hash-checked OVR spans above. C and
original bytes match at the parent length push and the B0B entry before
the helper audit begins.

For two flag-one sites × three colors × two node-case inputs, **12/12**
reached cases match at every one of 12 instruction transitions, including
the Pascal length store, the complete forward `REP MOVSB`, DS restore,
and `RETF 4`. Another **6/6** variants set only the source length byte
to zero at helper entry and match the zero-count copy route. Each
checkpoint compares all 1 MiB RAM bytes and 14 CPU fields with defined
flags. The test also checks the retained destination pointer, source
and destination bytes after original execution, and an unchanged
CPU/RAM/report on the next unsupported overlay PC. C compilation uses
strict C11 warnings.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_suffix_parent_b0b.py
```

The zero-length variants are synthetic late mutations. This proves
parity only on these reached frames and bounded spans; it does not
establish a natural saved-DOS flag-one route, complete earlier glyph or
printer effects in C, later parent behavior, hardware EGA planes, or
strict full-C/game identity. SYSTEM B0B is runtime code and adds no
game-function credit. Original media and captures stay under ignored
`resources/`. Bounded `deja "Poolrad SYSTEM 0AF8 0B0B Pascal copy entry
nine word parent frame bounded prefix"` timed out without a result;
no prior-session assertion was reused. The
[parent call-entry gate](RESIDENT_00BA_SUFFIX_PARENT_COPY_ENTRY.md) and
[saved-copy stepper evidence](SLOT_A_SAVED_COPY_1965_JOIN.md) were
rechecked as dependencies.
