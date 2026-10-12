# Unit 0x1c +0x125D: ECL CLEARMONSTERS (0x1C), bounded model with a heap boundary

Opcode `0x1C` (CLEARMONSTERS, reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), 0 operands) dispatches to unit `0x1c`
`+0x125D..+0x12C5`, file `0x1FA3..0x200B` (unit base `0x0D46`): **105 bytes, 36 instructions**, ending in the `retf` at `+0x12C5`, six bytes of locals.
SHA-256 `8112f2f70d8905153070bfe84d60f102d3a7e1e2aaa89e26c81fd79b10f4479d` (GAME.OVR, 206,105 bytes, `872964a3…7928614`; matches the PM). The next routine
is `+0x12C6`. `deja` timed out (PM's and mine) with no hit. **Classification: bounded model** with the Pascal heap manager as an explicit
host boundary; no strict full-C, count, identity, DOS/native or one-Up parity claim.

## Original order (all instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x1263` | `inc word [0x49ED]` (the PC) |
| `+0x1267..+0x1271` | `[0x4435] := 0`; `[0x84E7] := 0`; `[0x6F83] := 8` |
| `+0x1276..+0x1282` | `FillChar(DS:0x67F4, 0x1C, 0)` via `0xAF8:0x16A8` |
| `+0x1287..+0x128E` | `or` of the two head words `[0x6810]`, `[0x6812]`; zero ends the loop |
| `+0x1290..+0x129F` | `link := word es:[head+0x2A], word es:[head+0x2C]` (read **before** the free) |
| `+0x12A2..+0x12AE` | `FreeMem(head, 0x3F)` via `0xAF8:0x0254`: pushes the segment word, the offset word, then the size `0x3F` |
| `+0x12B3..+0x12C0` | `head := link`; loop; at the end `mov sp,bp; pop bp; retf` |

Resident routines (decoded from the START.EXE expanded image at `0xAF80 + off`): `0xAF8:0x16A8` (image `0xC628`, 32 bytes, `retf 8`) is `les di,[far]; mov cx,[count];
mov al,[value]; cld; rep stosb`, the Pascal `FillChar`, translated exactly: the 28 bytes at `DS:0x67F4..0x680F` become 0 and **the head words at `0x6810..0x6813` are
not part of the block**. `0xAF8:0x0254` (image `0xB1D4`, 92 bytes) is `FreeMem`: it calls the heap-manager routine at `0xAF8:0x03D3` and treats carry as runtime error 204
(`mov ax,0xCC`); the heap manager is not decoded and is the model's explicit boundary.

## Model

`pr_u1c_op_clearmonsters` (`overlay_u1c_clearmonsters_125d.{h,c}`): the PC advances first, the three DS bytes are set and the 28-byte DS block (host state) is zeroed, then each
list node (explicit host state: view, far offset, next link at `+0x2A/+0x2C`; a view too small for the four link bytes or a cyclic list is a host guard, LIST status) is passed to the
fallible hook `free_af8_0254(node, 0x3F)` in list order, the link having been read first; the head advances only after a successful free (missing hook: HOOK status;
nonzero return: CALLBACK status; the earlier frees, the zero fill and the DS bytes stand). No heap parity is claimed.

Tests (`test_overlay_u1c_clearmonsters_125d.c`) are fully synthetic: lists of 0..6 nodes (free order, size argument, head left nil, the zero fill of exactly 28 bytes,
the three DS bytes, the PC advance and wrap, nodes unmodified), a failing hook mid-list, a missing hook, the link-view guards (including a 16-bit offset wrap) and a cycle.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound (through HORIZONTAL MENU);
explicit host inputs, none from DOS: `[0x84E2] = 0`, PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result 0..2, SURPRISE rolls 1..6, the `0x2B:0x3E` result in {0, 1, 5}, the
four code constants read privately, counting menu/display/free hooks and menu result 0..3. The monster list is **host state** (the saved route builds it with earlier monster
opcodes the model does not yet run): tried empty and with one node.

At `0xB1AF` CLEARMONSTERS runs (zero or one free), `[0x6F83] = 8`, `[0x4435] = [0x84E7] = 0`. **Next stop** (checked on the supplied combinations, 288 runs reach it): UNMODELLED at PC `0xB1B0`,
opcode `0x0B` (LOAD MONSTER, handler `+0x0450`, 3 operands), previous opcode `0x1C`. Supplied-state probe only: no live DOS RNG, native search reachability or one-Up parity.
