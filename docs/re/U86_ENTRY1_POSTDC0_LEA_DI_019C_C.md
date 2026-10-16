# U86 entry 1: source-only +019C LEA DI

PM inbox i7974 assigned this bounded taken successor after the merged +01ED
JZ gate. The read-only ownership replies i7964/i7965/i7969 cleared pR, p5,
and p9 at the immediate instruction. Earlier `deja "019C"` found pR's prior
branch discussion in sessions `01a13c73-c6d` and `01a1281b-85e`; a fresh
`deja "U86 019C LEA DI BP 32"` found no exact earlier implementation. The
merged +01ED JZ source supplies the branch address and shared CPU convention,
not native reachability.

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U86 begins at overlay file `0x27018`, code size `0x37B5`, entry 1 `+008A`.
The original `+019C` bytes at file `[0x271B4,0x271B7)` are `8D 7E CE`,
SHA-256 `5a767b97c2cf4c7fa8157c8ba3f23d55e9e26b3da05163f8adcef919972d23dc`.
The next `+019F PUSH SS` byte is `16`, SHA-256
`7cb7c4547cf2653590d7a9ace60cc623d25148adfbc88a89aeb0ef88da7839ba`.
No loader fixup touches either span, so reconstructed loaded bytes equal
original bytes. There is no direct native loaded capture.

`pr_u86_entry1_019c_lea_di` requires supplied loaded U86 CS, IP +019C,
exact loaded `8D 7E CE`, and 1 MiB RAM. It computes DI = BP - 32h modulo
16 bits and stops at +019F before `PUSH SS`. LEA does not read the addressed
memory or change flags; every other CPU word and all RAM remain unchanged.
Unsupported inputs reject atomically. This gate does not establish the zero
frame words or other conditions required by the separate +019F PUSH SS model.

Strict C11, ASan/UBSan, and synthetic full CPU/RAM tests cover BP wrap,
A20-wrapped loaded code, wrong CS/IP/opcode, size, and null rejection. The
static audit checks original bytes, hashes, and fixup exclusion. This is a
supplied-state source slice; no native, whole-function, strict/full-C, or
game-function credit is claimed.
