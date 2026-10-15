# U3B0 +088E: bounded CBW

Original DOS v1.3 GAME.OVR U3B0 +088E byte is `98`, CBW.
The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D15D,0x2D15E)`
has SHA-256 `79bec7ff3e69d1b470eb8684f2005a867a7980222474cbc1afd26a9a4f206d4a`.

This supplied-state gate follows the +088A SS byte load into AL.
It verifies IP, the loaded CBW byte, cleared AH and the defined
+0883 XOR-result CF/PF/ZF/SF/OF bits. CBW copies AL's sign bit
into all of AH, then stops at +088F before `03 C2` ADD AX,DX.
Every other CPU word, every FLAGS bit and all 1 MiB RAM bytes are
preserved. Unsupported inputs reject without mutation.

The reviewed +088A predecessor at
`161f8c78b860506bc967f126ffc195f306c57cfd` supplies the
cleared-AH and defined-flag conditions. The +0883 AF value remains
an architecturally undefined local placeholder, so no original
same-frame AF or complete FLAGS parity is claimed. Base:
`f02a70acf2b695c51a2246019b9a38b345499458`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including AL 0x7F/0x80, 0x00/0xFF and different
unrelated registers and FLAGS. Wrong loaded byte, IP, predecessor
state, null pointers and short RAM reject without mutation.
The opcode span is measured from the original image; CPU/RAM cases
are synthetic supplied states. No native reached frame, natural
reach, full function closure or strict full-C count is claimed.
