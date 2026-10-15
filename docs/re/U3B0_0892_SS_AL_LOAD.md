# U3B0 +0892: bounded SS byte load

Original DOS v1.3 GAME.OVR U3B0 +0892 bytes are `8A 46 F7`,
MOV AL,SS:[BP-9]. The ignored 206105-byte image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D161,0x2D164)` has
SHA-256 `85b0a5d59f6a309ed19f8014d36fb38888c6b1b439b1957cf576954ab3ca0014`.
Execution stops at +0895 before the `50` PUSH AX.

This supplied-state gate verifies IP and all three loaded code bytes,
reads one byte at default SS:[BP-9] with 16-bit effective-offset and
20-bit physical wrap, replaces AL while preserving AH, and advances IP.
FLAGS, including AF, all other CPU words, and all 1 MiB RAM bytes are
preserved. The reviewed `overlay_u3b0_088a_ss_al_read.c` provides the
same SS byte-load/AL preservation convention; this gate independently
checks the new displacement and original bytes.

Six synthetic full-state cases compare all fourteen CPU words and every
RAM byte. Cases exercise offset underflow, A20 wrap, CS code-fetch wrap,
SS versus DS selection, AH preservation, and varied FLAGS. Wrong loaded
bytes, wrong IP, null pointers, and short RAM reject atomically. The
required `deja` query returned this session's earlier +0892 ownership
screen; no independent prior-session result is reused.

The original opcode span is measured; CPU/RAM frames are supplied-state
synthetic. No native reached GAME.OVR frame, natural reach, full
function closure, or strict full-C count is claimed.
