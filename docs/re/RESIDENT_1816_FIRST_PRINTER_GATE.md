# RES_00BA:1816 first-printer entry gate

This bounded C state transition executes `RES_00BA:1816` through the entry
of its first `CALL FAR RES_0709:0626`. It ends before the printer body. It
does not claim strict full-C or game-function credit. The later flag-dependent
suffix and `0AF8:0B0B` copy remain separate dependencies.

## Provenance and boundary

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `resources/DOS/POOLRAD/START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The hash-locked loader maps the exact prefix `[00BA:1816,00BA:1858)` to
expanded image `[0x023B6,0x023F8)`. Its 66 bytes hash to
`7b68709c7c03334a3b277bf7e2b100849407e2bf09c1f17b1e34f03e578c7d99`.
Capstone confirms the two conditional branches `1825→182D` and `1836→183E`,
and the final instruction at `1853` is `CALL FAR 0709:0626`. The C model
stops with `CS:IP=0709:0626` and the far return address `00BA:1858` on
the stack. No original or derived instruction bytes are published.

The starting stack has the far return address, then flag, row, column,
node offset, and node segment words. The prefix preserves AH while replacing
AL with the scalar low bytes, loads ES:DI from the node pointer, writes the
selected color to `[BP-1]`, pushes five printer arguments, and finally pushes
the far return CS and IP. The color is C if node `[+10D]` is zero, E if
`[+10D]` is nonzero and `[+10E]` is one, or B otherwise. The C transition
also models the last `CMP` flags and the 20-bit real-mode memory wrap.

## Differential result

`tests/recovered/check_resident_1816_printer_gate.py` compiles
`tools/replay/resident_1816_printer_gate.c` with `-std=c11 -Wall -Wextra
-Wpedantic -Werror`, executes the pinned original prefix in Unicorn, and
compares the C transition at the first printer instruction. Nine cases cover
all three color paths with three seed frames, including stack wrap, node
offset wrap, varied AH and initial flags. Each case compares all 14 CPU
register fields (architecturally meaningful flags) and every byte of 1 MiB
RAM. All nine pass. The assertions also check the seven-word printer entry
stack: return IP, return CS, node offset, node segment, color, row, column.

Run locally with the original input kept under ignored `resources/`:

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_1816_printer_gate.py
```

This is a first-entry gate only. A complete eight-site caller-to-callee
composition must stop at this dependency until the printer body or a
specified callback boundary is independently verified. The existing
`slot_a_printer_page` model covers one EGA page effect, not the printer's
whole CPU, stack, or memory behavior.

Before implementation, bounded `deja "RES_0709:0626 printer original C
parity ownership"` timed out with no result. No prior-session assertion was
reused. The merged `RESIDENT_UNEQUAL_DISPLAY_1816.md` and
`SLOT_A_FIRST_VIEW_PRINTER_PAGE.md` served as leads; addresses, branch
targets, and frame effects here were rechecked against pinned original bytes.
