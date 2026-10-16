# U3B0 +0899: bounded LES DI,SS:[BP-4]

Original DOS v1.3 GAME.OVR U3B0 +0899 bytes are `C4 7E FC`.
The ignored 206105-byte image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned three-byte span `[0x2D168,0x2D16B)`
has SHA-256 `4213bce6e9b7f566950c84818f8b9c3bf6b34a12c19a6ee084b609ea4e81426e`.
Execution stops at +089C before `26 8A 05` ES-prefixed MOV AL,[DI].

This supplied-state gate verifies IP and all three loaded code bytes. It
reads a four-byte far pointer at separately wrapped SS:[BP-4..BP-1]
offsets and 20-bit physical addresses, loads DI and ES, and advances
IP to +089C. FLAGS, including CF and AF, all other CPU words, and all
1 MiB RAM bytes are preserved. The reviewed +08BA LES gate supplies
the pointer-read convention; its route-specific CF precondition does
not apply to this instruction gate.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including CF-set acceptance, SS/DS separation, offset
wrap, A20 wrap, and wrapped CS code fetch. Wrong loaded bytes, wrong IP,
null pointers, and short RAM reject without mutation. The required `deja` query found no
indexed match.

The original opcode span is measured; CPU/RAM frames are supplied-state
synthetic. No native reached GAME.OVR frame, natural reach, full
function closure, or strict full-C count is claimed.
