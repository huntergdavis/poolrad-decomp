# U3B0 +0877: bounded SS byte compare

Original DOS v1.3 GAME.OVR U3B0 +0877 bytes are `36 3A 45 AC`,
SS-prefixed CMP AL,[DI-54h]. The ignored 206105-byte DOS image has
SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D146,0x2D14A)`
has SHA-256 `1025f7ef69d85b975825d564d3192a3d0c8ec09317eae695580822b79706a0e3`.

On an independently supplied CPU/1 MiB RAM, this gate verifies IP and all
four loaded instruction bytes at CS:+0877. It reads SS at the 16-bit
wrapped offset DI-0x54, then 20-bit wrapped physical address. It computes
AL minus that byte solely for CF, PF, AF, ZF, SF and OF; all other FLAGS,
all other CPU words and RAM are preserved. It stops at +087B before
the original `73 3D` JAE control instruction.

The reviewed +081D compare model at
`cdc38a67cd3fb36bcc036efe7116fc593de3b95f` supplied the x86
arithmetic flag equations; this target uses its distinct negative
displacement and an exact loaded-byte gate. Stack base: standalone +0874 `a50f7cba27752ea597ab3df30f0aa8113174b417`.
PM-reviewed prior gate head: `e01ad8c2dbda197c6ce7bfe0b5fa78b70556b011`. `deja` found no indexed match.
Six synthetic full-state frames compare all fourteen CPU words and
every byte of 1 MiB RAM, covering each arithmetic flag, preserved FLAGS,
SS/DS separation, offset wrap and A20 wrap. Wrong code bytes, wrong IP,
null pointers and short RAM reject without mutation. This is supplied-state
parity only; no natural callee return, native reached frame, full function
closure or strict full-C count is claimed.
