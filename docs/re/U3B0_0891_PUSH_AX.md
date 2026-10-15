# U3B0 +0891: bounded PUSH AX

Original DOS v1.3 GAME.OVR U3B0 +0891 is byte `50`, PUSH AX.
The ignored 206105-byte image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned one-byte span `[0x2D160,0x2D161)`
has SHA-256 `5c62e091b8c0565f1bafad0dad5934276143ae2ccef7a5381e8ada5b1a8d26d2`.
Execution stops at +0892 before `8A 46 F7` MOV AL,SS:[BP-9].

This supplied-state gate verifies IP and the loaded code byte, decrements
SP by two, writes AX little-endian at SS:SP, then advances IP to +0892.
SP and the second-byte offset wrap modulo 65536 before segment addition;
physical addresses wrap modulo 1 MiB. This 8086 boundary behavior is
specified by [Intel SDM volume 3B §25.33.1](https://cdrdv2-public.intel.com/874250/253669-090-sdm-vol-3b.pdf),
including PUSH when the initial SP is 1. The existing U3B0 entry6
`push16` helper supplies the SP decrement convention, while the reviewed
`overlay_u3b0_08c8_ss_dx_store.c` supplies the offset-before-physical
word-byte addressing convention. FLAGS, including AF, and all CPU words
other than SP/IP are preserved.

Six synthetic full-state frames compare all fourteen CPU words and every
RAM byte. Cases exercise SP underflow, second-byte offset wrap, A20 wrap,
CS code-fetch wrap, and a stack write aliasing the loaded code byte.
Wrong loaded byte, wrong IP, null pointers, and short RAM reject without
mutation. Required `deja` queries for this lead timed out; no indexed
prior-session result is reused.

The original byte is measured; CPU/RAM frames are supplied-state
synthetic. No native reached GAME.OVR frame, natural reach, full
function closure, or strict full-C count is claimed.
