# ECL LOAD MONSTER, unit 0x1c +0x0537..+0x0816 (tail: party link, clones, sidecar chains, epilogue)

Continues `U1C_LOAD_MONSTER_04AF.md`. GAME.OVR (206,105 bytes, SHA-256 `872964a3...7928614`), unit 0x1c file base 0x0D46. The tail is file 0x127D..0x155C, 736
bytes, 222 instructions, SHA-256 `a31517ef9c75925d057e7140d269e7f68700c4c57672e7b410f6466b09149f39`. No original bytes are reproduced here.

Frame (from the earlier slices): `[bp-0x10]` = A (the object the entry-5 wrapper filled), `[bp-0x142]`/`[bp-0x14E]` = list heads B/C (iterators here), `[bp-0x14A]`/`[bp-0x156]` =
their saved copies, `[bp-0x14]` = the saved active-character pointer, `[bp-8]` = cur, `[bp-4]` = n (set to 1 at `+0x0464`), `[bp-2]` = count, `[bp-0x131]` = the 0x11D-byte local copy of A taken at `+0x04AF`.

| Offset | Effect |
|---|---|
| +0x0537..+0x0559 | cur := far [0x5D96]:[0x5D98]; while cur.link (words at +0x104/+0x106) is non-nil, cur := cur.link: walk to the party list **tail** |
| +0x055B..+0x059F | tail.link := A; cur := A (read back); A[+0xBF] := [0x6F83]; A.link := nil; [0x4435]++; n++ (A itself is the first monster, not cloned) |
| +0x05A2..+0x05B4 | exit to the epilogue if n > count (unsigned byte) or [0x4435] >= 0x3F |
| +0x05B7..+0x063A | clone: P := GetMem(0x11D) (0xAF8:0x023F); cur.link := P; cur := P; Move(local copy, P, 0x11D); P[+0xBF] := [0x6F83]; P.link := nil; P[+0x7F..+0x82] := 0; P[+0xC8..+0xCB] := 0; n++; [0x4435]++ |
| +0x063E..+0x070D | if the saved B head is non-nil: for each source node I (link +0x2A/+0x2C): M := GetMem(0x3F); cur[+0xC8] := M; Move(I, M, 0x3F); M.link := the previous cur[+0xC8] pair (nil for the first node). The new nodes are prepended: the chain is the reverse of the source |
| +0x0710..+0x07EA | the same for the saved C head with GetMem(9), the pair at cur[+0x7F], the node link at +5/+7, Move(I, M, 9) |
| +0x07EA..+0x07FA | reset the iterators from the saved heads and loop to +0x05A2 |
| +0x07FD..+0x0816 | [0x6F83]++; [0x84E7] := 1; [0x5D92]/[0x5D94] := the saved pair; `retf` |

The clone's chain words are cleared before the sidecars are rebuilt from the original heads B and C, so every clone owns private copies of both chains; A keeps the chains the
entry-5 wrapper built (it is the original). The B chain (63-byte nodes) and C chain (9-byte nodes) sizes match the earlier map.

## Model

`pr_u1c_load_monster_tail` (`overlay_u1c_load_monster_tail_0537.{h,c}`) applies the slice to an explicit DS struct (`[0x4435]`, `[0x6F83]`, `[0x84E7]`, `[0x5D92]/[0x5D94]`), reads `[0x5D96]/[0x5D98]`
from the environment, and takes the frame values left by the earlier slices (A, saved B/C, count, n, the 0x11D local copy, the saved pointer). Every dereferenced far pointer is looked up
in a host object table (same convention as the entry-5 and 04AF models: exact far-pointer match, byte addresses wrap at 16 bits). Stops with earlier effects standing: `UNMAPPED` (including a nil
party head, which the original would read from low memory), `VIEW`, `CYCLE` (walk cap 4096 per list), `HOOK` (GetMem not bound), `CALLBACK` (GetMem returned nonzero). GetMem (heap allocator 0xAF8:0x0308) is the only hook;
nothing about object contents, the party or later engine behaviour is invented.

## Real saved slot-A route (private probe, none of the host values come from DOS)

With the saved slot-A script and the six recorded CHRDATA records as party nodes (host-assigned links, GetMem handing out host objects, B and C heads nil): count = 6 (slot value(2)), the walk steps over 5 links to the
tail, A is linked and 5 clones follow (5 GetMem(0x11D) calls), [0x4435] 0 -> 6, [0x6F83] 8 -> 9, [0x84E7] = 1. The 0x3E9:0x34 hook of the previous slice is bound to a fabricated callback.
Whether the real wrapper's B/C chains are non-nil (they decide the sidecar GetMem calls) is the `0x7C:0x3E` unresolved boundary.

## Remaining unresolved dependencies

`0x7C:0x3E` (unit 0x6f entry 6) record content and B/C chains, `0x464:0x0111`/`0x802:0x0716` (reached through `0x3E9:0x34`), and the heap allocator `0xAF8:0x0308` behind GetMem.

## Claims

Bounded model of a slice; synthetic tests only (`tests/recovered/test_overlay_u1c_load_monster_tail_0537.c`). With 0x450 prefix, 0x04AF and this tail every instruction of the routine is covered
by a bounded model, but the boundaries above are hooks: no strict full-C, count, identity, DOS-native or one-Up parity claim.
