# U6F +0482 as a whole function: the character picture builder (Claude C1, PM GO i8682)

**Scope.** The complete executable span of GAME.OVR unit 0x6F + 0x0482 translated into
portable C at the level of its Pascal semantics, with every external call kept as an
explicit boundary. Inputs: `GAME.OVR` 206,105 bytes, sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; `START.EXE` 65,231
bytes, sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. Unit 0x6F
starts at GAME.OVR offset 0x1DFE6 (9,189 code bytes); the function is public entry 3
(run-time stub `0x7C:0x2F`). Ownership: screened read-only and assigned by the PM in i8682
after the +1446 duplicate was withdrawn; no C of this span exists in the tree (the
only prior mentions are the +048C push site in
[RESIDENT_DAX_SELECTOR_PUSH.md](RESIDENT_DAX_SELECTOR_PUSH.md) and the record-offset
scan in [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md)). The `deja` recall hung (index
damaged per i8257); the merged notes cited below are the prior evidence. C:
`src/recovered/overlay_u6f_0482_picture_build.c`,
`include/poolrad/recovered/overlay_u6f_0482_picture_build.h`,
`tests/recovered/test_overlay_u6f_0482_picture_build.c`, built by
`tests/recovered/build_u6f_0482.sh`; CMake registration in a separate commit of the same
branch (the pattern the PM set for +22D5).

## 1. Byte identity, independently derived

| item | value |
| --- | --- |
| executable span | `[0482,0674)`, GAME.OVR `[1E468,1E65A)`, 498 bytes, 183 instruction starts, sha256 `be72ede6980e67ba2ef0d51e84c154a281ab9b27b9d00070b7701fcf4bd0a4a3` |
| outer return | `retf 2` at +0671 (one word argument at `[bp+6]`; only its low byte is tested) |
| preceding bytes | `[0476,0482)`, 12 bytes: two private 5-character constants with their length bytes (CS:0x0476, CS:0x047C); data (sha256 `1ad8d30bcb84ce5fdeb11220e995ead691198011e4b77d7928318613f43953f0`) |
| following bytes | `[0674,069E)`, 42 bytes: the private constants of entry 4 (+069E), excluded (sha256 `8ccc8f762faf22b781b85f7460f4b03870e121f535ea8fbe98ee3d8de467017f`) |
| frame | `sub sp, 0x22A`: record far pointer `SS:BP-0x29`, loop counter `SS:BP-0x24`, map_b (16 bytes) `SS:BP-0x23`, map_a (16 bytes) `SS:BP-0x13`, one-character temporary `SS:BP-0x129`, composed name `SS:BP-0x229` |
| calls | 16: six SYSTEM (`0xAF8:0x0B0B` x2, `0x0C3B` x2, `0x0B98` x2), three overlay (`0x3E9:0x34` x2 = unit 0x3CE entry 4; `0x3E9:0x2F` = unit 0x3CE entry 3), two near (+0546, +056A to +032F), five resident (`0x802:0x094A`, `0x464:0x1E5C` x2, `0x802:0x095D`, `0x899:0x0104`) |
| branches | 6: +0571 `jne +0576` / +0573 `jmp +065C`; +057A `jmp +057F`; +059D `jne +057C` (16-iteration loop); +05A3 `jmp +05A8`; +0600 `jne +05A5` (6-iteration loop); both trip counts fixed |
| callers | 9 fixup-confirmed far sites, all in unit 0x6C (the party menu): +3192 and +3233 push 1 (after the entry-10 insert call); +3862, +38A3, +3B52, +3B69, +3BA7, +3C5A, +3CF9 push 0 (after writing record bytes 0xBD, 0xBE, 0xBF or 0xC0); unit 0x6F + 0x1262 (entry 7) calls it near at +141D |

Source and ownership map of what the span touches:

| item | where it is established | owner |
| --- | --- | --- |
| resident `0x802:0x094A` with the pushed byte 3 (`DS:0x5377 := DS:0x5376; DS:0x5376 := 3`) | [RESIDENT_DAX_SELECTOR_PUSH.md](RESIDENT_DAX_SELECTOR_PUSH.md), complete translation `pr_rec_res_0802_094a`; lists this site | Codex; boundary here |
| `DS:0x5D92`, the active character far pointer | [ECL_STORE_HOOK_0A74.md](ECL_STORE_HOOK_0A74.md), [ECL_LOAD_HOOK_6B9B_RECORD.md](ECL_LOAD_HOOK_6B9B_RECORD.md) | caller-supplied record span here |
| record bytes 0xBD, 0xBE (loader p3 values), 0xBF (party slot 0..7), 0xC0 (letter index; set to 1 or 2 by entry 6), 0xC1..0xC6 (six color bytes, two nibbles each) | [U6F_PARTY_INSERT_1446.md](U6F_PARTY_INSERT_1446.md) (+0xBF), [U6F_ENTRY6_0EC4_MAP.md](U6F_ENTRY6_0EC4_MAP.md) (+0xC0); the roles of 0xBD/0xBE/0xC1..0xC6 are read off this span | caller-supplied |
| `DS:0x08C5 + rec[0xC0]`, a byte table of letters (in the file image) | this note; bytes not reproduced | caller-supplied table with span |
| unit 0x3CE entry 4 (`+01C8`, `retf 8`): (name, p3, slot) loads a picture pair into `DS:0x5E5A + 8*slot` | [U3CE_ENTRY4_01C8.md](U3CE_ENTRY4_01C8.md), bounded model `pr_u3ce_entry4` | Codex; boundary here |
| unit 0x3CE entry 3 (`+014E`, `retf 2`): releases both pictures of a slot through `0x464:0x2595` when non-nil (122 bytes, decoded read-only for this note) | this note | boundary here |
| near +032F (327 bytes, no calls, `retf 8`): ORs the source picture's bytes at +0x17.. into the destination's and ANDs the two far-pointed planes at +0x13, with the width from +0x11 and the mode byte `DS:[0x4A1E]` (decoded read-only for this note; its callers here pass `DS:[0x5EB2]` / `DS:[0x5EB6]` as the source and the slot pictures as the destination) | this note | unowned leaf; boundary here, candidate for a later inline translation |
| resident `0x464:0x1E5C`: (map_a, map_b, word, word, picture) copies both 16-byte maps and remaps a non-nil picture (image 0x0649C; decoded read-only to its first branch) | [U3CE_ENTRY4_01C8.md](U3CE_ENTRY4_01C8.md) names it the palette hook | resident lane; boundary here |
| `DS:0x3CF5 + k`, k = 1..6, the map positions of the six color bytes (in the file image; entries at most 7) | this note; bytes not reproduced | caller-supplied table with span |
| resident `0x802:0x095D` (`DS:0x5376 := DS:0x5377`), `0x899:0x0104` (flush: `while keypressed do read key`) | [U6F_22D5_TRANSITION_PROMPT.md](U6F_22D5_TRANSITION_PROMPT.md), [U3CE_ENTRY4_01C8.md](U3CE_ENTRY4_01C8.md) | resident lane; boundaries here |
| `0xAF8:0x0B0B`, `0x0B98`, `0x0C3B` | the PM decode in the ledger, section 1.19 | translated inline |

## 2. Semantics

```
procedure BuildPictures(recolor: word);              { entry 3, retf 2; the low byte is tested }
  SelectorPush(3);                                    { +0489: 0x802:0x094A }
  rec := DS:[0x5D92];                                 { +0491 }
  letter := Chr(DS:[0x08C5 + rec[0xC0]]);             { +04B4 }
  LoadPicture(CS:0x0476 + letter, rec[0xBD], 0x0B);   { +049B..+04D8: unit 0x3CE entry 4 }
  LoadPicture(CS:0x047C + letter, rec[0xBE], rec[0xBF]);   { +04DD..+0520 }
  Merge(DS:[0x5EB2], slot[rec[0xBF]].first);          { +0525..+0546: near +032F }
  Merge(DS:[0x5EB6], slot[rec[0xBF]].second);         { +0549..+056A }
  if Lo(recolor) <> 0 then begin                      { +056D }
    for k := 0 to 15 do begin map_a[k] := k; map_b[k] := k end;      { +0576..+059D }
    for k := 1 to 6 do begin                          { +059F..+0600 }
      i := DS:[0x3CF5 + k];
      map_b[i] := rec[0xC0 + k] and $0F;
      map_b[i + 8] := rec[0xC0 + k] shr 4;
    end;
    Remap(map_a, map_b, 0, 0, slot[rec[0xBF]].first); { +0602..+062A: 0x464:0x1E5C }
    Remap(map_a, map_b, 0, 0, slot[rec[0xBF]].second);   { +062F..+0657 }
  end;
  ReleaseSlot(0x0B);                                  { +065C: unit 0x3CE entry 3 }
  RestoreArchiveNumber;                               { +0664: 0x802:0x095D }
  FlushKeys;                                          { +0669: 0x899:0x0104 }
```

What it is: the party menu calls this entry after a character is inserted into the party
(with the recolor flag) and after each edit of the record's picture bytes (without it). It
loads the character's two pictures (a temporary one into slot 0x0B from byte 0xBD, and the
party-slot one from byte 0xBE into slot 0xBF), composites the two base images over them,
optionally recolors them from the six color bytes, releases the temporary slot, puts the
archive number back and drains the keyboard. The names are the two private 5-character
prefixes plus one letter selected by record byte 0xC0.

Facts the C preserves: the push helper receives AL = 3 (AH is inherited from the caller's
AX and not modelled, as in the Codex note); the letter is read before either load and the
same letter is used for both names; both loads happen before the merges and both merges
before the recolor test; the identity maps are built with a 16-iteration loop and the six
color bytes are placed at the table positions and at those positions plus 8 (low nibble
first, then high nibble); the two remap calls pass the zero words; the release, restore and
flush run on both paths; only the low byte of the argument word is tested (callers push
`mov al,1/0; push ax`).

## 3. Boundaries, kept explicit

| source call | identity | C boundary |
| --- | --- | --- |
| `0x802:0x094A` | archive-number push helper, `retf 2` | `selector_push(low_byte = 3)` |
| `0x3E9:0x34` | unit 0x3CE entry 4, the picture loader, `retf 8` | `load_picture(name, p3, slot)` x2 |
| near +032F | the picture merge leaf, `retf 8` | `merge_picture(slot, which)` x2: `which` 0 pairs `DS:[0x5EB2]` with the slot's first picture, 1 pairs `DS:[0x5EB6]` with its second |
| `0x464:0x1E5C` | palette remap, `retf 0x10` | `recolor_picture(map_a, map_b, slot, which)` x2, the two zero words implied |
| `0x3E9:0x2F` | unit 0x3CE entry 3, the slot release, `retf 2` | `release_slot(0x0B)` |
| `0x802:0x095D`, `0x899:0x0104` | restore, flush | `restore_archive_number()`, `flush_keys()` |
| `0x0B0B`, `0x0C3B`, `0x0B98` | load, Chr, concatenate | translated inline (6-character names) |

Host spans and contracts: the record span must hold byte 0xC0 on entry and byte 0xC6
before the color loop (`ARGUMENT` at +059F with the loads and merges kept); the letter
table must hold the byte at `rec[0xC0]` (`ARGUMENT` at +04B4 after the push, before any
load); the index table must hold entries 1..6 and each must be at most 7, because the
engine writes `map_b[i + 8]` inside its 16-byte local (a larger value would overrun into
`map_a`; the file-image table satisfies this); the two prefixes must have their original
length (5). A missing callback at a reached boundary reports `NEED_HOOK` with the call
instruction's offset in `next_offset`; a callback failure reports `HOST_FAILURE` with every
earlier effect kept. The private CS literals and the two DS tables are caller-supplied;
their bytes are not in the public tree. No host guard is needed: both loops have fixed trip
counts.

## 4. Classification

Whole-function **path-complete model**: all 183 instruction starts are covered at the
semantics level and all six branches are represented; the 16 calls are boundaries or
inline string operations. It is not a strict full translation (seven callbacks) and no
native run was made (host heavy-native hold). No strict credit is claimed.

## 5. Tests

Synthetic callbacks record the call order; synthetic prefixes, letter table (4 entries),
index table (9 entries) and a 0xEE-filled record with bytes 0xBD..0xC6 set. Recolor path:
order `S L L M M R R E A F`, the pushed byte 3, both names (prefix + the letter at index
2), p3 and slot arguments, the merge and remap slot/which pairs, the identity map and the
expected color map (low nibbles at 1, 2, 3, 4, 6, 7; high nibbles at 9, 10, 11, 12, 14,
15), the released slot, the result counts and maps; a high-bit flag value recolors too.
Plain path: order `S L L M M E A F`, no remap, accepted with a record of exactly 0xC1 bytes
and a 2-byte index table; letter indices 0 and 3. Each of the seven boundaries missing, in
reach order (the remap only when the flag is set), with the stopped offset and the earlier
effects; failures of the second load, the first merge, the first remap, the flush and the
push keeping the earlier effects; argument checks (record of 0xC0 bytes, prefix lengths 4
and 6, null hooks and output, no effects); a letter index past the table refused after the
push; a record of 0xC6 bytes, a 6-byte index table and an index entry of 8 refused at
+059F after the merges. Builds: strict (`-std=c11 -Wall -Wextra -Wpedantic -Werror`), the
same under `-fsanitize=address,undefined`, and `gcc -fanalyzer` on the model and the test
(clean). `cppcheck` and `clang-tidy` are not installed on this host.

## 6. Registration

Applied in the separate commit of this branch, after the +22D5 entries:

```cmake
# poolrad_recovered sources: + src/recovered/overlay_u6f_0482_picture_build.c
add_executable(poolrad_overlay_u6f_0482_picture_build_tests tests/recovered/test_overlay_u6f_0482_picture_build.c)
target_link_libraries(poolrad_overlay_u6f_0482_picture_build_tests PRIVATE poolrad_recovered)
add_test(NAME poolrad_overlay_u6f_0482_picture_build_tests COMMAND poolrad_overlay_u6f_0482_picture_build_tests)
```

## 7. Queue

Next, pending PM scope: +1262 (entry 7, 452 bytes; it calls +0EC4, +1446 and this entry
near, so it composes with existing models), then +069E (entry 4, 1,062 bytes; it calls
+0010, +01B8 and +02AE near). The +032F merge leaf (327 bytes, no calls) is a candidate
for an inline translation once a caller's model needs its effects. None is started.
