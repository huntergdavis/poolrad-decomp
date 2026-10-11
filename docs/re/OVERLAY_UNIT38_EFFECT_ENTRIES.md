# Full C translations: sixteen bounded unit 0x38 dispatch entries (issue #58)

`src/recovered/overlay_unit38_effects.c` translates sixteen complete DOS v1.3
`GAME.OVR` routines in overlay unit `0x38`: public entries 2, 3, 4, 13, 16,
18, 25, 28, 32, 36, 48, 59, 74, 77, 98 and 102. Each is a far Pascal
procedure with a proven entry (its public entry slot and the installer's
table store) and a proven exit (one `retf 0xA`), no calls, and every branch
and side effect reproduced in C with synthetic tests. Excluded by the issue:
`+0x0C06` (entry 30, merged in PR #54) and the path chosen by issue #56; no
indexed session or pushed branch for #56 existed when this work started
(`deja "issue 56 call path main menu first gameplay transition overlay
trace"` matched only this session), so the exclusion is stated as an
assumption for the PM to check.

Prior sessions: `deja "overlay unit 0x38 routine translation dispatch"`
returned only the issue #39–#51 sessions (entries 8, 9, 10, 30, 31). Their
dispatcher and installer findings were re-verified from the bytes below, not
reused; the argument-frame identification and the dispatcher's own decode
are new in this document.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x38` begins at `GAME.OVR` file offset 60,507 (`0x0EC5B`); its
run-time stub segment is `0x41`, entry *j* is reached through stub
`0x41:(0x20 + 5·j)` (the addressing model of
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) §3). Decoding used Capstone 5.0.7
in 16-bit mode from each entry offset; the public ledger rows
(`overlay_function_ledger.csv`, grade A, `far_in` 0, no runtime, overlay,
resident or near calls) agree with the spans measured here except where
noted in the table (the ledger's bounds run to the next prologue and include
trailing constants).

| Entry | Unit offset | File offset | Bytes | SHA-256 of the routine's bytes (entry through `retf`) |
| ---: | --- | --- | ---: | --- |
| 2 | +0x010D | 0x0ED68 | 16 | `870dfc9d737ed044d6ff9eae323534b19716dc36b040f6e126296cabcf5cb5ac` |
| 3 | +0x011D | 0x0ED78 | 30 | `662a14592b4bd33f363a7cd1321b1b0a084a960ea5aa001b065ac0023c4c9561` |
| 4 | +0x013B | 0x0ED96 | 52 | `a45847e50cad98c88814bbce4a5ade86d3bb35b6e47c26be77841906de2f30de` |
| 13 | +0x05D6 | 0x0F231 | 21 | `f54892d787482fdfed6a0e3fe96bf1f99a7ab108ac5115d5f5e2c3396bb92ab8` |
| 16 | +0x064E | 0x0F2A9 | 43 | `adf199f3567a94dc442f3f78fa2fdb153f4a202c0c5070992d7cb50f2cacd9ff` |
| 18 | +0x06E4 | 0x0F33F | 31 | `97fcb1bc4d530f555c59adfe60535c307c84bb1533166d86362c6cc727a7990c` |
| 25 | +0x0A3D | 0x0F698 | 28 | `c9b7e3a616f08c0ba4ebc8062a27eab5d9d0c1bd3a1cc3c8ee49e0141e8f06fe` |
| 28 | +0x0BAC | 0x0F807 | 35 | `429cb20f2a77d11e0894f28f39a54c044013a8bd53c89600f5f54b3654127579` |
| 32 | +0x0C26 | 0x0F881 | 32 | `80fe0c0751186f0d1915eb6beac8701eab590af05ed3c1e859204d258366e55d` |
| 36 | +0x10C1 | 0x0FD1C | 17 | `b5d955e5fffd21d95444654cb10c58fb8d12dc95c2c3ca746e53e6f8e13ec124` |
| 48 | +0x1452 | 0x100AD | 32 | `babc69f38d66e8851cf65882fc0a0db3f6035d7f924ca2509886626165e74c03` |
| 59 | +0x171F | 0x1037A | 17 | `1f2d6699bfe608f6c5a43258e1d0d8b2f80c0fdb73e6b24d166c8d0804ed2e91` |
| 74 | +0x2490 | 0x110EB | 73 | `7eb6a01d0dfcbf1ec13c6155f3f59f431c888e10458b3d41f84d8f7dd3b866f3` |
| 77 | +0x25A2 | 0x111FD | 26 | `02dceb0d2e63a952c80a99e5d37043b2e5130d3e86ba5728bb365a43ac385367` |
| 98 | +0x2A6D | 0x116C8 | 26 | `4db80a01a5beffca897e9d24e1b321c34847df4e295570ea97b30309ea22cc8a` |
| 102 | +0x2B63 | 0x117BE | 26 | `44a8afd97e0f07ee486a431a1ce8858bc63e0bc13c26826c7e7d6eada7aa2021` |

Trailing bytes between a routine's `retf` and the next entry (entries 4, 18,
25, 32, 36) are length-prefixed printable constants (16, 12, 12, 5 and 12
bytes), not code; they are not reproduced and no meaning is taken from them.

## Entry route: installer, dispatcher, and the argument frame

**Installer.** Unit `0x38 + 0x31A2` (entry 1, 1,799 bytes, 276 data-segment
references) stores each entry's far pointer as `mov ax, 0x20+5j; mov dx,
0x41; mov [addr], ax; mov [addr+2], dx`. A pattern scan over its decode finds
all 116 stores; the sixteen entries here each appear exactly once:

| Entry | Stub | Table address | Item `(addr − 0x6828) / 4` | Store at |
| ---: | --- | --- | ---: | --- |
| 2 | `0x41:0x2A` | `0x682C` | 1 | +0x31A5 |
| 3 | `0x41:0x2F` | `0x6830` | 2 | +0x31B2 |
| 4 | `0x41:0x34` | `0x6834` | 3 | +0x31BF |
| 13 | `0x41:0x61` | `0x6860` | 14 | +0x324E |
| 16 | `0x41:0x70` | `0x686C` | 17 | +0x3275 |
| 18 | `0x41:0x7A` | `0x6878` | 20 | +0x329C |
| 25 | `0x41:0x9D` | `0x689C` | 29 | +0x3311 |
| 28 | `0x41:0xAC` | `0x68AC` | 33 | +0x3345 |
| 32 | `0x41:0xC0` | `0x68BC` | 37 | +0x3379 |
| 36 | `0x41:0xD4` | `0x68D0` | 42 | +0x33BA |
| 48 | `0x41:0x110` | `0x6910` | 58 | +0x348A |
| 59 | `0x41:0x147` | `0x6944` | 71 | +0x3533 |
| 74 | `0x41:0x192` | `0x698C` | 89 | +0x3610 |
| 77 | `0x41:0x1A1` | `0x699C` | 93 | +0x3644 |
| 98 | `0x41:0x20A` | `0x69F0` | 114 | +0x3755 |
| 102 | `0x41:0x21E` | `0x6A00` | 118 | +0x3789 |

Item 0 (`0x6828`) is never stored; its four bytes are the far-pointer
temporary `[0x6826]/[0x6828]` that fifteen unit 0x38 routines (entry 4
among them) write. The 7-byte stub entry 116 (`0x41:0x264`) fills the nine
otherwise empty items 5, 13, 19, 24, 27, 60, 63, 92 and 127.

**Dispatcher.** Unit `0x9e + 0x0000` (entry 1, stub `0xB0:0x25`, 38 bytes,
decoded in full, `retf 0xC`): `push [bp+0xE]; push [bp+0xC]; push [bp+0xA];
push [bp+8]; mov al,[bp+6]; push ax; mov al,[bp+0x10]; xor ah,ah; mov di,ax;
shl di,1; shl di,1; lcall [di+0x6828]`. It is the only `lcall` through
`0x6828` in the overlay. Its own six words are (index byte, far pointer,
far pointer, byte). Therefore every table entry's frame is:

| Callee frame | Holds | Dispatcher parameter |
| --- | --- | --- |
| `[bp+0xC]`, `[bp+0xE]` | far pointer **A** (offset, segment) | second parameter |
| `[bp+0x8]`, `[bp+0xA]` | far pointer **B** | third parameter |
| `[bp+0x6]` | byte **C** (pushed as a word) | fourth parameter |

**Who calls the dispatcher, and what A and B are.** Eleven fixup-confirmed
far calls target `0xB0:0x25` (callgraph product, run-time addressing):

| Caller | Index pushed | A | B | C |
| --- | --- | --- | --- | --- |
| unit 0x77 `+0x14D0`, sites +0x1543 and +0x1663 | byte `+0x3E` of its own `[bp+6]` object | copy of `[0x5D92]/[0x5D94]`, the active character far pointer | its own `[bp+6]` far pointer (an object with bytes +0x2E, +0x34, +0x36, +0x3E) | 1, then 0 |
| unit 0x38 `+0x18B5` site +0x191F; `+0x1935` site +0x19AC | `0x3A` (item 58 = entry 48) | its own `[bp+0xC]` pointer passed through | `0:0` | 0 |
| unit 0x38 `+0x1E77` site +0x1FF7 | `0x0B` (item 11 = entry 11) | `[0x6826]/[0x6828]`, loaded from A′→`+0x108`→`+0xA/+0xC` | result cell of resident `0xBA:0x2199` (sidecar lookup, key `0x0B`) | 0 |
| unit 0x86 `+0x11DD` site +0x1290 | `0x0B` | `[0x6C95]/[0x6C97]` | result cell of `0xBA:0x2199` (key `0x0B`) | 0 |
| unit 0x38 `+0x2EDA` site +0x2F40 | byte `+0x3D` of its `[bp+8]` object | its own `[bp+0xC]` passed through | `0:0` | 0 |
| unit 0x38 `+0x0BCF` (entry 29) sites +0x0BE5, +0x0BFD | `0x2B`, `0x2C` (items 43, 44 = the 7-byte stubs 44, 45) | passed through | passed through | passed through |
| unit 0x86 `+0x185B` site +0x18E3; `+0x19C3` site +0x1A00 | (not decoded further) | | | |

A is a 285-byte character record: the unit 0x77 caller passes the active
record pointer, and the record offsets these routines use (`+0x15`,
`+0x108..0x10B`, `+0x111`, `+0x112`) all lie inside the verified 285-byte
frame, with `+0x104..0x10B` documented as in-memory far-pointer slots
([DOS_CHARACTER.md](../formats/DOS_CHARACTER.md)). Slot `+0x108` is written
by unit 0x6f (`+0x0AEC` at +0xC85/+0xC8A clears it after the disk read;
`+0x0EC4` at +0xF9B/+0xFA0 sets it), units 0x20 and 0x33, and dereferenced
by `les` in units 0x2f, 0x31, 0x38, 0x5e, 0x3c6, 0x77, 0x86, 0x9e. B is a
9-byte sidecar node in two callers ([RESIDENT_SIDECAR_LOOKUP.md](RESIDENT_SIDECAR_LOOKUP.md):
key at +0, link at +5/+7, so byte 3 is payload), a different structure in
unit 0x77, and null in three sites. Entry 48 is the one entry here with a
decoded caller index (0x3A from +0x18B5 and +0x1935); both pass B = `0:0`,
consistent with entry 48 never reading B. No routine here reads C.

## Instructions, branches, side effects

Register-level reads/writes are listed exactly; `es:[di+k]` after
`les di,[bp+0xC]` is record byte k, after `les di,[bp+8]` object-B byte k.
"Paths" counts the distinct paths from entry to `retf`.

### State-only routines

| Entry | Instructions (after `push bp; mov bp,sp`; before `pop bp; retf 0xA`) | Branches | Paths | Side effects |
| ---: | --- | --- | ---: | --- |
| 2 | `add byte [0x6825],5; inc byte [0x6822]` | none | 1 | `[0x6825] += 5`, `[0x6822] += 1` (mod 256) |
| 3 | `cmp byte [0x6825],5; jae L1; mov byte [0x6825],0; jmp L2; L1: sub byte [0x6825],5; L2: dec byte [0x6822]` | +0x0125 `jae`→+0x012E; +0x012C `jmp`→+0x0133 | 2 | `[0x6825]` saturating −5 (unsigned); `[0x6822] −= 1` (mod 256) |
| 18 | `mov al,[0x6819]; and al,1; or al,al; je L; mov al,[0x6818]; xor ah,ah; shr ax,1; mov [0x6818],al; add byte [0x6816],3` | +0x06EE `je`→+0x06FF | 2 | if bit 0 of `[0x6819]`: `[0x6818] >>= 1`, `[0x6816] += 3` |
| 25 | `mov al,[0x6818]; xor ah,ah; shr ax,1; shr ax,1; mov dx,ax; mov al,[0x6818]; xor ah,ah; sub ax,dx; mov [0x6818],al` | none | 1 | `[0x6818] −= [0x6818] >> 2` (no wrap possible) |
| 36 | `mov al,[0x681A]; xor ah,ah; shr ax,1; mov [0x681A],al` | none | 1 | `[0x681A] >>= 1` |
| 59 | `mov byte [0x681E],1; sub byte [0x6822],4` | none | 1 | `[0x681E] = 1`, `[0x6822] −= 4` (mod 256) |
| 77 | `mov al,[0x6819]; and al,1; or al,al; je L; mov al,[0x6818]; xor ah,ah; shr ax,1; mov [0x6818],al` | +0x25AC `je`→+0x25B8 | 2 | if bit 0 of `[0x6819]`: `[0x6818] >>= 1` |
| 98 | same with `and al,4` | +0x2A77 `je`→+0x2A83 | 2 | if bit 2 of `[0x6819]`: `[0x6818] >>= 1` |
| 102 | same with `and al,2` | +0x2B6D `je`→+0x2B79 | 2 | if bit 1 of `[0x6819]`: `[0x6818] >>= 1` |

Byte-level comparison: entries 77, 98 and 102 differ pairwise in exactly one
byte (index 7, the `and` immediate); entry 18 differs from the merged entry
10 (`+0x03E1`) in exactly one byte (index 7: immediate 1 instead of 2).

### Routines that touch the record (A) or object B

| Entry | Instructions | Branches | Paths | Side effects |
| ---: | --- | --- | ---: | --- |
| 13 | `les di,[bp+8]; mov al,es:[di+3]; les di,[bp+0xC]; mov es:[di+0x15],al` | none | 1 | `A[0x15] = B[3]` |
| 16 | `les di,[bp+0xC]; cmp byte es:[di+0x111],0x39; jae L1; les di,[bp+0xC]; mov byte es:[di+0x111],0x39; L1: inc byte [0x6816]; cmp byte [0x681B],0xF; jne L2; mov byte [0x6818],0` | +0x065A `jae`→+0x0665; +0x066E `jne`→+0x0675 | 4 | `A[0x111] = max(A[0x111], 0x39)` (unsigned); `[0x6816] += 1`; if `[0x681B] == 0xF`: `[0x6818] = 0` |
| 28 | `sub byte [0x6822],4; les di,[bp+0xC]; sub byte es:[di+0x111],4; les di,[bp+0xC]; sub byte es:[di+0x112],4; sub byte [0x6816],4` | none | 1 | `[0x6822] −= 4`, `A[0x111] −= 4`, `A[0x112] −= 4`, `[0x6816] −= 4` (mod 256, in that order) |
| 32 | `les di,[bp+0xC]; les di,es:[di+0x108]; cmp byte es:[di+3],0; jle L; mov byte [0x681E],1; mov byte [0x6822],0xFF` | +0x0C36 `jle`→+0x0C42 | 2 | P = far pointer `A[0x108..0x10B]`; if signed `P[3] > 0`: `[0x681E] = 1`, `[0x6822] = 0xFF` |
| 48 | `les di,[bp+0xC]; les di,es:[di+0x108]; mov byte es:[di+6],0; cmp byte [0x681D],0; je L; mov byte [0x681A],0` | +0x1467 `je`→+0x146E | 2 | `P[6] = 0` always; if `[0x681D] != 0`: `[0x681A] = 0` |
| 74 | `cmp byte [0x6DE3],0; jne M; cmp byte [0x6822],0; jne M; les di,[bp+8]; mov al,es:[di+3]; and al,0xF; les di,[bp+8]; mov es:[di+3],al; jmp D; M: les di,[bp+8]; mov al,es:[di+3]; and al,0x10; or al,al; jne D; mov byte [0x6822],0xFF; les di,[bp+8]; mov al,es:[di+3]; or al,0x10; les di,[bp+8]; mov es:[di+3],al; D:` | +0x2498 `jne`→+0x24B3; +0x249F `jne`→+0x24B3; +0x24B1 `jmp`→+0x24D5; +0x24BE `jne`→+0x24D5 | 3 | if `[0x6DE3] == 0 && [0x6822] == 0`: `B[3] &= 0x0F`; else if bit 4 of `B[3]` clear: `[0x6822] = 0xFF`, `B[3] |= 0x10`; else nothing |
| 4 | `les di,[bp+0xC]; les di,es:[di+0x108]; mov ax,es:[di+0xA]; mov dx,es:[di+0xC]; mov [0x6826],ax; mov [0x6828],dx; les di,[0x6826]; cmp byte es:[di+0x9F],4; jne L; add byte [0x6822],2; add byte [0x6818],2` | +0x015F `jne`→+0x016B | 2 | P = `A[0x108..]`; Q = `P[0xA..0xD]`; `[0x6826]/[0x6828] = Q` (always); if `Q[0x9F] == 4`: `[0x6822] += 2`, `[0x6818] += 2` (mod 256) |

Every jump target lies inside its routine (checked by the measurement
script); there are no indirect branches and no calls. Flags set by the
arithmetic are dead at the `retf` in all sixteen routines.

**Data-segment bytes touched here, with unit-wide reference counts** (decode
of all 127 ledger routines of unit 0x38): `[0x6816]` 16 references in 14
routines; `[0x6818]` 39 in 21; `[0x6819]` 22 in 20 (7 writes); `[0x681A]` 5 in
3 (entries 33, 36, 48); `[0x681B]` 5 in 5 (one write); `[0x681D]` 1 read
(entry 48 only, in this unit); `[0x681E]` 5 in 4; `[0x6822]` 20 in 19;
`[0x6825]` 5 in 3 (entries 2, 3, 11); `[0x6826]/[0x6828]` 71 in 15;
`[0x6DE3]` 1 read (entry 74 only, in this unit).

## C contract

`pr_u38_state` carries the eleven engine locations by address (ten bytes and
the scratch far pointer). Record A is a writable 285-byte buffer; object B is
a buffer of at least 4 bytes. Far pointers stored in memory (record slot
`+0x108`, and `P[0xA..0xD]` for entry 4) are read as raw little-endian
offset/segment words and handed to a caller-supplied resolver with the
number of bytes the routine will touch (4, 7, 14 or 0xA0); the engine performs
the same dereference unchecked. A null state, record, object or resolver, or
an unresolvable pointer, returns `PR_U38_ARGUMENT`; for entry 4 the scratch
far pointer is already updated when Q fails to resolve, matching the store
order. `paths` reports which conditional block ran
(`PR_U38_PATH_EFFECT`, `PR_U38_PATH_SECOND`).

## Tests (`tests/recovered/test_overlay_unit38_effects.c`)

All synthetic; no DOS bytes. Each test also checks that every other state
field and every other record byte is untouched.

| Entry | Coverage |
| ---: | --- |
| 2 | all 256 values of `[0x6825]` with complementary `[0x6822]`; wraparound |
| 3 | all 256 values: clamp path for 0..4, subtract path for 5..255; `[0x6822]` 0 → 0xFF; entry 2 then 3 |
| 18, 77, 98, 102 | all 256 flag bytes × 16 values of `[0x6818]`; only the own bit fires; entry 18 adds 3 (0xFE → 0x01); bit isolation across the four; repeated halving to 0 |
| 25 | all 256 values equal `v − (v >> 2)`, never above `v`; 0xFF → 0xC0, 3 → 3, 4 → 3 |
| 36 | all 256 values halve |
| 59 | all 256 values of `[0x6822]`; `[0x681E]` forced to 1 from any value |
| 13 | all 256 source values land in record byte 0x15 only; source untouched |
| 16 | all 256 record values × 18 values of `[0x681B]` including 0x0F: clamp below 0x39, increment, conditional clear, path mask |
| 28 | all 256 values across the four bytes, order and wraparound |
| 32 | all 256 values of `P[3]`: effect for 0x01..0x7F only; resolver asked for 4 bytes; unresolvable slot → no change |
| 48 | all 256 values of `[0x681D]`; `P[6]` cleared with neighbours intact; resolver asked for 7 bytes; out-of-range → no change |
| 74 | 4 × 6 × 256 combinations of `[0x6DE3]`, `[0x6822]`, `B[3]`: three paths; mark-then-idle-then-clear sequence |
| 4 | all 256 values of `Q[0x9F]` (only 4 fires); scratch pointer always written; +2/+2 wraparound; P unresolvable leaves scratch alone, Q unresolvable leaves it written |
| family | composition on the shared accumulators |

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds the
library and test with no diagnostics; the test prints `overlay unit 0x38
effect-entry tests passed`. The CMake target `poolrad_overlay_unit38_effects_tests`
passes under `ctest` together with the existing targets (12/12 on the 3c4f015 base; 14/14 after rebasing on 5bc363d).

## Uncertainty, stated plainly

- The meanings of the data-segment bytes (`[0x6816]`, `[0x6818]`,
  `[0x6819]` bits 0–2, `[0x681A]`, `[0x681B]`, `[0x681D]`, `[0x681E]`,
  `[0x6822]`, `[0x6825]`, `[0x6DE3]`), of record bytes `0x15`, `0x111`,
  `0x112`, of the object behind slot `0x108` (bytes 3, 6, far pointer at
  0xA) and of the record byte `0x9F` compared with 4 are not established.
  By use pattern `[0x6816]`/`[0x6822]` are accumulators and `[0x6819]` a flag
  byte, which is a reading only.
- B's type varies by caller (sidecar node, a unit 0x77 object, or null);
  entries 13 and 74 touch only its byte 3, and the C contract requires four
  bytes without naming the object.
- The dispatch item that selects each entry is known (installer table); the
  gameplay condition under which a caller chooses that index is not, except
  for entry 48's two constant-index callers.
- Entry 4 writes the scratch far pointer that overlaps table item 0; the
  engine never dispatches item 0, so no observable interaction follows.
- None of this affects completeness: each routine's behavior depends only on
  the bytes it compares and updates, all of which are modelled.

## Not translated from the same scan

Call-free grade-A entries left for later because they are longer or carry
more state: 6 (+0x01C3, 109 bytes), 75 (+0x24D9, 124), 81 (+0x268A, 118),
97 (+0x2A28, 69), 110 (+0x2E37, 163), 82 (+0x2700, 48, `es` only). The
7-byte stubs (15, 44, 45, 116) and the 5-byte entry 0 are empty prologues.

Throughput: 25 minutes to survey and select, 20 to decode the sixteen
routines and the dispatcher, 25 to identify the argument frame from the
eleven callers, 45 to translate, test and document.
