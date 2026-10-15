# Fourth saved-view RES_0464 +29E6 indexed word writer

## Scope and prior evidence

This bounded C gate starts at loaded `0C88:+29E6`, the fallthrough
from the separately reviewed `+29D7` exit decision, and stops before
`+29F3`. It writes one indexed data word. It is not a complete
resident routine or a direct native fourth-call result.

Before implementation, `deja "Poolrad fourth RES_0464 2632 29E6
4D70 writer 29F3 saved exit"` ran with a 35-second bound and timed
out. The reviewed branch and hash-pinned starting state were reused
from [RESIDENT_464_2632_FOURTH_29D7.md](RESIDENT_464_2632_FOURTH_29D7.md).

## Original-byte route and address evidence

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its 13-byte file span `0x705F..0x706C` maps to loaded
`0C88:+29E6..+29F3`; both span hashes are
`9ca81ab58e1a2b24f894125a76afde21c2e0ad12c9bc527463e3250a77c876b5`.
The starting 640 KiB RAM hash is
`39e9beed0ef26736226b15b280ba1bb3d2e904f9e6d93e87f008f0e190d82eb3`.

Five instructions produce six checkpoints: `+29E6` reads the stack
word at `[BP-0x16]` into AX, `+29E9` decrements it, `+29EA` reads
the index at `[BP-0x1E]` into DI, `+29ED` doubles DI, and `+29EF`
writes AX to `DS:[DI+0x4D70]`. The observed index targets
`DS:4DEC`, physical `0x197EC..0x197ED`. These are the only two
changed RAM addresses; boundary RAM SHA-256 is
`f0923e1fc6d1b28ea0da06c152b7b2a1cacf7e13da0bcd873be18ab862e8600f`.

Ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`:

    entry    0C88 29E6 148B 3E4E 0000 14A0 3E68 007C 3E80 3E5C 14A0 3DE4 1D16 7206
    boundary 0C88 29F3 148B 3E4E 0000 14A0 3E68 007C 3E80 3E5C 14A0 3DE4 1D16 7202

The C gate matches original-byte replay at all six checkpoints
across all 640 KiB RAM and all 14 CPU fields. Synthetic tests cover
the observed index, index zero, index with the high bit set, word
decrement and flag outcomes, plus invalid and unsupported input
without mutation. The public
`tools/replay/resident_464_fourth_29e6_unicorn.py` checks the ignored
image and RAM hashes and optionally writes private checkpoints under
ignored `resources/`.

This route is derived from the reviewed fourth-call prefix; a direct
same-run native fourth-call capture and its 18-byte IRQ-like return
residue remain unresolved. The bounded result does not prove final
visible-frame effects or the remainder of the resident routine.
