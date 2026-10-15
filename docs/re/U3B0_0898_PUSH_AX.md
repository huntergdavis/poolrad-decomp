# U3B0 +0898: bounded PUSH AX

Original DOS v1.3 GAME.OVR U3B0 +0898 is byte `50`, PUSH AX.
The ignored 206105-byte image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned one-byte span `[0x2D167,0x2D168)`
has SHA-256 `5c62e091b8c0565f1bafad0dad5934276143ae2ccef7a5381e8ada5b1a8d26d2`.
Execution stops at +0899 before `C4 7E FC` LES DI,SS:[BP-4].

This supplied-state gate verifies IP and the loaded code byte, decrements
SP by two, writes the current AX little-endian at SS:SP, then advances
IP to +0899. The second-byte offset wraps modulo 65536 before segment
addition; physical addresses wrap modulo 1 MiB. The reviewed
`overlay_u3b0_0895_push_ax.c` supplies the PUSH convention; this gate
independently verifies the +0898 loaded byte and stop. FLAGS, including
AF, and all CPU words other than SP/IP are preserved.

Six synthetic full-state frames compare all fourteen CPU words and every
RAM byte. Cases exercise SP underflow, second-byte offset wrap, A20
wrap, CS code-fetch wrap, and a stack write aliasing the +0898 code
byte. Wrong loaded byte, wrong IP, null pointers, and short RAM reject
without mutation. The required `deja` query found no indexed match.

The original byte is measured; CPU/RAM frames are supplied-state
synthetic. No native reached GAME.OVR frame, natural reach, full
function closure, or strict full-C count is claimed.
