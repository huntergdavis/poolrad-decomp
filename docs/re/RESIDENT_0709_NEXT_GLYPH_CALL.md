# RES_0709:0694 next glyph-call boundary

This gate isolates the printer loop edge after its first `0709:01DF`
glyph drawer returns. A two-character synthetic Pascal source starts at
each of the eight pinned overlay callers and reaches the first drawer
through the merged `1816` and `0626` routes. Original code executes the
first complete glyph and returns to printer `0709:0694`. The new C
transition starts at that return and stops at the second `0709:01DF`
entry, before the second glyph's side effects.

## Exact bounds and route

The ignored original `START.EXE` is 65,231 bytes with SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is 206,105 bytes with SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The test executes the expanded glyph drawer `[0x726F,0x767A)` (1,035
bytes, SHA-256
`8d71dd4c0162462f99aef0fbbd4cfee472702f330bca1e07b50f192ffd972aef`)
and original SYSTEM upcase helper START `[0xC4A0,0xC4B3)` (19 bytes,
SHA-256
`738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c`).
The prior [post-copy control gate](RESIDENT_0709_0626_POSTCOPY_CONTROL.md)
pins the printer, Pascal string-copy helper, and eight overlay caller
prefixes. The test checks those spans again against the ignored inputs.

At `0694`, the original increments the low column byte, tests the
copied Pascal length against the one-based glyph index, increments that
index, and pushes column, row, color, one-cell count, indexed character,
upcase flag, CS, and return IP for the next call at `0691`. For the
synthetic two-character source, the second entry frame starts with
`0694`, `0709`, upcase 1, the second synthetic character, and a word
whose low byte is count 1. Its high byte is preserved from the drawer's
return AX until `XOR AH,AH` later in the argument setup. The test
separately checks the column and index advances.

## Differential result and limits

**24/24** eight caller × three `1816` color cases match. At the second
drawer entry, C and original-byte execution have the same 14 CPU fields
and every byte of 1 MiB flat RAM. AF is excluded from arithmetic flag
comparison after reached `XOR` instructions because it is undefined;
other meaningful flags are checked. The test compiles the C source with
`-std=c11 -Wall -Wextra -Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_next_glyph_call.py
```

The EGA mode, page, mask table, font pointer and characters are synthetic.
Unicorn uses flat mapped `A000` memory rather than hardware planes. The
first glyph's original-byte drawer and page writes are executed only to
reach the same-run `0694` stop; this C model covers `0694` to the next
`01DF` entry. The second glyph drawer, later printer return, and the
separate `0709:0768` route remain outside this gate. There is no strict
full-C or playable-game credit.

Before implementation, bounded
`deja "Poolrad RES_0709 0694 second glyph call 0691 Pascal printer"`
timed out without an indexed result. The original printer loop from the
merged post-copy evidence was independently checked against the ignored
bytes. All game files and any capture files stay under ignored
`resources/`; the public record contains addresses, counts and hashes.
