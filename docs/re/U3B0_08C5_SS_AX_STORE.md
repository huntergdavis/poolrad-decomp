# U3B0 +08C5: bounded SS word store from AX

Original DOS v1.3 GAME.OVR U3B0 +08C5 bytes are `89 46 FC`,
MOV SS:[BP-4],AX. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D194,0x2D197)`
has SHA-256 `d541bd25aeb8f3bdaa91e0ef740cbfc5b13aa35e0dd3936d935ad06154fbf7d0`.

This supplied-state gate follows the +08C1 ES read on the CF-clear
+087B taken route. It verifies IP and all three loaded code bytes,
writes AX little-endian through default segment SS at separately
wrapped 16-bit offsets BP-4 and BP-3, applies 20-bit physical wrap
to each, and stops at +08C8 before `89 56 FE` MOV [BP-2],DX.
All fourteen CPU words except IP, including all FLAGS, are preserved.
Only those two RAM bytes change. Unsupported inputs reject without
mutation.

The reviewed +08C1 successor at
`75c30a6890b36a761c638eecc370d982d1ba8898` supplied the
bounded IP/code/CF gate; the SS store semantics and displacement are
independently checked here. Base:
`80a130765833a644b8bc39dbb924c09aef356ac8`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including SS/DS separation, effective-offset and
A20 wrap. Wrong loaded bytes, IP, CF-set input, null pointers and
short RAM reject without mutation. No natural reach, native frame,
full function closure or strict full-C count is claimed.
