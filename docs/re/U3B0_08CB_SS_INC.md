# U3B0 +08CB: bounded SS byte increment

Original DOS v1.3 GAME.OVR U3B0 +08CB bytes are `FE 46 F7`,
INC byte SS:[BP-9]. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D19A,0x2D19D)`
has SHA-256 `93fe24de287b2f6a4cd32978a770b96f89542c205e604a1d1f285f04119419d4`.

This supplied-state gate follows the +08C8 DX store on the CF-clear
+087B taken route. It verifies IP and all three loaded code bytes,
increments one byte at default SS:[BP-9] using 16-bit offset and
20-bit physical wrap, and stops at +08CE before `E9 3B FF` JMP.
INC updates OF/SF/ZF/AF/PF from the 8-bit result and preserves CF
and all other FLAGS bits. All other CPU words and all other 1 MiB RAM
bytes are preserved. Unsupported inputs reject without mutation.

The reviewed +08C8 predecessor at
`e3d24263a07f1087213c3ec7885ce52ccd643115` supplied the
bounded IP/code/CF gate; increment flags and BP-9 displacement are
independently checked here. Base:
`6a32af48d9bfee330ead50ce9d3824a77d81985f`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including zero, auxiliary carry, signed overflow,
0xFF wrap, even parity, effective-offset and A20 wrap. Wrong loaded
bytes, IP, CF-set input, null pointers and short RAM reject without
mutation. The opcode span is measured from the original image;
CPU/RAM cases are supplied-state synthetic. No native reached frame,
natural reach, full function closure or strict full-C count is claimed.
