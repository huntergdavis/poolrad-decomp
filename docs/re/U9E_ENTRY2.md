# Unit 0x9e entry 2 (+0x0026, stub 0xB0:0x2A) and entry 1 (+0x0000): bounded model

`src/recovered/overlay_u9e_entry2.c` models DOS v1.3 `GAME.OVR` unit `0x9e`
public entry 2, the routine unit `0x7d` `+0x0000` calls to remove an item
record ([U7D_ZERO_CALLEE.md](U7D_ZERO_CALLEE.md)), together with its near
callee `+0x0000` (entry 1, a 38-byte indirect dispatcher). Classification:
**bounded model, not strict full C**: the handlers behind the DS far-pointer
table at `0x6828` and the runtime `FreeMem` are host hooks. **No count or
credit is claimed**; the PM decides after parity review.

`deja "Poolrad unit 0x9E entry2 +0x0026 0xB0:0x2A"` found no prior session.
Reused with citation: U7D_ZERO_CALLEE.md (caller, the three pushed arguments,
the `+0x7F`/`+0x104`/`+5` field layout), U7D_ENTRY1.md (stub mapping), the
ledger rows `0x9e:0x0000`, `0x9e:0x0026`, `0x9e:0x0160`, and GAMEPLAY_PATH.md
(`0xAF8:0x0254` as a release by argument shape).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

Unit `0x9e`: code at file offset `175,850` (`0x2AEEA`), stub paragraph `0xB0`.

| Item | Value |
| --- | --- |
| entry 2 code | `+0x0026..+0x013F` = file `0x2AF10..0x2B029`, **282 bytes, 94 instructions**, `retf 0xA` at `+0x013D`. SHA-256 `12578dfdfb77080908623cf336552b20354b56128580f377b3d26ad2caf1df12` |
| trailing bytes | `+0x0140..+0x015F`, 32 bytes, up to the next prologue `+0x0160`: a Pascal 256-bit set constant. The routine at `+0x0160` pushes `cs:0x0140` at `+0x01A0` and calls runtime `0xAF8:0x0DC0` (a set-membership test by argument shape, as in GAMEPLAY_PATH.md). Not part of entry 2; the PM's 314-byte bound is 282 + 32 (the ledger counts 313 decoded bytes because it decodes into the constant) |
| entry 1 | `+0x0000..+0x0025`, **38 bytes, 16 instructions**, `retf 0xC` at `+0x0023`; SHA-256 `1572342d9be429f7a7b7f6b015b2a609726b4d5580e34235fda31eb2a43b9217` |
| frame | `sub sp, 8`: `[bp-8]` item far pointer, `[bp-4]` second far pointer (predecessor scan); args `[bp+0xE]:[bp+0xC]` member, `[bp+0xA]` type byte, `[bp+8]:[bp+6]` item (10 bytes, `retf 0xA`) |
| calls | 2: near `push cs; call +0x0000` at `+0x00A1` (five words: type, member, item, `1`; 12 bytes, `retf 0xC`), and far runtime `0xAF8:0x0254` at `+0x0135` with (item far pointer, 9) |
| DS accesses (entry 2) | none |
| ES accesses | reads at member `+0x7F`/`+0x81` (item head), item `+0` (type byte), `+4` (byte), `+5`/`+7` (next); writes to member `+0x7F`/`+0x81` or item `+5`/`+7` (the unlink) |

## Entry 2 paths

| Offsets | Effect |
| --- | --- |
| `+0x002C..+0x003E` | `[bp-8] := item`; item non-null: skip the search |
| `+0x0040..+0x0076` | item null: `p := member.head (+0x7F)`; `while p <> nil and p.type <> type do p := p.next (+5)` |
| `+0x0078..+0x0080` | `p = nil`: `jmp +0x013A`, return with nothing done |
| `+0x0083..+0x00A1` | `p.byte4 <> 0`: near `+0x0000`(type, member, p, 1) |
| `+0x00A4..+0x00D1` | `member.head = p` (re-read after the call): `member.head := p.next` |
| `+0x00D3..+0x0127` | else `q := member.head; while q <> nil and q.next <> p do q := q.next; q.next := p.next` (a `q = nil` here stores through `es:[0:5..8]`) |
| `+0x012B..+0x0135` | `FreeMem(p, 9)` (argument shape; the record is 9 bytes: type `+0`, word `+1`, byte `+4`, next `+5`) |
| `+0x013A..+0x013D` | `mov sp, bp; pop bp; retf 0xA` |

The type argument is only used for the search and for the `+0x0000` call;
a given item is removed whatever its type byte is. Entry 1 (`+0x0000`) pushes
member, item and the mode byte, then `lcall [di + 0x6828]` with `di = type * 4`:
a 256-entry far-pointer table (1 KB) of handlers taking `(member, item, mode
byte as a word)` (the handler returns with `retf 0xA`). With mode 1 (what entry
2 passes) the handlers presumably undo the item's effect before it is
removed; that is **not asserted**: the table's contents and the handlers are
not read here.

## Model, boundaries and guards

* `handlers[256]` stand for the DS table; a `NULL` slot is a null far
  pointer (the original would call `0000:0000`); the model skips it and
  reports `handler_was_null`. The handler runs **before** the unlink, with
  the record still linked, as the test checks.
* `release(item, 9)` stands for the runtime `FreeMem`. It runs after the
  unlink. Argument shape only.
* The record is not in the member's list at the unlink (`q` runs off the
  end): the original writes through a null far pointer and then frees the
  record. The model returns `PR_U9E_UNLINK_FAILED` there with no release.
  This also arises when a handler removes the record itself.
* The data structure is the `pr_u7d0_member`/`pr_u7d0_item` pair of
  overlay_u7d_zero.h; `pr_u7d0_item` gained a `flag4` field at the end
  (byte `+4`), which overlay_u7d_zero.c does not read.
  `pr_u9e_as_u7d0_remove_hook` has the `pr_u7d0_remove_hook` signature, so
  that model's unit 0x9e call can be wired to this one.

## Verification

`tests/recovered/test_overlay_u9e_entry2.c`: 4,000 generated lists (0..6
items, three type values, random `flag4`, targets: each given item, a search
that hits and one that misses) compared with an id-array reference for the
surviving order, the selected record, the release call, handler presence and
order, `was_head`, and the handler seeing the record still linked with mode 1
and the right member; pinned cases (first match wins, given item ignores the
type, head/tail unlink, handler by type byte, NULL slot, entry 1 direct,
ARGUMENT, UNLINK_FAILED, a handler that removes the record); and a
connected run of overlay_u7d_zero.c with this model as its hook, hand-traced
from the bytes (A and the tail C freed, B reduced). Synthetic data only. The
reference shares my reading of the decode; this is not a DOS replay.
