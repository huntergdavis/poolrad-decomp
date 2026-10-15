# U86 entry 1: joined PUSH SS after LEA DI

This one-instruction local model starts at U86 `+019F` with independently
supplied state after `LEA DI,[BP-32h]`. It requires AX=0, DI=BP-32h
modulo 16 bits, the defined logical flags of the joined path, and zero
words at SS:[BP-5] and SS:[BP-3]. `PUSH SS` decrements SP by two modulo
16 bits, then writes SS little endian to SS:SP with bytewise 16-bit
offset and 20-bit physical wrap. It stops at `+01A0` before `PUSH DI`.
The frame is validated before the write, so the new push may overlap it.
All CPU fields except SP/IP and all RAM outside the pushed word are
preserved.

The ignored original `GAME.OVR` is 206105 bytes with SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U86 starts at file offset `0x27018`, spans `0x37B5` code bytes, and entry 1
is `+008A`. PUSH `+019F..+01A0` is file `[0x271B7,0x271B8)` with SHA-256
`7cb7c4547cf2653590d7a9ace60cc623d25148adfbc88a89aeb0ef88da7839ba`:

```asm
+019C  lea di, [bp-32h]
+019F  push ss
+01A0  push di
```

The static checker pins predecessor, PUSH, and successor bytes and verifies
that no loader fixup touches them. Reconstructed loaded bytes therefore
match original bytes; no native loaded capture was made. Eight synthetic
cases cover the two frame words, push overlap, SP underflow, offset and A20
wrap, full CPU state, and all 1 MiB RAM. Invalid IP, DI, AX, flags, RAM
size, or frame words reject without mutation. Bounded `deja` failed at
its read-only local index lock; the reviewed local LEA predecessor at
`786caaad3e86b60fb003445d40da43b251055251` and independent byte
decode establish this boundary. This is source and supplied-state
evidence only, without native parity or full-C equivalence.

Verification: strict C11 `-Wall -Wextra -Werror -pedantic` 8/8 PASS;
ASan/UBSan 8/8 PASS (`detect_leaks=0` under the traced shell); static
original/loaded byte and ownership check PASS. No shared CMake changes.
