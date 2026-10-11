# Full C translations: unit 0x38 entries 81 and 110 (issue #81)

`src/recovered/overlay_unit38_walk.c` translates DOS v1.3 `GAME.OVR` unit
`0x38` public entries 81 (`+0x268A`) and 110 (`+0x2E37`). Both are
call-free far Pascal procedures with a proven entry (public entry slot and
installer store) and exactly one reachable `retf 0xA`. Entry 81 has an
inherited-stack path that is represented explicitly; entry 110's two loops
are bounded as stated below, with the one unbounded case (a cyclic list)
named rather than hidden. ECL issue #75 and resident files are untouched.

`deja "unit 0x38 entry 81 110 +0x268A +0x2E37 loop"` matched only this
worker's PR #63/#69/#77 sessions. Reused with citation: the dispatcher and
argument frame of [OVERLAY_UNIT38_EFFECT_ENTRIES.md](OVERLAY_UNIT38_EFFECT_ENTRIES.md)
(far pointer A at `[bp+0xC]` is a 285-byte character record), the entry 75
bracket analysis of [OVERLAY_UNIT38_EFFECT_ENTRIES_NEXT.md](OVERLAY_UNIT38_EFFECT_ENTRIES_NEXT.md),
and the in-memory far-pointer slots `0x7F..0x82`, `0xC8..0xCB`,
`0x104..0x10B` of [DOS_CHARACTER.md](../formats/DOS_CHARACTER.md). Each was
re-checked from the bytes here (installer stores, byte-identity of the
bracket body, slot use).

## Provenance (ignored local input; hashes only)

`GAME.OVR`, 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; unit
`0x38` at file offset 60,507 (`0x0EC5B`), stub segment `0x41`, entry *j* at
stub `0x41:(0x20 + 5·j)`. Capstone 5.0.7, 16-bit mode, decoded from each
entry offset to the next ledger prologue; every jump target lies inside its
routine; neither routine has trailing bytes after its `retf`.

| Entry | Unit offset | File offset | Bytes | Insns | Edges | Item | Installer store | SHA-256 of the routine's bytes |
| ---: | --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| 81 | +0x268A | 0x112E5 | 118 | 48 | 16 | 97 | `[0x69AC]`/`[0x69AE]` at +0x3678/+0x3681 | `62ce886a4ee5f8caa06d31f6f5767f4144a1442db79138135311118fb60055f1` |
| 110 | +0x2E37 | 0x11A92 | 163 | 52 | 9 (2 backward) | 126 | `[0x6A20]`/`[0x6A22]` at +0x37F1/+0x37FA | `403d92e71f8aa0d52bbe6133fb0a9a8b83195a5b6285b83b92672e716ee3b1d4` |

The issue's leads (118/48 and 163/52) match the measured spans exactly.
Public classification: grade A, `far_in` 0 for both, so the dispatch table
is the only identified route. Neither reads far pointer B or the byte
argument; all decoded dispatcher call sites push constant indices other than
97 and 126 or an object byte, so both are reached only by computed index.

## Entry 81 (+0x268A): entry 75's bracket body behind a two-value gate

`sub sp,2; cmp byte [0x682A],4; je body; cmp byte [0x682A],2; jne done;
body: les di,[bp+0xC]; mov al,es:[di+0x14]; cmp al,4; jb n1; cmp al,6; ja
n1; mov byte [bp-1],1; jmp add; n1: cmp al,7; jb n2; cmp al,0xA; ja n2; mov
byte [bp-1],2; jmp add; n2: cmp al,0xB; jb n3; cmp al,0xD; ja n3; mov byte
[bp-1],3; jmp add; n3: cmp al,0xE; jb n4; cmp al,0x11; ja n4; mov byte
[bp-1],4; jmp add; n4: cmp al,0x12; jb add; cmp al,0x14; ja add; mov byte
[bp-1],5; add: mov al,[bp-1]; xor ah,ah; mov dx,ax; mov al,[0x6816]; xor
ah,ah; add ax,dx; mov [0x6816],al; done: mov sp,bp; pop bp; retf 0xA`.

Edges: +0x2695 `je`→+0x269E, +0x269C `jne`→+0x26FA, then the five `jb`/`ja`
pairs and four `jmp`s at +0x26A7/+0x26AB/+0x26B1, +0x26B5/+0x26B9/+0x26BF,
+0x26C3/+0x26C7/+0x26CD, +0x26D1/+0x26D5/+0x26DB, +0x26DF/+0x26E3.

**Byte identity.** The 98 bytes from `les di,[bp+0xC]` at +0x269E through
the `retf 0xA` ending at +0x26FF are identical to the 98 bytes of the
reviewed entry 75 from +0x24E6 through +0x2547 (entry 75 relative +13
through +110); the two routines differ only in the gate (entry 75: `cmp byte [0x682A],0; jne done`,
7 bytes; entry 81: the two compares above, 14 bytes). The other two readers
of `[0x682A]` in the unit, entries 95 (+0x2A01) and 109 (+0x2E27), both
compare it with 0; its only writer is unit 0x9e entry 7 (+0x0DAD, from an
argument byte).

**Paths and side effects.** Gate closed (`[0x682A]` neither 4 nor 2):
nothing changes. Gate open: `v = record[0x14]`; brackets 4..6 → 1, 7..10 →
2, 11..13 → 3, 14..17 → 4, 18..20 → 5 (unsigned); `[0x6816] += bracket`
(16-bit add of zero-extended bytes, low byte stored, i.e. modulo 256).

**Inherited stack byte, stated explicitly.** For `v < 4` or `v > 20` no
store to `[bp-1]` precedes the `mov al,[bp-1]` at +0x26E9. The byte added is
whatever the stack held one byte below the saved `bp` when the routine was
entered: content left by earlier calls on the same stack, not determinable
statically. The translation takes it as the `stack_residue` argument, adds
it exactly as the original would, and flags the path with
`PR_U38_PATH_SECOND`. No value is invented. A DOS oracle trace (issue #3)
would show whether callers reach this path and what the residue is.

## Entry 110 (+0x2E37): walk a linked list from the record behind slot 0x108

`sub sp,6; les di,[bp+0xC]; les di,es:[di+0x108]; mov ax,es:[di+0xA]; mov
dx,es:[di+0xC]; mov [0x6826],ax; mov [0x6828],dx; les di,[0x6826]; mov
ax,es:[di+0xC8]; mov dx,es:[di+0xCA]; mov [bp-4],ax; mov [bp-2],dx;
outer: mov ax,[bp-4]; or ax,[bp-2]; je done; cmp byte [0x681E],0; jne done;
les di,[bp-4]; cmp byte es:[di+0x34],0; je next; mov byte [bp-5],1; jmp
body; step: inc byte [bp-5]; body: mov al,[bp-5]; xor ah,ah; les di,[bp-4];
add di,ax; cmp byte es:[di+0x2E],0x98; jne alt; les di,[bp-4]; cmp byte
es:[di+0x31],0xFC; je set; alt: mov al,[bp-5]; xor ah,ah; les di,[bp-4];
add di,ax; cmp byte es:[di+0x2E],0x76; jne after; set: mov byte
[0x681E],1; after: cmp byte [bp-5],3; jne step; next: les di,[bp-4]; mov
ax,es:[di+0x2A]; mov dx,es:[di+0x2C]; mov [bp-4],ax; mov [bp-2],dx; jmp
outer; done: mov sp,bp; pop bp; retf 0xA`.

**Edges, all nine.** +0x2E6E `je`→+0x2ED4 (null pointer exit), +0x2E75
`jne`→+0x2ED4 (flag exit), +0x2E7F `je`→+0x2EC1 (skip inner loop when
`node[0x34] == 0`), +0x2E85 `jmp`→+0x2E8A (enter the body with i = 1),
+0x2E99 `jne`→+0x2EA5 (probe is not 0x98), +0x2EA3 `je`→+0x2EB6 (0x98 and
`node[0x31] == 0xFC`: set), +0x2EB4 `jne`→+0x2EBB (probe is not 0x76),
+0x2EBF `jne`→+0x2E87 **backward** (i != 3: increment and repeat), +0x2ED2
`jmp`→+0x2E68 **backward** (next node).

**Pointer and record access order.** (1) `P` = far pointer at
`record[0x108..0x10B]`; (2) `Q` = far pointer at `P[0xA..0xD]`, stored to
`[0x6826]/[0x6828]`; (3) head = far pointer at `Q[0xC8..0xCB]`. `Q` is read
at a documented far-pointer slot of the 285-byte character record
(`0xC8..0xCB`), the same chain that the merged entries 4 and 6 follow to
byte `0x9F` of `Q`. Per node: the null test on the pointer words, the flag
test, `node[0x34]`, then for i = 1, 2, 3 the byte `node[0x2E+i]` (that is
`0x2F`, `0x30`, `0x31`) and, when it is 0x98, `node[0x31]`; finally the link
at `node[0x2A..0x2D]`. Bytes `0x2E` and `0x32` are never examined. The list
shape (head at record `+0xC8`, link at node `+0x2A`) is the same one noted
for unit 0x25 entry 21 in [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) §2.

**Loop termination.** The inner loop is exact: `[bp-5]` starts at 1, the
exit compares it with 3 after each body, so the body runs for i = 1, 2, 3
and never more, and a match does not leave the inner loop early (the flag
is simply set, idempotently). The outer loop exits when the current pointer
is `0:0` (tested first) or when `[0x681E]` is nonzero (tested second, so a
match on node k is observed at the top of the iteration for node k+1, after
k's link has been read). The original has no other bound: on a cyclic list
with no matching enabled node it never returns. The translation adds an
explicit `node_limit` and reports `PR_U38_WALK_LIMIT` when it is reached, a
result the original cannot produce except by hanging. A node count and a
"matched" flag are reported for tests.

**Side effects.** `[0x6826]/[0x6828]` = `Q` (always, before any test);
`[0x681E] = 1` when a match is found; nothing else, and the record and nodes
are never written.

## C contract and tests

`pr_u38w_state` holds `[0x6816]`, `[0x681E]`, `[0x682A]` and the scratch
far pointer. Far pointers are resolved through the shared callback with the
byte counts the routine touches (14 for `P`, 0xCC for `Q`, 0x35 per node).
Entry 81 returns the shared `pr_u38_status`; entry 110 returns
`pr_u38_walk_status` with the four exits `END`, `FLAG`, `ARGUMENT`, `LIMIT`.

`tests/recovered/test_overlay_unit38_walk.c`: entry 81 over all 256 gate
values (only 2 and 4 open it), all 256 record values × three residues under
each open gate value, thirteen bracket boundaries with the byte add wrapping
from 0xFE, record untouched, null contracts. Entry 110: null head (END, no
node read, scratch written, null test precedes the flag test); flag preset
on a non-null head (FLAG, no node read); disabled node skipped; P/Q/node
pointer failures with the exact scratch state; exhaustive probe byte at each
of the three examined offsets against four `[0x31]` values (match iff 0x76,
or 0x98 with `[0x31] == 0xFC`); offsets `0x2E` and `0x32` ignored; a match
on the second of three nodes exits by the flag after reading the second
node's link and before reading the third (resolver trace checked); three
nodes without a match reach END; a two-node cycle without a match hits the
limit, and with a match exits by the flag; a limit equal to the chain length
still reaches END.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds the
library and test with no diagnostics and prints `overlay unit 0x38
walk-entry tests passed`; CMake build has no warnings; `ctest` passes 22 of
22 targets on main 8f84b89 (twenty-one existing plus
`poolrad_overlay_unit38_walk_tests`).

## Limitations, stated plainly

- Meanings of `[0x6816]`, `[0x681E]`, `[0x682A]`, record byte `0x14`, the
  list nodes' bytes `0x2F..0x31` and `0x34`, and the compared constants
  0x98, 0xFC, 0x76 are not established; the game role of both routines is
  not asserted.
- Entry 81's out-of-range path depends on inherited stack content; it is
  represented, not resolved.
- Entry 110 on a cyclic list is non-terminating in the original; the
  translation's `node_limit` is a guard, not original behavior.
- The gameplay conditions that compute dispatch indices 97 and 126 are not
  established.
