# U3B0 +0874: bounded SS word read

Original DOS v1.3 GAME.OVR U3B0 +0874 bytes are `8B 7E 06`,
MOV DI,SS:[BP+6]. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D143,0x2D146)`
has SHA-256 `363d5b333a8be5412d48aea27ced35bd0571ce84f1dfd9e0697eb0ba35dd7989`.

On an independently supplied CPU/1 MiB RAM, this gate first verifies the
fourteen-word CPU's IP and all three loaded code bytes at CS:+0874.
It reads the low and high bytes through SS with separately wrapped
16-bit offsets and 20-bit physical addresses, sets DI, and stops at
+0877. It preserves all other CPU words, including arbitrary FLAGS, and RAM.
The stop is before the SS-prefixed compare at +0877.

The reviewed +085C DI read at
`45acc769fc89892da2d8e798d83a2e81fa87d332` supplied the SS word-read
pattern, with its ZF-set path restriction removed here because +0874
follows an unresolved callee return. Standalone base: `e337afcce347b453c5379a1930f179a8c8caa0e0`.
PM-reviewed prior gate head: `f34493cf0e482e163e5eb1f2e8bdea1e3b28f7d9`.
`deja` found no indexed match for this target.

Five synthetic full-state frames compare all fourteen CPU words and every
byte of 1 MiB RAM, including SS/DS separation, offset wrap and A20 wrap.
Wrong code bytes, wrong IP, null pointers and short RAM reject without
mutation. This is supplied-state parity only; no natural callee return,
native reached frame, full function closure or strict full-C count is claimed.
