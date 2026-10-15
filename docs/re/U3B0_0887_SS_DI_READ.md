# U3B0 +0887: bounded SS word read into DI

Original DOS v1.3 GAME.OVR U3B0 +0887 bytes are `8B 7E 06`,
MOV DI,SS:[BP+6]. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D156,0x2D159)`
has SHA-256 `363d5b333a8be5412d48aea27ced35bd0571ce84f1dfd9e0697eb0ba35dd7989`.

This supplied-state gate follows the +0885 MOV DX,AX successor
of +0883 XOR AH,AH. It verifies IP, all three loaded code bytes,
cleared AH, DX=AX and the defined XOR-result CF/PF/ZF/SF/OF bits.
It reads a little-endian word from default segment SS at separately
wrapped 16-bit offsets BP+6 and BP+7, applies 20-bit physical wrap
to each, replaces DI, and stops at +088A before `36 8A 45`.
All other CPU words, every FLAGS bit and all 1 MiB RAM bytes are
preserved. Unsupported inputs reject without mutation.

The reviewed +0885 successor at
`049cbeded1d1162046045d6af7027c6b67be01c1` supplies the
AX/DX and defined-flag conditions. The +0883 AF value remains an
architecturally undefined local placeholder, so no original
same-frame AF or complete FLAGS parity is claimed. Base:
`97a1ef0990f5ad6cb570b40a94d866c63452af2b`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including SS/DS separation, effective-offset and
A20 wrap. Wrong loaded bytes, IP, predecessor state, null pointers
and short RAM reject without mutation. No natural reach, native
frame, full function closure or strict full-C count is claimed.
