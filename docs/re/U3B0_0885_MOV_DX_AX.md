# U3B0 +0885: bounded MOV DX,AX successor

Original DOS v1.3 GAME.OVR U3B0 +0885 bytes are `8B D0`,
MOV DX,AX. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D154,0x2D156)`
has SHA-256 `646806b9f5f977d0d079c0450de6b2810d4bb4c647dd287306b9eaf3a4b05573`.

This supplied-state gate follows the +0883 XOR AH,AH fallthrough.
It verifies IP, both loaded code bytes, cleared AH and the defined
XOR-result CF/PF/ZF/SF/OF bits, copies AX to DX and stops at +0887 before
`8B 7E 06` MOV DI,[BP+6]. All other CPU words, every FLAGS bit and
all 1 MiB RAM bytes are preserved. Unsupported inputs reject without
mutation.

The +0883 reviewed gate at
`5dd4bcf4b1ad334abfd3ac63624585a37fbcc810` supplies the defined
flag condition. Its architecturally undefined AF remains an explicit
local placeholder; this gate does not establish original same-frame
parity for AF or complete FLAGS. Base:
`1d3ffc8be575923eea5e54da2375f56dda0c472f`.
The required `deja` query found no indexed match.

Six synthetic full-state frames compare all fourteen CPU words and
every RAM byte, including supplied AF set and clear. Wrong loaded
bytes, IP, nonzero AH, defined flags, null pointers and short RAM reject without
mutation. No natural reach, native frame, full function closure or
strict full-C count is claimed.
