# Unit 0x7d internal +0x0000 (the near callee of entry 1): bounded model

`src/recovered/overlay_u7d_zero.c` models DOS v1.3 `GAME.OVR` unit `0x7d`
internal routine `+0x0000..+0x02F1`, the routine merged entry 1
([U7D_ENTRY1.md](U7D_ENTRY1.md)) calls with `(n, idx)` after it rewrites
window 0 words `+0x018C..+0x0199`. Classification: **bounded path-complete
model, not strict full C**. The one far call (unit `0x9e` entry 2) is a host
hook; the DS tables, window 1 and the far-pointer lists are host state; two
loops the original can run without end are reported, not reproduced. **No
count or credit is claimed**; the PM decides after parity review.

`deja "Poolrad unit 0x7D +0x0000 near callee 754 bytes"` found no prior
session. Reused with citation: U7D_ENTRY1.md (caller, window-0 words, DS
limit table `0x363A`, list head `[0x5D96]` and the `+0x104` next pointer
seen in the `+0x02F2` normalizer), the ledger rows
`0x7d:0x0000` and `0x9e:0x0026`, FUNCTION_LEDGER.md §"Segment 0xB0 is
overlay unit 0x9e".

**Correction to the entry 1 note.** `0xB0:0x2A` is **not resident**:
segment `0xB0` is the stub paragraph of overlay unit `0x9e` and `0x2A` its
entry 2 (`+0x0026`, ledger: 314 bytes, `retf 0xA`, which is exactly the
three arguments below). U7D_ENTRY1.md said resident; fixed there.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| extent | unit `0x7d` `+0x0000..+0x02F1` = file `0x23DCC..0x240BD`, **754 bytes, 248 instructions**, one `retf 4` at `+0x02EF`; the next prologue is `+0x02F2` (the merged normalizer), so there is **no trailing data**. SHA-256 `89f3ee9874752acb304c19bc51e0359391d2b19259524c3d51001b51bf4119b1` |
| frame | `sub sp, 0x1C`; args `[bp+8]` n (zero-extended, so the stale AH `0x93` from entry 1 is dropped), `[bp+6]` idx (byte, later compared as a signed byte) |
| calls | 2, both far: `+0x0019` runtime `0xAF8:0x16A8` (pointer, count, value: a fill, by argument shape only); `+0x01FE` `0xB0:0x2A` = unit `0x9e` entry 2 |
| DS reads | byte `[0x49F3]`; bytes `[0x471D + i]`; word table `[di + 0x363A]`; far pointers `[0x5D96]`, `[0x49D6]` |
| DS writes | `[0x471D + i]` bytes only (fill `0x471E..0x4765`, clears, sets) |
| ES reads | window-1 word `es:[di + 0x67C]`; member records at `+0x7F` (item head), `+0x104` (next member); item records at `+0` (byte), `+1` (word), `+5` (next). Quantities `es:[item + 1]` are the only memory this routine writes through ES (`sub word es:[di+1]`, `+0x01B7`) |

## Paths

| Offsets | Effect |
| --- | --- |
| `+0x0006..+0x001E` | `[0x49F3] != 2`: fill 72 bytes at `DS:0x471E` with 1, then to `+0x005C` |
| `+0x0020..+0x0059` | `== 2`: `found := 0; i := 1; do { if [0x471D+i] then found := 1; i++ } while (!found and i <= es:[[0x49D6]+0x67C])`, `i` a byte. No flag: `jmp +0x02EC`, return, nothing written. With the word `>= 255` the byte wraps and the scan never ends if no flag is set (model: `NONTERMINATING`) |
| `+0x005C..+0x008C` | `r := n; b := idx; while (signed b > 1) { r := (low word of limit[b-1] * r); b-- }`; the table word is `DS:0x363A + 2*(b-1)` |
| `+0x008E..+0x0094` | `r = 0` (`ja` unsigned): return |
| `+0x0097..+0x00A6` | `chunk := min(r, 10)` (byte) |
| `+0x00A9..+0x00D3`, `+0x02BF..+0x02D5` | `i := 1; node := [0x5D96]`; per member: flag `[0x471D+i]` zero skips it; `i` is a byte, `node := +0x104` |
| `+0x00D6..+0x0155` | flag cleared; item head = `node+0x7F`; `tail` = the last item reachable through `+5` (found by walking; skipped when the head is null) |
| `+0x015B..+0x016C` | loop test: `cur = nil` or `done` leaves for the final scan |
| `+0x016F..+0x017F` | `cur = tail` sets `done` (before `cur` is processed, so the tail is still processed) |
| `+0x0183..+0x019E` | quantity 0: `cur := cur.next` |
| `+0x01A1..+0x01D8` | `chunk < quantity`: `quantity -= chunk`, flag `[0x471D+i] := 1`, `cur := cur.next` |
| `+0x01DA..+0x022D` | `chunk >= quantity`: save `cur.next`; call unit `0x9e` entry 2 with (member far pointer, `cur.type` as a word with stale AH, `cur` far pointer); then `cur := node.head` when the predecessor snapshot equals the (re-read) head, else the saved next |
| `+0x0239..+0x028B` | snapshot := head; advance until the node is `cur` or its next is `cur` (so: `cur` itself when `cur` is the head, else its predecessor, else the tail when `cur` is nil); back to `+0x015B` |
| `+0x028E..+0x02BD` | final scan: from `cur`, for each item with a nonzero quantity flag `[0x471D+i] := 1` |
| `+0x02D8..+0x02E9` | `r := r > 10 ? r - 10 : 0`; back to `+0x008E` |
| `+0x02EC..+0x02EF` | `mov sp, bp; pop bp; retf 4` |

**Observable effect, in words.** For each chunk of up to 10 of the converted
amount, every flagged member loses `chunk` from each item quantity that
exceeds it; items whose quantity does not exceed it are handed to unit
`0x9e` entry 2 (what that does to the list is not asserted: the model's test
hook unlinks the item). Members that still have a nonzero quantity stay
flagged for the next chunk. The conversion multiplies `n` by the
limit-table words below `idx`, so with the same `limit` table the carry
normalizer of entry 1 uses, `idx = 3` and `n = 12` is 12 units of the third
word counted in units of word 0. What an item's quantity "is" (a duration
or something else) is not asserted here.

**A quirk the bytes have, reproduced.** When an expiring item's predecessor
is the head (the head itself is unchanged by the removal), the walk
restarts at the head, so the head item is processed a second time in the
same chunk unless the tail has been reached (`done`). The test pins this
(30 → 20 → 10 on the head). It is the instruction trace's behaviour; I do
not know whether it is intended.

## Host representation and guards

* `flags[256]` stand for `DS:0x471D..0x481C` (index 0 is only read by the
  byte-counter wrap). `PR_U7D0_ARGUMENT`: null state, mode 2 with a window 1
  shorter than 0x67E, or a limit table with fewer than `idx` words when the
  multiply is reached (idx 2..127). These are checked before any write.
* A hook that never unlinks, on a list whose head is not its tail, repeats
  the head restart forever in the original; the model stops after
  4,000,000 item steps with `PR_U7D0_STEP_LIMIT` (state partly updated).
* The head-null dead read at `+0x0118` (the original reads `es:[0:5]` into
  `[bp-0x17]`, never used afterwards) is not modelled.

## Verification

`tests/recovered/test_overlay_u7d_zero.c`: 3,000 generated worlds in each
of mode 0 and mode 2 (1..5 members, 0..6 items, quantities 0/10/11/random,
random flags, random `n` 0..4, `idx` 0..3) compared with an index-based
restatement (items by id, lists as arrays) for flags, surviving quantities
and order, removed items and hook count; pinned conversion cases
(`n=2, idx=3`, idx 0/1/0x80, `n=0`, 16-bit wrap at `n=255`); the head
restart; mode-2 early returns, `w = 0`, the `w >= 255` wrap (flag 0
reached) and its non-termination; ARGUMENT guards; the step cap. The
restatement shares my reading of the bytes; byte-level correspondence is
the decode in the tables above, not a DOS replay.

## Next smaller connected target

Unit `0x9e` entry 2 (`+0x0026`, stub `0xB0:0x2A`, 314 bytes, `retf 0xA`,
one runtime call, 22 ES references) is the hooked callee and takes exactly
the three arguments pushed here. It is the next connected target; not
started.
