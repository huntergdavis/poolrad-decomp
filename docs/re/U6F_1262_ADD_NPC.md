# U6F +1262 as a whole function: the ADD NPC party routine (Claude C1, PM GO i8787)

**Scope.** The complete executable span of GAME.OVR unit 0x6F + 0x1262 translated into
portable C at the level of its Pascal semantics, with every external call kept as an
explicit boundary. Inputs: `GAME.OVR` 206,105 bytes, sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; `START.EXE` 65,231
bytes, sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. Unit 0x6F
starts at GAME.OVR offset 0x1DFE6 (9,189 code bytes); the function is public entry 7
(run-time stub `0x7C:0x43`). Ownership: screened read-only (i8671) and assigned by the PM
in i8787; no C of this span exists in the tree. It composes the merged +0482 model
([U6F_0482_PICTURE_BUILD.md](U6F_0482_PICTURE_BUILD.md)) and +1446 model
([U6F_PARTY_INSERT_1446.md](U6F_PARTY_INSERT_1446.md)) as boundaries and edits neither.
The `deja` recall hung (index damaged per i8257); the merged notes cited below are the
prior evidence. C: `src/recovered/overlay_u6f_1262_add_npc.c`,
`include/poolrad/recovered/overlay_u6f_1262_add_npc.h`,
`tests/recovered/test_overlay_u6f_1262_add_npc.c`, built by
`tests/recovered/build_u6f_1262.sh`; CMake registration in a separate commit of the same
branch.

## 1. Byte identity, independently derived

| item | value |
| --- | --- |
| executable span | `[1262,1426)`, GAME.OVR `[1F248,1F40C)`, 452 bytes, 174 instruction starts, sha256 `f22447045d8935d2af21768c0a3a0052a321e0806d6d939adc4a4bc2d3816bd7` |
| outer return | `retf 2` at +1423 (one word argument at `[bp+6]`; only its low byte is read) |
| preceding bytes | entry 6 (+0EC4..+1261, the record initializer, Codex bounded model) ends immediately before |
| following bytes | `[1426,1446)`, 32 bytes of data before the +1446 prologue, excluded (sha256 `d22f8e3450655784853879c7ecfee4ff36192352f6ffe0d40593dee56f583350`) |
| frame | `sub sp, 0x12`: record far pointer `SS:BP-8` (copied to `SS:BP-0x12`), count `SS:BP-9`, loop index `SS:BP-0xA`, the index list `SS:BP-0xF` (entries 1..7) |
| calls | 11: SYSTEM `0xAF8:0x023F` (GetMem) and `0x11C0` (Random); near +0EC4, +1446, +0482; overlay `0xB0:0x48` (unit 0x9E entry 8) x6 |
| branches | 12: +1272 `jbe +1277` / +1274 `jmp +1420` (the count gate); +12B6 `jmp +12BB`, +12CA `jle +12DC`, +12E0 `jne +12B8` (the 7-slot loop); +12E6 `jne +12F7`; +1312 `jne +1347`, +1349 `jne +137D`, +137F `jne +13B3` (the class switch) with +1344, +137B, +13B1 `jmp +13E3` |
| caller | unit 0x1C + 0x2B7E only (far site +2B9B, fixup-confirmed): the ADD NPC script handler (opcode 0x36, [ECL_CONTRACTS.md](ECL_CONTRACTS.md) section 3), which pushes the low byte of its first operand (slot value 1) |

Source and ownership map of what the span touches:

| item | where it is established | owner |
| --- | --- | --- |
| the window-1 word at `[0x49D6] + 0x67C`, the party count | [SLOT_A_NATIVE_PARTY_COUNTER_67C.md](SLOT_A_NATIVE_PARTY_COUNTER_67C.md), [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md) | caller-supplied here |
| `GetMem(0x11D)`, the 285-byte character record | [U1C_LOAD_MONSTER_04AF.md](U1C_LOAD_MONSTER_04AF.md) (the same size) | `alloc_record` callback (the host owns the record) |
| near +0EC4, entry 6: `(object, id)` fills the record from the id | [U6F_ENTRY6_0EC4.md](U6F_ENTRY6_0EC4.md), bounded `pr_u6f_entry6` | Codex; boundary `init_record` |
| `0xAF8:0x11C0` with the word 0x100: `Random(range)` on the DS seed | [U1C_RANDOM_0234.md](U1C_RANDOM_0234.md), [U9E_ENTRY8_ROLL.md](U9E_ENTRY8_ROLL.md) (seed recurrence) | boundary `random_byte` |
| record bytes 0x96..0x9C (the signed class vector; positive = present) | [U68_PARTY_BYTE_0236.md](U68_PARTY_BYTE_0236.md), [SLOT_A_NATIVE_U9B_ENTRY_RETURN_PARITY.md](SLOT_A_NATIVE_U9B_ENTRY_RETURN_PARITY.md) | read inline |
| unit 0x9E entry 8, `(count, range)` dice roll in AL | [U9E_ENTRY8_ROLL.md](U9E_ENTRY8_ROLL.md), translation `pr_u9e_entry8_roll` | boundary `roll` |
| record byte 0x9E (signed, the race row) and the DS tables `0x08D7`, `0x08EB`, `0x08F5`, `0x08E1` (stride 5) and `0x08C7` (stride 8), all inside the file image; their roles (class picture byte 0xBC, picture byte 0xBB) are read off this span | this note; bytes not reproduced | caller-supplied tables with spans |
| near +1446, the party insert | [U6F_PARTY_INSERT_1446.md](U6F_PARTY_INSERT_1446.md), merged `pr_u6f_party_insert_1446` | boundary `insert_party` |
| near +0482 with 1, the picture builder | [U6F_0482_PICTURE_BUILD.md](U6F_0482_PICTURE_BUILD.md), merged `pr_u6f_0482_picture_build` | boundary `build_pictures` |

## 2. Semantics

```
procedure AddNpc(id: word);                           { entry 7, retf 2; the low byte is used }
  if Window1.count > 7 then exit;                     { +1268: [0x49D6] + 0x67C, unsigned }
  rec := GetMem($11D);                                { +1277: the result is never tested }
  InitRecord(rec, Lo(id));                            { +1286: near +0EC4 }
  rec[$AB] := Random($100);                           { +129D: 0xAF8:0x11C0 }
  count := 0;
  for k := 0 to 6 do                                  { +12AE..+12E0 }
    if ShortInt(rec[$96 + k]) > 0 then begin count += 1; list[count] := k end;
  if count = 0 then begin count := 1; list[1] := 2 end;   { +12E2 }
  sel := list[Roll(1, count)];                        { +12F7: unit 0x9E entry 8 }
  v := Roll(1, 5); race := ShortInt(rec[$9E]);        { +1310..+13DE: one of four tables }
  case sel of
    0: rec[$BC] := DS:[$08D7 + race*5 + v];
    5: rec[$BC] := DS:[$08EB + race*5 + v];
    6: rec[$BC] := DS:[$08F5 + race*5 + v];
    else rec[$BC] := DS:[$08E1 + race*5 + v];
  end;
  rec[$BB] := DS:[$08C7 + race*8 + Roll(1, 8)];       { +13E3 }
  InsertParty(rec);                                   { +140F: near +1446 }
  BuildPictures(1);                                   { +1419: near +0482 }
```

What it is: the ADD NPC script opcode's worker. With room in the party, it allocates a
record, has entry 6 fill it from the NPC id, writes a random byte, picks one of the
record's present classes at random, rolls the two picture bytes from the race-indexed
tables, inserts the record into the party and builds its pictures with recoloring.

Facts the C preserves: the count gate is unsigned (`jbe`) against 7; GetMem's result is
never tested; the class scan is signed (`jle` skips zero and negative bytes) and appends
slot indices 0..6 in order to a 1-based list; an empty list becomes the single class 2;
the first roll's range is the list length; the second roll (range 5) happens inside the
selected branch, after the selection, and the race byte is sign-extended (`cbw`) and
scaled by 5 (`shl`, `shl`, `add`) or by 8 (`shl 3`) in 16-bit arithmetic, so a negative
race reaches below the table in the engine; the third roll (range 8) follows the class
byte write; the insert precedes the picture build; the picture build receives the flag 1.

## 3. Boundaries, kept explicit

| source call | identity | C boundary |
| --- | --- | --- |
| `0xAF8:0x023F` | GetMem(0x11D) | `alloc_record(&record, &size)`; the host owns the record |
| near +0EC4 | entry 6, the record initializer, `retf 6` | `init_record(record, size, id)` |
| `0xAF8:0x11C0` | Random(0x100) | `random_byte(0x100, &al)` |
| `0xB0:0x48` | unit 0x9E entry 8, the dice roll, `retf 4` | `roll(count, range, &al)` x3 (ranges count, 5, 8) |
| near +1446 | the party insert, `retf 4` | `insert_party(record)` |
| near +0482 | the picture builder, `retf 2` | `build_pictures(1)` |

Host spans and contracts: the allocated record must be non-NULL and span 0x11D bytes
(`ARGUMENT` at +127B, nothing written); the first roll's result must lie in 1..count,
because the engine would otherwise read an unwritten byte of its list local (`ARGUMENT` at
+12FE after the random byte); each table index (`race*stride + roll`, computed as the
engine does with a sign-extended race) must lie inside the supplied span (`ARGUMENT` at
the branch's roll site, before the write; the class byte already written stands when the
picture-table read fails at +13E9). A missing callback at a reached boundary reports
`NEED_HOOK` with the call instruction's offset in `next_offset`; a callback failure
reports `HOST_FAILURE` with every earlier effect kept. The five DS tables are
caller-supplied; their bytes are not in the public tree. No host guard is needed: the
single loop has a fixed trip count.

## 4. Classification

Whole-function **path-complete model**: all 174 instruction starts are covered at the
semantics level and all twelve branches are represented; the 11 calls are boundaries. It
is not a strict full translation (six callbacks) and no native run was made (host hold).
No strict credit is claimed.

## 5. Tests

Synthetic callbacks record the call order; a fake initializer fills the record with a
chosen class vector and race; synthetic tables whose bytes encode their own index.
Full path: order `G I R D D D P B`, the id, the random byte at 0xAB, the single present
class selected, the three rolls' counts and ranges, the class byte from the "other" table
at race*5 + roll, the picture byte at race*8 + roll, the record passed to the insert, the
flag 1 passed to the build. Class selection: three present slots listed in order and the
third selected (class 5 table), classes 0 and 6 through their tables, no present slot
giving class 2 with a range-1 roll, race 0 with the largest rolls. The party-count gate at
8 (nothing happens) and 7 (proceeds); each of the six boundaries missing, in reach order,
with the stopped offset and the earlier effects; insert, build, initializer and roll
failures keeping the earlier effects; a NULL and a short record, a roll of 0 and a roll
above the count refused; a negative race and a race past the class table refused at the
branch's roll site with nothing written; a short picture table refused after the class
byte was written; null arguments with no effects. Builds: strict
(`-std=c11 -Wall -Wextra -Wpedantic -Werror`), the same under
`-fsanitize=address,undefined`, and `gcc -fanalyzer` on the model and the test (clean).
`cppcheck` and `clang-tidy` are not installed on this host.

## 6. Registration

Applied in the separate commit of this branch, after the +0482 entries:

```cmake
# poolrad_recovered sources: + src/recovered/overlay_u6f_1262_add_npc.c
add_executable(poolrad_overlay_u6f_1262_add_npc_tests tests/recovered/test_overlay_u6f_1262_add_npc.c)
target_link_libraries(poolrad_overlay_u6f_1262_add_npc_tests PRIVATE poolrad_recovered)
add_test(NAME poolrad_overlay_u6f_1262_add_npc_tests COMMAND poolrad_overlay_u6f_1262_add_npc_tests)
```

## 7. Queue

Next, pending PM scope: +069E (entry 4, 1,062 bytes, pinned in i8757; its near callees
+0010, +01B8 and +02AE have merged models). Not started.
