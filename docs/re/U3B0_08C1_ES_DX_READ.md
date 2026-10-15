# U3B0 +08C1: bounded ES word read into DX

Original DOS v1.3 GAME.OVR U3B0 +08C1 bytes are
`26 8B 55 2C`, ES-prefixed MOV DX,[DI+2Ch]. The ignored
206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D190,0x2D194)`
has SHA-256 `9260320ab9bd5e86c2e2736ae435023ded239c7b58c4d83feb939cb07af122c5`.

This supplied-state gate follows the +08BD ES word read on the
CF-clear +087B taken route. It verifies IP and all four loaded code
bytes, reads a little-endian word through ES at separately wrapped
16-bit offsets DI+0x2C and DI+0x2D, applies 20-bit physical wrap to
each, replaces DX, and stops at +08C5 before `89 46 FC`
MOV [BP-4],AX. All other CPU words, every FLAGS bit and all 1 MiB
RAM bytes are preserved. Unsupported inputs reject without mutation.

The reviewed +08BD word-read model at
`0799588ad73ff668da270329cbf08ff9904d059a` supplied the
separate-offset wrap pattern; displacement and destination are
independently verified here. Base:
`456d9ec6a2024419c1ec81a1f05bb0be425196dc`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including ES/DS separation, effective-offset and
A20 wrap. Wrong loaded bytes, IP, CF-set input, null pointers and
short RAM reject without mutation. No natural reach, native frame,
full function closure or strict full-C count is claimed.
