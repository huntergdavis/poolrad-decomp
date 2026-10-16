# U3B0 +089F: bounded XOR AH,AH

Original DOS v1.3 GAME.OVR U3B0 +089F bytes are `30 E4`, XOR AH,AH.
The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D16E,0x2D170)` has
SHA-256 `3514181eb1bfa6790767368f32a6841c5ee79054c8bc877a23ed6b79de407f5d`.
Execution stops at +08A1 before `8B D0` MOV DX,AX.

This supplied-state gate verifies IP and both loaded code bytes,
clears AH while preserving AL, sets the defined XOR result flags ZF
and PF, clears defined CF/SF/OF, and advances IP to +08A1. It preserves
all other CPU words and all 1 MiB RAM bytes. The reviewed +0883 XOR AH
gate supplies the defined logic-flags convention; its route-specific
CF precondition does not apply here.

AF is architecturally undefined for XOR. Preserving supplied AF is an
explicit local placeholder, following the reviewed +0883 gate. No
original same-frame parity is claimed for AF or the complete FLAGS
word. Six synthetic full-state frames compare all fourteen CPU words
under that placeholder policy and every RAM byte, covering incoming
AF and CF set and clear plus wrapped CS code fetch. Wrong loaded bytes,
wrong IP, null pointers, and short RAM reject without mutation. The
required `deja` query found no indexed +089F match.

The original opcode span is measured; CPU/RAM frames are supplied-state
synthetic. No native reached GAME.OVR frame, natural reach, full
function closure, or strict full-C count is claimed.
