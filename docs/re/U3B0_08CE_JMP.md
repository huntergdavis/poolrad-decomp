# U3B0 +08CE: bounded near jump

Original DOS v1.3 GAME.OVR U3B0 +08CE bytes are `E9 3B FF`,
a signed 16-bit relative JMP from +08D1 to +080C. The ignored
206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D19D,0x2D1A0)` has
SHA-256 `854416135301f2de96d09af10d361026aa5d612385556d32b638cb1d9ccab2d6`.
The target begins `8B 46 FC`; target instruction execution is excluded.

This supplied-state gate accepts the +08CB INC successor with CF clear,
verifies IP and all three loaded code bytes, then sets only IP to +080C.
All thirteen other CPU words and all 1 MiB RAM bytes are preserved.
Unsupported inputs reject without mutation.

Six synthetic full-state cases compare all fourteen CPU words and
every RAM byte across distinct CS values, including 20-bit code fetch
wrap, flags, stack registers, and segment registers. Wrong loaded
bytes, CF-set input, wrong IP, null pointers, and short RAM reject
atomically. The required `deja` lead query was attempted but timed out;
no prior-session result is reused. The +08CB predecessor contract is
from `U3B0_08CB_SS_INC.md` at reviewed local head `a66565d`.

The original code bytes and jump displacement are measured. CPU/RAM
frames are supplied-state synthetic; no native reached GAME.OVR frame,
natural reach, full function closure, or strict full-C count is claimed.
