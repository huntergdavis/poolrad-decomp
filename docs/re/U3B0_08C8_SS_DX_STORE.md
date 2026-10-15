# U3B0 +08C8: bounded SS word store from DX

Original DOS v1.3 GAME.OVR U3B0 +08C8 bytes are `89 56 FE`,
MOV SS:[BP-2],DX. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D197,0x2D19A)`
has SHA-256 `c27a51a4cdc6e664c4e004ef62bd8967d4bad7257dc79300959f88c9f1aa47a9`.

This supplied-state gate follows the +08C5 AX store on the CF-clear
+087B taken route. It verifies IP and all three loaded code bytes,
writes DX little-endian through default segment SS at separately
wrapped 16-bit offsets BP-2 and BP-1, applies 20-bit physical wrap
to each, and stops at +08CB before `FE 46 F7`.
All fourteen CPU words except IP, including all FLAGS, are preserved.
Only those two RAM bytes change; the preceding AX store at BP-4
remains intact. Unsupported inputs reject without mutation.

The reviewed +08C5 predecessor at
`b399905c8211debd39d86d70d3a8b011da4b43f9` supplied the
bounded IP/code/CF gate; the SS store displacement and DX source are
independently checked here. Base:
`0f14f40a0f88f89aec95fedf23f32272f8654ac2`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including SS/DS separation, effective-offset and
A20 wrap and preserved preceding AX-store bytes. Wrong loaded bytes,
IP, CF-set input, null pointers and short RAM reject without mutation.
No natural reach, native frame, full function closure or strict
full-C count is claimed.
