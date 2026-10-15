# U3B0 +0883: bounded XOR AH,AH

Original DOS v1.3 GAME.OVR U3B0 +0883 bytes are `30 E4`,
XOR AH,AH. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D152,0x2D154)`
has SHA-256 `3514181eb1bfa6790767368f32a6841c5ee79054c8bc877a23ed6b79de407f5d`.

This supplied-state gate requires CF set from the +087B fallthrough,
verifies IP and both loaded code bytes, clears AH while preserving AL,
sets the defined XOR result flags ZF and PF, clears defined CF/SF/OF,
and stops at +0885 before `8B D0` MOV DX,AX. It preserves all other
CPU words and all 1 MiB RAM bytes. Unsupported inputs reject without
mutation.

AF is architecturally undefined for XOR. The gate preserves supplied
AF only as an explicit local placeholder, matching the reviewed U3B0
+0E2A XOR convention at
`f170c87274305d41af7306d977c4cc0e0efac65b`. No original
same-frame parity is claimed for AF or the complete FLAGS word.
Base: `91ebe4d849b4fd8ced06d3871c6782dfc120a945`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words under
that placeholder policy and every RAM byte, covering initial AF set
and clear plus other FLAGS. Wrong loaded bytes, wrong IP, CF-clear
input, null pointers and short RAM reject without mutation.
No natural reach, native frame, full function closure or strict full-C
count is claimed.
