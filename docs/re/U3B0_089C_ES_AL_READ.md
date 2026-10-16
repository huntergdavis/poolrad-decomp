# U3B0 +089C: bounded ES byte read

Original DOS v1.3 GAME.OVR U3B0 +089C bytes are `26 8A 05`,
ES-prefixed MOV AL,[DI]. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D16B,0x2D16E)` has
SHA-256 `1ce94d634545b4f8d5e88fdc1ad48622f1b9dac345761bc1f88dce3496c89b51`.
Execution stops at +089F before `30 E4` XOR AH,AH.

This supplied-state gate verifies IP and all three loaded code bytes,
reads one byte at the 20-bit wrapped ES:DI physical address, replaces
only AL, and advances IP to +089F. AH, FLAGS including AF/CF, all other
CPU words, and all 1 MiB RAM bytes are preserved. The reviewed +0880
ES byte-read model supplies the AL-only and segment convention; its
route-specific CF precondition does not apply here.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including CF-clear and CF-set acceptance, ES/DS
separation, A20 wrap, and wrapped CS code fetch. Wrong loaded bytes,
wrong IP, null pointers, and short RAM reject without mutation. The
required `deja` query found no indexed +089C match.

The original opcode span is measured; CPU/RAM frames are supplied-state
synthetic. No native reached GAME.OVR frame, natural reach, full
function closure, or strict full-C count is claimed.
