# U3B0 +0880: bounded ES byte read

Original DOS v1.3 GAME.OVR U3B0 +0880 bytes are `26 8A 05`,
ES-prefixed MOV AL,[DI]. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D14F,0x2D152)`
has SHA-256 `1ce94d634545b4f8d5e88fdc1ad48622f1b9dac345761bc1f88dce3496c89b51`.

This supplied-state gate requires CF set from the +087B fallthrough
through +087D LES and verifies IP and all three loaded bytes at CS:+0880.
It reads one byte at the 20-bit wrapped ES:DI physical address, replaces
only AL, and stops at +0883 before `30 E4` XOR AH,AH. It preserves
AH, all other CPU words, all FLAGS and all 1 MiB RAM bytes.
Unsupported inputs reject without mutation.

The reviewed +0871 ES byte-read model at
`bf045b08a8c7f43624a9021960e6f541e1fd9540` supplied the read
pattern. Base: `c0096a98ce724b21b07adfb61426e4d92da122b3`.
The required `deja` lookup timed out under concurrent index load;
no indexed result was used. Five synthetic full-state frames compare
all fourteen CPU words and every RAM byte, covering ES/DS separation,
A20 wrap and preserved FLAGS. Wrong loaded bytes, wrong IP, CF-clear
input, null pointers and short RAM reject without mutation.
No natural reach, native frame, full function closure or strict full-C
count is claimed.
