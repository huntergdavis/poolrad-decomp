# U3B0 +087D: bounded fallthrough LES source

Original DOS v1.3 GAME.OVR U3B0 +087D bytes are `C4 7E FC`,
LES DI,SS:[BP-4]. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D14C,0x2D14F)`
has SHA-256 `4213bce6e9b7f566950c84818f8b9c3bf6b34a12c19a6ee084b609ea4e81426e`.

This supplied-state gate requires CF set, the fallthrough condition
from the +087B JAE, and verifies IP and all three loaded code bytes.
It reads four pointer bytes at separately wrapped SS:[BP-4..BP-1]
offsets and 20-bit physical addresses, loads DI and ES, and stops
at +0880 before the ES byte read `26 8A 05`. All other CPU words,
all FLAGS and all 1 MiB RAM bytes are preserved. Unsupported inputs
reject without mutation. The identical +08BA LES copy is outside
this gate.

The reviewed +086E LES source at
`f2e1bad1849f939c271ee636d98447f06b563d23` supplied the pointer
read pattern. Standalone base: `c1a235cbf606892ac73df111816089ae07a6bd59`.
PM-reviewed prior local head: `8917fc71881ef4e2f2963d6e702e19af0bea42ee`.
`deja` found no indexed match for this target.
Five synthetic full-state frames compare all fourteen CPU words and every
RAM byte, including SS/DS separation, offset wrap and A20 wrap.
Wrong code bytes, wrong IP, CF-clear input, null pointers and short RAM
reject without mutation. This is supplied-state parity only; no natural
reach, native frame, full function closure or strict full-C count is claimed.
