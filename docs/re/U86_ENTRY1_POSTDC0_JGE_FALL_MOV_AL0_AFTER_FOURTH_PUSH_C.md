# U86 entry 1: MOV AL,0 after fourth PUSH AX

This one-instruction local model starts at U86 `+015E` with independently
supplied state after the fourth `PUSH AX`. It requires SF != OF and four
SS stack words: current AX at SP, then AH:26h, AH:03h, and AH:01h at
SP+2, SP+4, and SP+6. `MOV AL,0` clears only AL and stops at `+0160`
before the next `PUSH AX`. AH, SP, flags, RAM, and all other CPU fields
are preserved. The preceding local byte is not re-read: the preceding
push may have overwritten its address if it aliased the push destination.

The ignored original `GAME.OVR` is 206105 bytes with SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U86 starts at file offset `0x27018`, spans `0x37B5` code bytes, and entry 1
is `+008A`. MOV `+015E..+0160` is file `[0x27176,0x27178)` with SHA-256
`46cc6a8cb5bcc0120f98b84c71699b2ac5b90cb34a781ae2a83305fa98cd8e67`:

```asm
+015D  push ax
+015E  mov al, 0
+0160  push ax
```

The static checker pins predecessor, MOV, and successor bytes and verifies
that no loader fixup touches them. Reconstructed loaded bytes therefore
match original bytes; no native loaded capture was made. Eight synthetic
cases cover all four stack words, SP offset and A20 wrap, full CPU state,
and all 1 MiB RAM. Wrong IP, flags, RAM size, or a corrupted stack word
reject without mutation. Bounded `deja` failed at its read-only local
index lock; the reviewed local fourth-PUSH predecessor at
`1a1c37a03b847c4d5a99ee8268a4e40bb5e71976` and independent byte
decode establish this boundary. This is source and supplied-state
evidence only, without native parity or full-C equivalence.

Verification: strict C11 `-Wall -Wextra -Werror -pedantic` 8/8 PASS;
ASan/UBSan 8/8 PASS (`detect_leaks=0` under the traced shell); static
original/loaded byte and ownership check PASS. No shared CMake changes.
