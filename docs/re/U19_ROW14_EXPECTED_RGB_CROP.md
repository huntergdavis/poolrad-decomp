# U19 fixed row-14 expected RGB crop

This source-only C adapter derives the 36-cell fixed U19 row-14 printer crop
from caller-supplied source bytes and the decoded DAX font. It also compares
the expected crop with a caller-supplied canonical 640×400 RGB24 frame. No
host frame, guest page, or emulator state is embedded or inferred.

A bounded `deja "Poolrad U19 expected row14 source map visual comparison
codeword prompt 459 cell credits map"` attempt timed out without usable
output. The [fixed-text source](COLD_START_CODEWORD_SOURCE.md),
[printer route](COLD_START_CODEWORD_ROW14_PRINTER_ROUTE.md),
[font contract](FONT_8X8D_201.md), and
[paired-frame boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md) supplied
leads. The original caller and source intervals were checked again against
the ignored DOS v1.3 image before this adapter was written.

The ignored `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U19 `+02C0..+02E2`, file `[0x0C6C,0x0C8E)`, is 34 bytes, SHA-256
`144b7073fef51cd92e155eaf08dd5c679772c7788a989432d2bafc95356647ce`.
Its decoded sequence passes low-byte column 1, row 14, color 10, a local
string buffer and fixed U19 source `+00A9` through the Pascal copy, then
calls the resident printer at U19 `+02D8`. The printer-call instruction,
file `[0x0C84,0x0C89)`, hashes
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`.
The length-prefixed source, file `[0x0A55,0x0A7A)`, has length 36 and SHA-256
`b30aab4f52eef30b18a13ad80c2eaf017b15cc6a08c70528a97fdc51539e316f`.
The decoded 1,416-byte `8X8D1.DAX` block 201 hashes
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
The caller must verify the original inputs before extracting the 36 source
bytes and font for this C function.

`pr_u19_row14_expected_rgb24` applies the recovered lowercase-to-uppercase,
modulo-64 glyph lookup and each glyph row's leftmost bit first. Set bits are
EGA color 10, RGB `(85,255,85)`; cleared bits are black. The 288×8 source
crop has 2,304 pixels: 802 foreground and 1,502 black. Doubling both axes
produces a 576×16 RGB24 crop at host x `16..591`, y `224..239`: 27,648 bytes,
SHA-256
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`.
The hash was reproduced by an ignored private C smoke and an independent
source calculation. No source text or font pixels are tracked.

`pr_u19_row14_compare_host_rgb24` accepts a separate typed 640×400 RGB24
frame. It counts different host pixels, source pixels with any differing
member of their 2×2 block, affected cells, and nonuniform 2×2 blocks. It
does not select a guest EGA page or locate a live writer. The focused
synthetic C11 test passed with `-Wall -Wextra -Wpedantic -Werror`. The
retained U19 host capture is absent from this isolated checkout, so this
change does not report an independently run native comparison. The merged
paired-frame note separately reports a row-14 mismatch of 477 source pixels
at `+02DD`, `+02E2`, and `+02FD`, resolving at `+0302`.

This source crop and comparator add no native route, first-writer, page,
full-frame, strict game-function, or C-native parity credit. The separate
U17 459-cell credits map is not part of this U19 branch. Source-only
follow-ons are the first credits border/caption expected cells and the
title-to-credits transition image origin.
