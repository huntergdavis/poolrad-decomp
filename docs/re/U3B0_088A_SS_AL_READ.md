# U3B0 +088A: bounded SS byte read into AL

Original DOS v1.3 GAME.OVR U3B0 +088A bytes are
`36 8A 45 1E`, SS-prefixed MOV AL,[DI+1Eh]. The ignored
206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D159,0x2D15D)`
has SHA-256 `d216f7bd251bf3fe151ac6609a803d5fd70a09a8590fc9051adff7ad8cdb351e`.

This supplied-state gate follows the +0887 SS word read into DI
on the +0883 XOR route. It verifies IP, all four loaded code bytes,
cleared AH, DX=AX and the defined XOR-result CF/PF/ZF/SF/OF bits.
It reads one byte from explicitly selected SS at a wrapped 16-bit
DI+0x1E offset and 20-bit physical address, replaces AL, and stops
at +088E before `98` CBW. Every other CPU word, every FLAGS bit and
all 1 MiB RAM bytes are preserved. Unsupported inputs reject without
mutation.

The reviewed +0887 successor at
`e9c46b400b753b7e5704adaaa50da5a2f9f823bc` supplies the
AX/DX and defined-flag conditions. The +0883 AF value remains an
architecturally undefined local placeholder, so no original
same-frame AF or complete FLAGS parity is claimed. Base:
`72d22eaa8da53392b6c7417485f3129a31a848cf`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including SS/DS separation, effective-offset and
A20 wrap. Wrong loaded bytes, IP, predecessor state, null pointers
and short RAM reject without mutation. No natural reach, native
frame, full function closure or strict full-C count is claimed.
