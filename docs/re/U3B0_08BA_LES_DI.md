# U3B0 +08BA: bounded taken-route LES copy

Original DOS v1.3 GAME.OVR U3B0 +08BA bytes are `C4 7E FC`,
LES DI,SS:[BP-4]. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D189,0x2D18C)`
has SHA-256 `4213bce6e9b7f566950c84818f8b9c3bf6b34a12c19a6ee084b609ea4e81426e`.

This supplied-state gate requires CF clear, the +087B JAE taken
condition, and verifies IP and all three loaded code bytes. It reads
four pointer bytes at separately wrapped SS:[BP-4..BP-1] offsets and
20-bit physical addresses, loads DI and ES, and stops at +08BD before
the ES-prefixed word read `26 8B 45 2A`. All other CPU words, all FLAGS
and all 1 MiB RAM bytes are preserved. Unsupported inputs reject without
mutation. The identical +087D LES bytes are a distinct branch target.

The reviewed +087D LES source at
`9d6b315610e9880b6bcf976a079a857f826f3985` supplied the pointer
read pattern. Base: `c82c3d049d016361e1b95490e81bdbf3a07adb24`.
The required `deja` lookup timed out under concurrent index load;
no indexed result was used. Five synthetic full-state frames compare
all fourteen CPU words and every RAM byte, including SS/DS separation,
offset wrap and A20 wrap. Wrong code bytes, wrong IP, CF-set input,
null pointers and short RAM reject without mutation.
No natural reach, native frame, full function closure or strict full-C
count is claimed.
