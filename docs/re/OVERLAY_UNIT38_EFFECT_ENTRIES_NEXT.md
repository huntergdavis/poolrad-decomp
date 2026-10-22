# Full C translations: unit 0x38 entries 6, 75, 82 and 97 (issue #67)

`src/recovered/overlay_unit38_effects_next.c` translates four more bounded,
call-free DOS v1.3 `GAME.OVR` unit `0x38` public entries. Each has a proven
entry (public entry slot plus the installer's table store), exactly one
reachable `retf 0xA`, no calls, no indirect branches, and every branch and
side effect reproduced in C with synthetic tests. Issue #56's path and all
resident files are untouched.

`deja "unit 0x38 effect entries"` returned the issue #58 session (PR #63)
and the earlier entry 8–10 sessions; nothing covered these four. The
dispatcher and argument-frame findings of
[OVERLAY_UNIT38_EFFECT_ENTRIES.md](OVERLAY_UNIT38_EFFECT_ENTRIES.md) are
reused and were re-checked here by re-decoding the installer stores from the
hash-checked file (table below) and confirming that no fixup-confirmed far
call targets any of the four (`far_in` 0 in `overlay_classification.csv`).

## Provenance (ignored local input; hashes only)

`GAME.OVR`, 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; unit
`0x38` at file offset 60,507 (`0x0EC5B`), stub segment `0x41`, entry *j* at
stub `0x41:(0x20 + 5·j)`. Capstone 5.0.7, 16-bit mode, decoded from each
entry offset up to the next ledger prologue; every jump target was checked to
lie inside its routine.

| Entry | Unit offset | File offset | Bytes | Insns | Item | Installer store | SHA-256 of the routine's bytes |
| ---: | --- | --- | ---: | ---: | ---: | --- | --- |
| 6 | +0x01C3 | 0x0EE1E | 101 | 34 | 6 | `[0x6840]`/`[0x6842]` at +0x31E6/+0x31EF | `6bf2f1903ff586d168c7f6ab5e6c3215590cca5ee90293c153799293ed509229` |
| 75 | +0x24D9 | 0x11134 | 111 | 46 | 90 | `[0x6990]`/`[0x6992]` at +0x361D/+0x3626 | `aa378e2a668ca4819b429584d7a96ce51313ae5583d09bded7fb75112ab054b1` |
| 82 | +0x2700 | 0x1135B | 48 | 15 | 98 | `[0x69B0]`/`[0x69B2]` at +0x3685/+0x368E | `d773c59091a2f6377bcf644dbd3e7b9ed9f8dcb9dea6bcb29794ef4a3adee42b` |
| 97 | +0x2A28 | 0x11683 | 69 | 27 | 113 | `[0x69EC]`/`[0x69EE]` at +0x3748/+0x3751 | `06db3471a42a6a6ad79db554fbe907431fc3c011bb1a0a17384042ed51d65d0a` |

Bounds versus the issue's leads: the ledger bounds (109, 124, 48, 69) run to
the next prologue. Entries 6 and 75 end 11 and 14 bytes earlier at their
`retf`; the trailing bytes are length-prefixed printable constants (10 and
13 characters), not code, and are not reproduced. Entries 82 and 97 have no
trailing bytes. Entry 6's span is 101 bytes, not the 109 (or 112 to the next
entry) in the lead.

## Argument frame

Every routine is called through unit 0x9e entry 1's `lcall [di + 0x6828]`
with far pointer A at `[bp+0xC]` (a 285-byte character record), far pointer
B at `[bp+8]` and a byte at `[bp+6]`. These four read only A (entries 6, 75,
82) or nothing from the frame (97); none reads B or the byte. The record
offsets used (`0x14`, `0x32`, `0x108..0x10B`, `0x11B`) lie inside the
verified 285-byte frame; `0x108` is a documented in-memory far-pointer slot.
All eleven fixup-confirmed dispatcher call sites are now decoded for their
index: the constant indices are 0x0B (unit 0x38 +0x1FF7, unit 0x86 +0x1290),
0x17 (unit 0x86 +0x1A00), 0x2B and 0x2C (unit 0x38 +0x0BE5/+0x0BFD), 0x3A
(unit 0x38 +0x191F/+0x19AC) and 0x4E (unit 0x86 +0x18E3); the other three
sites take the index from an object byte. None selects item 6, 90, 98 or
113, so each routine here is reached only through a computed index.

## Instructions, branches, side effects

### Entry 6 (+0x01C3): classify `Q[0x9F]`, add to two accumulators, set `[0x6819]`

`sub sp,2; les di,[bp+0xC]; les di,es:[di+0x108]; mov ax,es:[di+0xA]; mov
dx,es:[di+0xC]; mov [0x6826],ax; mov [0x6828],dx; les di,[0x6826]; mov
al,es:[di+0x9F]; cmp al,0xA; jne L1; mov byte [bp-1],1; jmp T; L1: cmp al,9;
je L2; cmp al,0xC; jne L3; L2: mov byte [bp-1],2; jmp T; L3: cmp al,4; jne
L4; mov byte [bp-1],3; jmp T; L4: mov byte [bp-1],0; T: mov al,[bp-1]; add
[0x6822],al; mov al,[bp-1]; add [0x6818],al; mov byte [0x6819],9; mov sp,bp`.

Edges: +0x01EB `jne`→+0x01F3, +0x01F1 `jmp`→+0x020F, +0x01F5 `je`→+0x01FB,
+0x01F9 `jne`→+0x0201, +0x01FF `jmp`→+0x020F, +0x0203 `jne`→+0x020B, +0x0209
`jmp`→+0x020F. Four classification paths (values 0x0A → 1; 0x09 or 0x0C →
2; 0x04 → 3; else 0) joining one tail. Side effects, always: scratch far
pointer `[0x6826]/[0x6828]` = `P[0xA..0xD]`; `[0x6822] += class`; `[0x6818]
+= class` (byte adds, modulo 256); `[0x6819] = 9`. The same pointer chain
(`A→0x108→(0xA,0xC)`, then byte `0x9F` of the result) appears in the merged
entry 4, which compares that byte with 4 only.

### Entry 75 (+0x24D9): bracket `A[0x14]` into 1..5 and add to `[0x6816]`, gated by `[0x682A]`

`sub sp,2; cmp byte [0x682A],0; jne done; les di,[bp+0xC]; mov
al,es:[di+0x14]; cmp al,4; jb n1; cmp al,6; ja n1; mov byte [bp-1],1; jmp
add; n1: cmp al,7; jb n2; cmp al,0xA; ja n2; mov byte [bp-1],2; jmp add; n2:
cmp al,0xB; jb n3; cmp al,0xD; ja n3; mov byte [bp-1],3; jmp add; n3: cmp
al,0xE; jb n4; cmp al,0x11; ja n4; mov byte [bp-1],4; jmp add; n4: cmp
al,0x12; jb add; cmp al,0x14; ja add; mov byte [bp-1],5; add: mov al,[bp-1];
xor ah,ah; mov dx,ax; mov al,[0x6816]; xor ah,ah; add ax,dx; mov
[0x6816],al; done: mov sp,bp`. Fifteen edges (one gate `jne` to the exit,
five `jb`/`ja` pairs, four `jmp`s to the add).

Paths: gate closed (nothing changes); five brackets (unsigned ranges 4..6,
7..10, 11..13, 14..17, 18..20 add 1..5); and the fall-through for values
below 4 or above 20, which reaches the add **without any store to `[bp-1]`**.
The byte added on that path is whatever the stack held at `[bp-1]` on entry
(the byte just below the saved `bp`), left by earlier calls on the same
stack; it is not determinable statically. The translation takes it as an
explicit `stack_residue` argument and flags the path. The add is a 16-bit
add of two zero-extended bytes with the low byte stored, so it is modulo 256.
`[0x682A]` is written only by unit 0x9e `+0x0D63` (entry 7, from its
`[bp+8]` argument, at +0x0DAD) and read by unit 0x38 entries 75, 81, 95 and
109; it is the low byte of dispatch-table item 0's segment word, which the
installer never writes.

### Entry 82 (+0x2700): `A[0x11B] += 3`, capped at `A[0x32]`

`les di,[bp+0xC]; add byte es:[di+0x11B],3; les di,[bp+0xC]; mov
al,es:[di+0x11B]; les di,[bp+0xC]; cmp al,es:[di+0x32]; jbe done; les
di,[bp+0xC]; mov al,es:[di+0x32]; les di,[bp+0xC]; mov es:[di+0x11B],al`.
One edge (+0x271B `jbe`→+0x272C), two paths. The add wraps before the
unsigned compare, so `0xFE + 3 = 0x01` is not capped. No engine state is
touched. Record byte `0x11B` is also written by unit 0x38 `+0x0058`, entry 7
(`+0x0233`) and `+0x2122`, and by units 0x1e, 0x6c, 0x86 and 0x9e; byte
`0x32` is read by nine unit 0x38 routines and written by entries 7 and 21 and
`+0x2122`.

### Entry 97 (+0x2A28): decrement `[0x6818]` `[0x681C]` times, clamped at `[0x681C]`

`sub sp,2; mov al,[0x6819]; and al,1; or al,al; je done; mov al,[0x681C];
mov [bp-2],al; mov al,1; cmp al,[bp-2]; ja done; mov [bp-1],al; jmp body;
next: inc byte [bp-1]; body: dec byte [0x6818]; mov al,[0x6818]; cmp
al,[0x681C]; jae keep; mov al,[0x681C]; mov [0x6818],al; keep: mov
al,[bp-1]; cmp al,[bp-2]; jne next; done: mov sp,bp`. Edges: +0x2A35
`je`→+0x2A67, +0x2A42 `ja`→+0x2A67, +0x2A47 `jmp`→+0x2A4C, +0x2A57
`jae`→+0x2A5F, +0x2A65 `jne`→+0x2A49.

Three exits: bit 0 of `[0x6819]` clear; `[0x681C] == 0` (the `cmp al,1 ...
ja` test with `al = 1`); or after the loop. The loop counter starts at 1 and
is compared with the latched limit after each body, so the body runs exactly
`[0x681C]` times (1..255), terminating. Body: `[0x6818] -= 1` (wrapping
`0 → 0xFF`), then if `[0x6818] < [0x681C]` (unsigned) `[0x6818] = [0x681C]`.
`[0x681C]` is written only by unit 0x9e `+0x0E2E` (entry 9, from its `[bp+8]`
argument, at +0x0E37) and read by unit 0x38 entries 50, 97 and 106. Bit 0 of
`[0x6819]` is the same bit that the merged entries 18 and 77 test; entry 6
above sets `[0x6819]` to 9 (bits 0 and 3).

## C contract

`pr_u38n_state` holds the six engine bytes touched by these routines and the
scratch far pointer; far pointers in memory are resolved through the same
callback type as the merged file (14 bytes for P, 0xA0 for Q). Null or
unresolvable inputs return `PR_U38_ARGUMENT`; for entry 6 the scratch pointer
is already written when Q fails, matching the store order. Entry 75 takes
the stack residue explicitly and reports the undefined path with
`PR_U38_PATH_SECOND`; entry 97 reports the iteration count.

## Tests (`tests/recovered/test_overlay_unit38_effects_next.c`)

Expected values are hand-derived, not recomputed with the routine's own
arithmetic. Entry 6: all 256 values of `Q[0x9F]` (exactly four values
classify nonzero), the scratch pointer and `[0x6819] = 9` on every call,
wraparound with class 3, P-unresolvable leaves everything, Q-unresolvable
leaves only the scratch pointer. Entry 75: all 255 nonzero gate values
change nothing; all 256 record values × three residues; the thirteen bracket
boundaries (3/4, 6/7, 10/11, 13/14, 17/18, 20/21, 255) with the byte add
wrapping from 0xFE. Entry 82: all 256 values × 52 caps, plus the
`0xFE + 3 = 0x01` wrap and the equal-to-cap case. Entry 97: twelve
hand-computed cases (gate bits, limit 0, wrap from 0, clamp-and-stay,
limit 255) and a sweep over every limit 1..255 checking the iteration count
and the closed form `max(start − limit, limit)` from start 0x80. Every test
checks that other state fields and other record bytes are untouched.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds the
library and test with no diagnostics and prints `overlay unit 0x38 next
effect-entry tests passed`; CMake build has no warnings and `ctest` passes
16 of 16 targets on main 4cd91ce (fifteen existing plus
`poolrad_overlay_unit38_effects_next_tests`).

## Limitations, stated plainly

- Meanings of `[0x6816]`, `[0x6818]`, `[0x6819]`, `[0x681C]`, `[0x6822]`,
  `[0x682A]`, record bytes `0x14`, `0x32`, `0x11B`, and byte `0x9F` of the
  object two pointers away from slot `0x108` are not asserted. The 1..5
  bracket thresholds and the {0x0A, 0x09/0x0C, 0x04} classes are stated as
  compared values only.
- Entry 75's out-of-range path is undefined in the original (uninitialized
  local); the translation cannot know the residue and takes it as input. A
  DOS oracle trace (issue #3) could show whether callers ever reach it.
- The gameplay conditions under which a caller computes indices 6, 90, 98 or
  113 are not established.

## Reproduce

Added 2026-10-22 by par-e (VP i21177). Until then this document described its checks but
gave no command to rerun them. The evidence kind is recorded in
[credit_evidence_kind.csv](credit_evidence_kind.csv) (labels: [CREDIT_EVIDENCE_KIND.md](CREDIT_EVIDENCE_KIND.md)).

Synthetic C tests. These are hand-derived checks of the C, with no DOS bytes, so they are not
an original-byte differential:

```sh
cmake -S . -B build
cmake --build build --target poolrad_overlay_unit38_effects_next_tests
ctest --test-dir build -R '^poolrad_overlay_unit38_effects_next_tests$' --output-on-failure
```

| Credit | Exercised by |
| --- | --- |
| `OVL_0038_01C3` | entry 6: `pr_u38_entry6` (its row in the Tests table above) |
| `OVL_0038_24D9` | entry 75: `pr_u38_entry75` (its row in the Tests table above) |
| `OVL_0038_2700` | entry 82: `pr_u38_entry82` (its row in the Tests table above) |
| `OVL_0038_2A28` | entry 97: `pr_u38_entry97` (its row in the Tests table above) |

Measured on `ee07bd68`: `poolrad_overlay_unit38_effects_next_tests` passed (1/1 under ctest).
