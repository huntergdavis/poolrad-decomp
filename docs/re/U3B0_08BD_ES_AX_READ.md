# U3B0 +08BD: bounded ES word read

Original DOS v1.3 GAME.OVR U3B0 +08BD bytes are `26 8B 45 2A`,
ES-prefixed MOV AX,[DI+2Ah]. The ignored 206105-byte DOS image has
SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D18C,0x2D190)`
has SHA-256 `7f2a2d51a09a41364d7c54e0d4b819e04621fc9429287d372929bdbec1e43f9e`.

This supplied-state gate requires CF clear from the +087B JAE taken
route through +08BA LES and verifies IP and all four loaded code bytes.
It reads a little-endian word through ES at separately wrapped 16-bit
offsets DI+0x2A and DI+0x2B, applies 20-bit physical wrap to each,
replaces AX, and stops at +08C1 before the next ES word read
`26 8B 55 2C`. All other CPU words, all FLAGS and all 1 MiB RAM
bytes are preserved. Unsupported inputs reject without mutation.

The reviewed +0874 word-read model at
`a50f7cba27752ea597ab3df30f0aa8113174b417` supplied the
separate-offset wrap pattern; the segment and displacement are
independently verified here. Base:
`a6261d57718070c77b5fffbed8ff0a936cd3a103`.
The required `deja` query found no indexed match.
Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including ES/DS separation, effective-offset and
A20 wrap. Wrong loaded bytes, wrong IP, CF-set input, null pointers
and short RAM reject without mutation.
No natural reach, native frame, full function closure or strict full-C
count is claimed.
