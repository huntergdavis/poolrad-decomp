# U3B0 +0896: bounded MOV AL,0

Original DOS v1.3 GAME.OVR U3B0 +0896 bytes are `B0 00`, MOV AL,0.
The ignored 206105-byte image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D165,0x2D167)` has
SHA-256 `46cc6a8cb5bcc0120f98b84c71699b2ac5b90cb34a781ae2a83305fa98cd8e67`.
Execution stops at +0898 before the `50` PUSH AX.

This supplied-state gate verifies IP and both loaded code bytes, sets
AL to zero while preserving AH, and advances IP to +0898. FLAGS,
including AF, all other CPU words, and all 1 MiB RAM bytes are
preserved. The reviewed `overlay_u3b0_088a_ss_al_read.c` supplies the
AL-only update convention; this gate independently checks the immediate
and original bytes.

Six synthetic full-state cases compare all fourteen CPU words and every
RAM byte, including wrapped CS code fetch, varied incoming AL/AH and
FLAGS, and stack registers. Wrong loaded bytes, wrong IP, null pointers,
and short RAM reject atomically. The required `deja` query found no
indexed match.

The original opcode span is measured; CPU/RAM frames are supplied-state
synthetic. No native reached GAME.OVR frame, natural reach, full
function closure, or strict full-C count is claimed.
