# U3B0 +088F: bounded ADD AX,DX

Original DOS v1.3 GAME.OVR U3B0 +088F bytes are `03 C2`, ADD AX,DX.
The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D15E,0x2D160)` has
SHA-256 `dc98dcbca7373c462b8ddafeaf7590bc8549185d60dd109e82f5573804afb6e5`.
Execution stops at +0891 before the `50` PUSH AX.

This supplied-state gate checks the +088E CBW successor: AX must be the
sign extension of AL and the defined XOR flags from +0883 must equal
`0x0044`. The predecessor's AF is undefined and accepted in either
state. It verifies both loaded code bytes, adds DX to AX, and updates
OF/SF/ZF/AF/PF/CF from the 16-bit result. Other FLAGS bits, the other
CPU words, and all 1 MiB RAM bytes are preserved. Unsupported inputs
reject before mutation.

Eight synthetic full-state frames compare all fourteen CPU words and
every RAM byte. Defined result flags were measured independently with
host x86 `addw`/`pushfq` for these operand pairs, including carry,
signed overflow, auxiliary carry, sign, zero, and parity. Wrong loaded
bytes, non-CBW AX, wrong predecessor flags/IP, null pointers, and short
RAM reject without mutation. The required `deja` lead query was attempted
but timed out; no prior-session result is reused. The +088E predecessor
contract is from `U3B0_088E_CBW.md` at reviewed local head `f4b5b1c`.

The opcode span is measured from the original image; CPU/RAM frames are
supplied-state synthetic. Host arithmetic instruction checks flags only:
they are not a native reached GAME.OVR frame. No natural reach, full
function closure, or strict full-C count is claimed.
