# Unit 0x6f entry 6 (+0x0EC4, `0x7C:0x3E`): bounded model of the whole path

Builds on `U6F_ENTRY6_0EC4_MAP.md` (static first-path map). GAME.OVR (206,105 bytes, SHA-256 `872964a3...7928614`), unit 0x6f base 0x1DFE6; the routine is file 0x1EEAA..0x1F247, 926
bytes, 341 instructions, SHA-256 `1160665fef6397a88b922ce115836e5cf4b55f09b5c4db04e765e8a02f08f747` (re-verified). No original bytes are reproduced here.

## Exact order (this note adds the whole decoded path, the earlier map described its shape)

Frame 0x11A; `[bp+6]` far object, `[bp+0xA]` id word (AL only). Five unit string constants (Pascal; length and hash prefix only): `cs:0x0E98` len 3 `403e928a`, `0x0E9C` len 3 `a68c46ae`,
`0x0EA0` len 4 `a374cb55` (used as the extension argument), `0x0EBC` len 3 `dc2b108f`, `0x0EC0` len 3 `7d4aaad1`; `0x0EA5` len 22 `7487c754` (error message).

1. Name 1 = StrLoad(`0E98`) + Str(`[0x5376]`, width 0, string[1]) via `0xAF8:0x1366` + StrCat(`0E9C`); ext = StrLoad(`0EA0`). `0x802:0x0716(name, ext, id, &size, &ptr)` (resident ABI as in `RESIDENT_ECL_DAX_BLOCK.md`: `[bp+0x14]` name, `[bp+0x10]` ext, `[bp+0xE]` id,
   `[bp+0xA]` out size, `[bp+6]` out pointer, `retf 0x12`). The two retained string pointers from StrLoad/StrCat are the first two arguments.
2. size = 0: copy the 22-byte message, `0x709:0x0BF5` (display), `0x899:0` (DOS terminate, from the earlier U3BD note): the model returns `ERROR_EXIT` before the display call.
3. Else `0xAF8:0x1685(block, object, 0x11D)`. That runtime routine (decoded here) is an overlap-safe move: it copies backward when the source offset is below the destination offset, forward otherwise. Then clear the word pairs
   `+0xC8/+0xCA`, `+0x7F/+0x81`, `+0x104/+0x106`, `+0x108/+0x10A`.
4. Class byte: load `+0x2E`; {1,3,5} -> `[+0xC0] := 1`, {2,4,6,7} -> `[+0xC0] := 2`, otherwise `+0xC0` keeps the loaded byte.
5. Five rolls `0xB0:0x48(1, 6)`; each stores the low byte of `((r + 8) << 4) + r` at `[+0xC0 + i]` for i = 1, 2, 3, then `[+0xC4] := 0xC4`, then i = 5, 6.
6. `FreeMem(block, size)`; DS `[0x557C] := 1`.
7. Block 2: name suffix `0EBC`. size 0 skips. Otherwise `p := GetMem(9)` stored at `object[+0x7F/+0x81]`; do { Move(block + off, p, 9); p.link(+5/+7) := nil; off += 9; if off >= size stop;
   q := GetMem(9); p.link := q; p := q } while off < size; `FreeMem(block, size)`. The last unit always reads a full 9 bytes (possibly past the block's size).
8. Block 3: suffix `0EC0`; same with 0x3F-byte units, the head at `object[+0xC8/+0xCA]`, links at `+0x2A/+0x2C`.
9. DS `[0x557C] := 0`; `0x899:0x0104` (key-flush loop); `retf 6`.

## Model

`pr_u6f_entry6` (`overlay_u6f_entry6_0ec4.{h,c}`). Explicit hooks: `0xAF8:0x1366`, `0x802:0x0716`, GetMem, FreeMem, `0xB0:0x48`, `0x899:0x0104`. The block reader hook has the signature of the
already merged bounded model `pr_rec_res_0802_0716` (issue #74, `RESIDENT_ECL_DAX_BLOCK.md`): a host adapter binds it by converting the two Pascal strings to C strings, calling that function, and
placing the returned block in the object table; this PR does not duplicate or edit the reader (coordinated with the resident-loader owner). That adapter is not part of this PR. The file-open helper (image `0x085ED`), Pascal I/O and
heap semantics remain unresolved boundaries inside that reader. Objects go through an explicit host table; an unmapped pointer is `UNMAPPED`, a short view `VIEW`, a unit loop whose 16-bit offset would wrap `RANGE`. Earlier effects
stand on every stop. The model invents no file or block content.

## Saved slot-A inputs

The id is slot value(1) of LOAD MONSTER; in the supplied-state route it is the plain class-1 address `0x6E80` value (the prefix note), and `[0x5376]` is a runtime DS byte not in the save. The three block files'
contents are not read here, so no real record can be produced and no real block size is claimed.

## Claims

Bounded model of the whole routine with hooks at every file/heap/random/display effect; synthetic tests only (`tests/recovered/test_overlay_u6f_entry6_0ec4.c`). No strict full-C, count, identity, DOS-native or one-Up parity claim.
