# Unit 0x68 internal +0x0236: party byte test (full translation)

`src/recovered/overlay_u68_party_byte_0236.c` translates DOS v1.3 `GAME.OVR`
unit `0x68` internal routine `+0x0236..+0x0293`, the helper entry 1's message
blocks call three times ([U68_ENTRY1.md](U68_ENTRY1.md)). It walks the party
list and reports whether any node's signed byte at `+0x96 + k` is positive.
Classification: **full translation of all 32 instructions** (no calls; the only
writes are its own locals); the party list and node records are explicit host
state. PM postmerge parity review accepted strict full-C and verified
game-identity credit. Saved one-Up reachability remains unproved.

`deja "Poolrad unit 0x68 +0x0236 near callee"` found no earlier trace. Reused
and re-checked: U68_ENTRY1.md (the three call sites and the argument values),
U68_LIST_SEARCH_05EE.md and RESIDENT_PARTY_COUNT.md (party list head `[0x5D96]`,
next pointer `+0x104`; same list shape), and PR #155's `+0x04DF` note, which
is a different routine and is not duplicated.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| extent | unit `0x68` (file `91,851`) `+0x0236..+0x0293` = file `0x16901..0x1695E`, **94 bytes, 32 instructions**, `retf 2` at `+0x0291`; next prologue `+0x0294` (a 587-byte routine), so the ledger bound is exact and there is **no trailing data**. SHA-256 `3640b91e304bc0d171d0c8d02f9b25e43142942aedb1dabfddfea6ee3a4d6876` |
| frame | `sub sp, 6`: `[bp-6]` found, `[bp-5]:[bp-3]` node far pointer, `[bp-1]` result copy; one word argument `[bp+6]` read as a byte |
| calls | none |
| DS reads | far pointer `[0x5D96]` (party head) |
| memory reads | `es:[node + 0x96 + k]` (byte), `es:[node + 0x104]` and `+0x106` (next far pointer) |
| writes | locals only; nothing in DS or ES |
| near callers | exactly three, all in entry 1, each `push cs; call +0x0236` after `mov al, imm; push ax`: `+0x0DDE` with 6, `+0x0F1D` with 6, `+0x0F57` with 5 (a scan of every `call rel16` in the unit with target `+0x0236`) |
| branches | 3 forward conditionals (`+0x0253 je`, `+0x0259 jne`, `+0x026A jle`), one backward `jmp +0x024D` (the loop) |

## Paths

| Offsets | Effect |
| --- | --- |
| `+0x023C..+0x024A` | `found := 0; node := [0x5D96]:[0x5D98]` |
| `+0x024D..+0x0253` | node = nil -> `+0x0285` |
| `+0x0255..+0x0259` | `found <> 0` -> `+0x0285` |
| `+0x025B..+0x026C` | `di := offset(node) + cbw(k)`; byte `es:[di + 0x96]` signed `> 0` (`jle` skips) -> `found := 1`. `k` is sign-extended, so `k` in 0x80..0xFF reads below `+0x96` (down to `+0x16`) and `k` = 0x7F reads `+0x115` |
| `+0x0270..+0x0283` | `node := es:[node + 0x104]:[node + 0x106]`, **also after a hit**; loop to `+0x024D` |
| `+0x0285..+0x0291` | `[bp-1] := found; AL := [bp-1]`; `mov sp, bp; pop bp; retf 2` |

So the result is 1 when some party node's byte at `+0x96 + k` is in `1..0x7F`,
else 0 (empty list included), and the walk stops after the first such node.
A node after the first hit is never read. What the byte or the index 5 and 6
(offsets `+0x9B`, `+0x9C`) mean is **not asserted**: DOS_CHARACTER.md does not
name that region and this routine only tests the sign and zero.

## Host representation and guards

* `pr_u68_0236_node` stands for a far pointer: `segment` is the host byte view
  of the node's ES segment from offset 0 (at most 64 KiB), `offset` the far
  pointer's offset word, `next` the far pointer at `+0x104`.
* **16-bit address semantics are reproduced.** `add di, ax` at `+0x0262` is
  `(offset + cbw(k)) mod 65536` and the displacement `+0x96` of the read at
  `+0x0264` wraps the same way, so the byte read is
  `segment[(offset + cbw(k) + 0x96) mod 65536]`. The report carries the
  effective address of the hit. Tests cover wraps in the first addition
  (`0xFFF0` with `k = 0x7F`, and `0x0010` with `k = 0x80`) and in the
  displacement (`0xFFE0`, `k = 0`).
* The positivity test is written without an implementation-defined cast:
  signed byte `> 0` is `b > 0 && b < 0x80`.
* `PR_U68_0236_ARGUMENT` (result untouched): a null result pointer, or a
  visited node whose segment is null or smaller than the wrapped byte address
  or than the four link bytes at `offset + 0x104` (the link is read after a
  hit too), or a list longer than 65,536 nodes (the original has no cycle
  exit). Nodes after a hit are not validated, matching the original, which
  never reads them.

## Verification

`tests/recovered/test_overlay_u68_party_byte_0236.c`: for every `k` 0..255,
every byte value at the tested index, and lists of 0..4 nodes (the first
positive byte wins; later nodes carry other values), the result, the index,
the visited count and the hit node against an independent raw-byte
restatement (about 1.3 million cases, 0.1 s); the sign boundary values
`0, 1, 0x7F, 0x80, 0xFF` at `+0x9B` and `+0x9C` (the arguments entry 1 passes);
the index extremes `+0x16` and `+0x115`; the three 16-bit wrap cases above
on a 64 KiB view (including that the unwrapped address is not what is read);
a view too small for the wrapped address; a short record after a hit (not read)
and before one (ARGUMENT); a cyclic list with and without a hit; and the null
result. The test also runs clean under AddressSanitizer and
UndefinedBehaviorSanitizer. Synthetic data only. The restatement shares my reading of the decode;
the byte-level correspondence is the table above, not a DOS replay.
