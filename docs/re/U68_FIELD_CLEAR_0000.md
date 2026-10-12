# Unit 0x68 internal +0x0000: clear one 2-bit field of the cell byte (full translation)

`src/recovered/overlay_u68_field_clear_0000.c` translates DOS v1.3 `GAME.OVR`
unit `0x68` internal routine `+0x0000..+0x0108`, the call entry 1 makes when its
message compare is equal ([U68_ENTRY1.md](U68_ENTRY1.md)). It clears one 2-bit
field of the byte at `far[0x6A5C] + 0x300 + 16 * y + x`: the fourth quarter of
the 1,024-byte buffer whose bit pairs unit `0x3bd` entry 3 reads
([U3BD_FIELD_ENTRY3.md](U3BD_FIELD_ENTRY3.md); buffer layout from
[U3BD_WALLSET_ENTRY7.md](U3BD_WALLSET_ENTRY7.md)). Classification: **full
translation of all 112 instructions**: no calls, and the only memory touched is
that one byte; the far pointer is explicit host state. Candidate for the strict
category only after PM parity review: **no count or credit is claimed**.

`deja "Poolrad unit 0x68 +0x0000 near callee entry-1 message equal"` found no
earlier trace. Reused and re-checked: U68_ENTRY1.md (the two call sites and the
argument order), U3BD_FIELD_ENTRY3.md (selector 6/4/2/0 = bits 7..6, 5..4, 3..2,
1..0 of `buffer[0x300 + 16 y + x]`), U68_PARTY_BYTE_0236.md (the same 16-bit
far-address treatment). Not duplicated: p5's entry-2 key dispatch and p9's save
adapter files.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| extent | unit `0x68` (file `91,851` = `0x166CB`) `+0x0000..+0x0108` = file `0x166CB..0x167D3`, **265 bytes, 112 instructions**, `retf 6` at `+0x0106`; the next routine starts at `+0x0109`, so the ledger bound is exact and there is **no trailing data**. SHA-256 `964f2b2dd6beaf0c3bf4f23235a38fd02b4aa69a9e3c8eb81c525492111e3a55` |
| frame | `push bp; mov bp, sp`, no locals; args `[bp+0xA]` x, `[bp+8]` y, `[bp+6]` selector (each pushed as a word, read as a byte); `retf 6` |
| calls | none |
| near callers | exactly two, both in entry 1: `+0x0E8D` and `+0x0FCC`, each pushing `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]` (a scan of every `push cs; call rel16` in the unit with target `+0x0000`; it is not a public entry) |
| DS reads | far pointer `[0x6A5C]` (offset word, segment word), at each of the four cases |
| memory | one byte read and written back at `es:[di + 0x300]` per case; nothing else |

## Paths

| Offsets | Effect |
| --- | --- |
| `+0x0003..+0x0008` | `al := selector`; `= 6`? else next |
| `+0x000A..+0x0044` | selector 6: `dx := cbw(x)`; `ax := cbw(y) << 4` (16-bit); `les di, [0x6A5C]; di += ax; di += dx`; `bl := es:[di + 0x300] and 0x3F`; the address is recomputed identically; `es:[di + 0x300] := bl`; to `+0x0105` |
| `+0x0047..+0x0085` | selector 4: the same with mask `0xCF` |
| `+0x0087..+0x00C5` | selector 2: the same with mask `0xF3` |
| `+0x00C7..+0x0100` | selector 0: the same with mask `0xFC` |
| `+0x00C9` | any other selector: `jne +0x0105`, nothing read or written |
| `+0x0105..+0x0108` | `pop bp; retf 6` |

The address is the original's 16-bit one: the far offset word plus `cbw(y) * 16`
plus `cbw(x)` wraps modulo 65536, and so does the `+0x300` displacement. There is
no range check: `x` and `y` are signed bytes (a value of 0x80 or more is
negative) and are not limited to 0..15. The mask clears exactly the bit pair
entry 3 reads for that selector; the test confirms this by running the merged
entry 3 on the result.

## Host representation and guards

* `pr_u68_0000_state` holds `segment` (the host bytes of `[0x6A5C]`'s segment
  from offset 0, at most 64 KiB), `segment_size` and the far `offset` word.
* `PR_U68_0000_ARGUMENT` (nothing written): a null state or segment, a view
  larger than 64 KiB, or a (wrapped) address at or past the view. A selector
  outside {0, 2, 4, 6} never touches the view and returns `OK` without
  reading it, as the original does.
* No game data is embedded; the buffer content is whatever the host supplies.

## Dependencies

No unresolved dependency: the routine has no callee. The only external state is
the 1,024-byte buffer behind `[0x6A5C]`, filled by unit `0x3bd` entry 7 (merged
as a bounded model, [U3BD_WALLSET_ENTRY7.md](U3BD_WALLSET_ENTRY7.md)); the host
supplies it here. Its effect in entry 1's flow: after the message compare is
equal, entry 1 calls it with the party's `(x, y, facing)`, then takes its
`[bp-1] = 0` path (`0x8AE:0x00DC`); what that means in game terms is not
asserted.

## Verification

`tests/recovered/test_overlay_u68_field_clear_0000.c`: every `x` and `y` byte
against an independent restatement with wide integers and an explicit modulo
(`(offset + 16 sy + sx) mod 65536 + 0x300`), for the four live selectors and
seven others, at four far offsets: the result byte, the report, the
out-of-view guard, and that only the one byte changes (full buffer comparison on
a sample); three 16-bit wrap cases on a 64 KiB view (`0xFF00` + `0x0353` wraps to
`0x0253`; `x = -1` at offset `0xFFFF`; `y = -128` at offset 0); the merged entry
3 run on the cleared byte (the selected facing reads 0, the other three still
read 3); the guards. About 2.9 million cases, 0.3 s, clean under AddressSanitizer
and UndefinedBehaviorSanitizer. The restatement shares my reading of the decode;
the byte-level correspondence is the table above, not a DOS replay.
