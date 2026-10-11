# Audit of the DOS Decode Kit's Pool of Radiance research against the supplied DOS v1.3 build

**Date:** 2026-10-10. **Auditor:** Claude Fable 5.1, working only in the isolated copy
`/home/hunter/workspace/poolrad-decomp-fable`. **Purpose:** roadmap research input for
stages 0 to 2 of [ROADMAP.md](../ROADMAP.md). This document contains hashes, counts, offsets,
and format descriptions only. No game bytes, text, images, or save data are reproduced.

## 1. Inputs and provenance

| Input | Identity |
| --- | --- |
| Audited research | [DrEvil-TitaniumHelix/dos-decode-kit](https://github.com/DrEvil-TitaniumHelix/dos-decode-kit), commit `d9f7fb4525e0778f20158c7c0fe67beda9190888` (2026-06-13), MIT. Files under `games/pool-of-radiance/` (`docs/`, `analysis/`, `reproduction/`) and `kit/dax.py`. |
| Independent reference | [bsimser/Gold-Box-Explorer](https://github.com/bsimser/Gold-Box-Explorer), revision `eac30abaa6ee66aea6f5d65ebe6d676b10015a8f`, MIT. Files cited: `src/Common/Plugins/GeoDax/GeoDaxFile.cs`, `GeoDax/GeoWallRecord.cs`, `Dax/EgaBlockSpecification.cs`, `Dax/EgaBlock.cs`, `Dax/DaxWallDefFile.cs`, `DaxEcl/DaxEclFile.cs`, `DaxEcl/Program.cs`. |
| Prior local research | [Mac v1.1 research snapshot](prior-research/macmaps/README.md), especially `AREA_NAMES.md` and `RESEARCH.md`. Macintosh addresses are not DOS facts; only its ECL and GEO format statements were used. |
| Supplied build | `resources/DOS/POOLRAD/` in this copy: 113 `.DAX` archives, `START.EXE`, `GAME.OVR`, 12 `CHRDAT*.SAV`, 12 `.ITM`, 10 `.SPC`, `ITEMS`, `SAVGAMA.DAT`, `SAVGAMJ.DAT`, `POOL.CFG`. |

Hashes of the two executables in the supplied build (SHA-256):

```
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd  START.EXE  (65,231 bytes)
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614  GAME.OVR   (206,105 bytes)
```

MD5, for matching against older community lists: `START.EXE 4c28d37e6d94930e865dd59b528f4cfc`,
`GAME.OVR 2ac38b4b35a33dafd375b76957463804`. A full SHA-256 manifest of the 113 archives was
produced during the audit and can be regenerated with `sha256sum` over the folder.

**The kit publishes no hashes.** A grep of every Markdown file in the kit for md5, sha1, sha256,
or crc found nothing. Identity therefore rests on structural fingerprints, all of which match:

| Fingerprint | Kit claim | Supplied build | Match |
| --- | --- | --- | --- |
| `GAME.OVR` size, magic | 206,105 bytes, `FBOV`, dword at 4 = 206,097 | 206,105, `FBOV`, 206,097 (= size minus 8-byte header) | yes |
| `START.EXE` size, MZ relocations, header | 65 KB, 0 relocations, resident code at file 0x200 | 65,231 bytes, 0 relocations, 32 header paragraphs (0x200) | yes |
| Pascal prologues `55 89 E5` in overlay | 675 | 675 (and zero `55 8B EC`) | yes |
| `int 21h` / `int 10h` / `int 16h` in overlay | 0 / 2 / 3 | 0 / 2 / 3 | yes |
| far returns (`retf`) in overlay | 314 | 314 | yes |
| overlay entropy | about 6.9 bits/byte | 6.90 | yes |
| version string | "version 1.3" present | present once, at overlay offset 0x193 | yes |
| `.DAX` archive count / block count | 113 / 1245 | 113 / 1245 | yes |

Conclusion: the supplied build is, to every structural measure the kit gives, the same binary
the kit studied. Treat it as the same build, but record our own hashes as the authority.

## 2. Byte-verified by this audit

Each item was re-derived from the supplied files with independent code, not by trusting the kit.

1. **DAX container and RLE codec** (kit `FINDINGS` sections 1 and 2, `kit/dax.py`). Running the
   kit's own validator over the copy: 113/113 archives satisfy `2 + 9*N + sum(packed) == size`.
   A stricter re-implementation that requires the RLE decoder to consume every packed byte and hit
   the directory's unpacked length exactly passes for all 1245 blocks. 224 blocks carry the
   0x80 id bit. 229 groups of byte-identical block contents exist across archives (845 distinct
   blocks), so a reader may deduplicate by content hash. This codec agrees with Gold Box Explorer's
   `DaxFile.cs` and the Mac snapshot's description.
2. **GEO record size and count.** All 29 GEO blocks unpack to exactly 1,026 bytes; the first
   two bytes are the same 16-bit value in every block. 29 maps matches the kit's "29 dungeon maps"
   and the Mac snapshot's "29 distinct records".
3. **Overlay segment map** (kit `FINDINGS` 5c, `UNIT_LEDGER.md`). Scanning the resident image
   for Turbo Pascal overlay descriptors (`CD 3F`, zero word, 32-bit file offset, code size, fixup
   size, entry count) yields exactly 34 units with 404 entry points. All 34 (stub segment, overlay
   offset) pairs in the kit's unit table match. Units tile `GAME.OVR` contiguously from offset 212
   to the end with no gaps: code 197,451 bytes plus fixup tables 8,442 bytes = 205,893 bytes
   (99.9%). The kit's "96%" figure omits the fixup tables; the layout claim itself is right.
   Descriptors are not paragraph aligned, and packed-file entry stubs are 5 bytes each starting
   22 bytes after the descriptor. At run time, entry `j` is addressed as the unit's assigned
   segment:`(0x20 + 5*j)`; the next unit's descriptor records that segment.
4. **ECL interpreter loop and dispatch** (kit `ECL_VM_DECODED.md` sections 1.2 and 1.3).
   Disassembled with capstone (16-bit): the function at overlay offset 0x3EB7 stores its parameter
   to the PC global at 0x49ED, clears the stop byte at 0x442E, and loops at 0x3EC5: it tests the two
   stop bytes (0x442E, 0x49FF), latches the previous opcode (0x6F7F to 0x6F7E), loads the buffer far
   pointer from 0x49DE, adds the PC, fetches one byte with the 0x6700 bias, stores it to 0x6F7F, and
   calls 0x3C73. The dispatcher at 0x3C73 is a compare chain on 0x6F7F with 61 distinct immediates
   covering 0x00 to 0x3D; only 0x1F is absent. All 29 handler addresses the kit lists are reached
   from that chain. Byte-verified exactly as the kit describes.
5. **Handler anchors.** The STOP handler region (0x0D47 onward) contains the write of 1 to the
   stop byte at 0xDE1; the chain-to-script handler (0x1917 onward) contains the far call the kit
   cites at 0x198D and a second stop-byte write at 0x19A0. The character remap leaf at 0x7BA6
   compares its argument to 0x1F and adds 0x40 when not above it, exactly the kit's description.
   The string unpacker entry at 0x8626 has the 0x208-byte frame the kit implies.
6. **Call-graph counts.** Near-call fan-in from `E8 rel16` matches the kit: 140 callers at
   0x2B04A (frame 228 bytes), 33 at 0x278DB, 22 at 0xC7F2, 21 at 0x2ED72, 9 at 0x04636. Far-call
   segment histogram matches for 0xBA (515), 0xB0 (311), 0x709 (264), 0x2B (208). Far-call offset
   histogram for segment 0x2B matches the kit's per-helper site counts (0x25: 52, 0x2A: 42,
   0x52: 35, 0x8E: 6, 0x75: 2).
7. **Six-bit text codec** (kit section 3). Running the kit's `ecl_textcodec.py` over the copy
   recovers 98 coherent strings in ECL1 block 18 (kit: 92 to 98), block size 0x1996 (kit: 0x1996),
   and strings at the three offsets the kit cites (0x13E, 0x9B6, 0xE18). The keywords the kit uses
   to identify the auction scene all occur. Gold Box Explorer's `DaxEclFile.cs` uses the same bit
   layout and the same +0x40 remap, so the codec is corroborated by two independent sources plus
   the engine leaf above.
8. **Character record anchors.** Every `.SAV` is 285 bytes; byte 284 is 9, 6, or 12 across the
   six supplied characters, consistent with the kit's class-group reading. Every `.ITM` is a
   multiple of 63 bytes; every `.SPC` is a multiple of 9 bytes.
9. **Wall-definition and tile archives.** WALLDEF blocks are 780 bytes (a few are 1,560 or
   2,340, that is 2 or 3 wallsets). Gold Box Explorer's `DaxWallDefFile.cs` reads a wallset as
   five 156-byte slices with a ten-entry view-offset table, which is the same 156-byte period the
   kit found (section offsets 13/169/325/481/637). Tile sheet blocks in `8X8D*.DAX` are 2,257
   bytes and carry a tile count of 70 at header byte 8, as the kit states.

## 3. Verified as structure, uncertain as semantics

- **Opcode semantics at confidence 0.50 to 0.70** in the kit's table remain inferences. Handler
  addresses are right; names are not all right (see section 4, item 3).
- **Far-call resolution model.** The first audit treated `seg*16+off` as a position in
  the packed `START.EXE` image. That model misnumbered overlay entries: `0x2B:0x25` and
  `0x2B:0x2A` are entries 1 and 2 of unit `0x25`, and `0x7C:0x57` is unit `0x6F`
  entry 11. The corrected run-time stub model in the
  [fixup-aware call graph](re/callgraph_summary.md) classifies 4,012 fixup-confirmed
  far-call sites: 1,161 overlay entries (1,157 on exact Pascal prologues),
  1,790 Turbo Pascal runtime sites in segment `0xAF8`, and 1,061 other resident
  sites. Another 28 `0x9A` byte patterns lack relocation and remain candidates.
  The earlier 170 apparent stub misalignments disappear. Confirmed sites come
  from segment-word fixups; the resident segments'
  file positions remain unresolved.
- **GEO plane 3 (bytes 770 to 1025).** Bits 0, 2, 4, and 6 are set in 768, 716, 777, and 715
  cells; bits 1, 3, 5, and 7 in only 45, 32, 34, and 33. Gold Box Explorer's `GeoWallRecord.cs`
  reads this plane as four 2-bit door fields (1 door, 2 locked, 3 wizard-locked) for N/E/S/W, and
  the Mac snapshot verified live that this plane is the one the game mutates for door state. The
  kit's "feature flags, bits 0/2/4/6 = interactive" is the same bits read with less structure;
  it misses locked and wizard-locked states.
- **Wall-type to appearance.** Fractions of wall nibble values that carry a door bit on this
  build: type 13 at 0.95, 10 at 0.80, 15 at 0.79, 5 at 0.70, 11 at 0.69, 8 at 0.57, 3 at 0.55.
  That supports "some types are doors", but the kit's formula `block = (T-1)//5, face = (T-1)%5`
  is contradicted by the wallset-load evidence in section 4, item 6.
- **WALLDEF face vocabulary** ("base, window, gate, plain, door" in fixed order) is a visual
  inference by the kit; not checked here.

## 4. Contradictions and errors in the kit

1. **ECL header is misparsed.** The kit reads five 4-byte records `[word, 01, 01]` with the
   first word a "constant sentinel 0x1388" and the other four "event-handler entry points whose
   `& 0x7FFF` values are not raw offsets" (left UNCERTAIN). Gold Box Explorer and the Mac snapshot
   read the same bytes as a 2-byte prefix followed by five GOTO instructions at offsets 2, 6, 10,
   14, and 18, each opcode 0x01 with a word operand tagged 0x01, and map a virtual address `a` to
   unpacked offset `a - 0x9900 + 2`. Test on all 29 blocks (145 entry jumps): the reference
   formula puts 145/145 targets in range and on a valid opcode byte; the kit's empirical
   `label - 0x98F1` puts 143/145 in range and only 102/145 on a valid opcode. Every block has
   opcode 0x01, tag 0x01 at those five offsets. The kit's "biggest open item" is already solved in
   the MIT reference it did not consult.
2. **Opcode count.** The kit's README says "41-opcode VM" and its table covers 0x00 to 0x27
   (40 names). The dispatcher covers 0x00 to 0x3D with 61 handled opcodes. Opcodes 0x28 to 0x3D
   are undocumented by the kit. Gold Box Explorer's `Program.cs` names all 62 slots (0x1F marked
   "notsure"), including ROB, ENCOUNTER MENU, PARLAY, DAMAGE, AND, OR, SPELL, LOAD PIECES.
3. **Opcode names disagree with the reference for about half the table.** Agreements:
   0x00, 0x01, 0x03 to 0x09, 0x13 to 0x1E, 0x20, 0x25, 0x26. Disagreements (kit vs reference):
   0x02 CONTROL1 vs GOSUB; 0x0A MOVE_PARTY vs LOAD CHARACTER; 0x0B ADD_NPC vs LOAD MONSTER;
   0x0C LOOP_INIT vs SETUP MONSTER; 0x0D LOOP_NEXT vs APPROACH; 0x0E START_COMBAT vs PICTURE;
   0x0F SHOW_PICTURE vs INPUT NUMBER; 0x10 PRINT_TEXT vs INPUT STRING; 0x11 DISPLAY_WINDOW vs
   PRINT; 0x21 SET_AREA_ENCOUNTER vs LOAD FILES; 0x22 TEST_PARTY_CLASS vs PARTY SURPRISE;
   0x24 AREA_STEP_EVENT vs COMBAT; 0x27 LOAD_RESOURCE_PIC vs TREASURE. One case was settled by
   bytes here: in ECL1 block 18 the fifth entry jump lands at offset 0x73 on opcode 0x21 whose first
   byte operand is 18, the block's own map id, and the next instruction is 0x37 with three byte
   operands 1, 3, 5, which are exactly the three block ids in `WALLDEF1.DAX`. So 0x21 loads the
   area files and 0x37 loads wallsets, as the reference says; the kit's names for both are wrong
   (0x37 is not documented by the kit at all). The kit's combat/picture/print assignments should be
   treated as unverified until the handlers are re-read against the reference.
4. **String framing.** The kit says ECL strings are self-delimited by a zero code. The reference
   reads a 0x80 tag followed by a length byte. On this build 1,130 of the 1,772 strings the kit's
   scanner finds are immediately preceded by 0x80 and a byte equal to the packed length (for the
   first strings of block 18 the length bytes match the packed size exactly). The remaining finds
   are mostly sub-strings or other operand tags. Length-prefixed framing is the correct reading;
   it also fixes the "0x80+ string operand width" item the kit left open.
5. **GEO plane 2 (bytes 514 to 769) is the event plane, not a texture id.** The kit calls it a
   "per-cell backdrop/texture id" with a finite set {0x00, 0x84 to 0x8A, 0x0B}. On this build the
   plane takes 121 distinct values across the 29 maps; map 18 alone uses 13. Gold Box Explorer reads
   it as the event number (low 7 bits) with the high bit as a flag, and the Mac snapshot calls it
   the event plane. The 0x00 cells (2,834) are simply cells without an event. The kit's per-building
   stone tint keyed on this byte in `approximate.html` renders event numbers as colours.
6. **Wallset selection comes from the script, not from a fixed formula.** The 0x37 instruction
   appears 29 times across the scripts with triples such as (1,3,5), (24,25,1), (17,17,17),
   (9,20,23), and (7,8,23). The three loaded wallsets are what the wall nibble indexes. The kit's
   "wall type T selects WALLDEF block 1/3/5 by (T-1)//5" only works on map 18 because that map
   happens to load (1,3,5). Any renderer must read the triple from the area script (or from a
   table the engine builds from it).
7. **"Neighbor redundancy 480/480" is false.** `export_game.py` says the 16 by 16 four-plane
   reading was "validated vs Cluebook Podol Plaza, neighbor redundancy 480/480". Checking that a
   cell's south nibble equals the next row's north nibble and its west nibble equals the previous
   column's east nibble gives 357/480 on map 18 and 11,757/13,920 (84.5%) over all maps. One-sided
   walls and doors are normal in this engine, so the plane layout is still right (it matches the
   reference and the Mac live evidence), but the stated proof is not.
8. **Image header.** The kit describes EGA images as "u8 width in pixels plus 3 header bytes"
   and later patches this per family (an "8-byte header" for tiles, "44 bytes per row with the
   header bleeding into row 0" for portraits, a "width sweep" to find 88 pixels). Gold Box
   Explorer's `EgaBlockSpecification.cs` gives the real header: u16 height, u16 width in 8-pixel
   units, u16 x, u16 y, u8 item count, 17 bytes in all, then `height * width * 4` bytes per item.
   This formula fits every block checked here exactly: 24 by 3 by 1 item = 305 bytes (CBODY,
   COMSPR, CPIC), 48 by 11 = 2,129 (BODY), 40 by 11 = 1,777 (HEAD, which is 88 by 40 as the kit
   eventually found), 8 by 1 by 70 items = 2,257 (8X8D), 24 by 3 by 40 items = 11,537 (BACPAC).
   The kit's byte 0 is the height; it equals the width only for the square 24 by 24 sprites. The
   kit's decoded art is nevertheless usable because 4-bit chunky, high nibble first, is right.
9. **Tile sheet counts.** The kit says blocks 1, 3, 5 of the tile archives hold 70/31/45 tiles.
   On this build blocks 1, 3, 5 each hold 70; the 31- and 45-tile sheets are blocks 202 and 203
   of `8X8D1.DAX`. Block 1 is byte-identical across all eight tile archives.
10. **Item database size.** The kit says `ITEM1-4.DAX` is "the 110-item treasure database".
    Those four archives hold 271 records of 63 bytes (one 57-record block appears in both ITEM1
    and ITEM3). `ITEMS` (2,050 bytes) is not a multiple of 63 and is a different structure.
11. **Segment 0x2B is not resident.** The kit's narrative that the operand helpers live in a
    "resident 0x2B helper segment" that "self-relocates" and therefore "needs a 16-bit emulator"
    is wrong: those calls resolve to the entry stubs of overlay unit 0x25 and to ordinary Pascal
    functions inside `GAME.OVR` (including the unit's operand loader). They can be disassembled
    statically like everything else.
12. **Internal inconsistencies.** The findings file says GEO is "NOT YET CONFIRMED" in section 4
    and "fully decoded" at the end; it gives 728 strings in one place and 858 in another; the
    README says 41 opcodes while the dispatcher has 61; the ECL doc's worked example is at offset
    0x21, not "from offset 0x14"; the kit lists the three Eye of the Beholder games as Gold Box
    engine titles (they are Westwood's, not SSI Gold Box); and it calls area 18 "Slums / Podol
    Plaza" where the Mac snapshot's script-verified table has 18 = Podol Plaza and 20 = Slums.
13. **Reproducibility gaps.** The kit's `analysis/` scripts use hard-coded Windows relative paths
    (`.\games\pool-of-radiance\extracted\poolrad`). The artifacts the documents rely on
    (`overlay_segmap.json`, `unit_ledger.json`, `unit_roles.json`, `ecl_area_text.json`,
    `_ecl_ECL1_18.asm`) are not in the repository (ignored or never committed), so the unit ledger
    cannot be consumed; it has to be regenerated.

## 5. Fidelity gaps in the playable JavaScript reconstruction

The kit itself labels the browser build "approximate" and "a reconstruction". Reading
`reproduction/index.html`, `approximate.html`, `combat.js`, and `analysis/export_game.py`:

- **No script execution.** The ECL interpreter is never run. "Encounters" are triggered by
  `Math.random() < 0.11` per step after a four-step grace period; the dialog shown is a random
  line from a filtered pool of decoded strings for that map. Events, quests, flags, menus, and
  transitions driven by the real scripts are absent.
- **Invented monsters and rules.** `combat.js` uses a hard-coded four-entry monster table
  (kobold, orc, goblin, hobgoblin) with made-up AC, THAC0, and hit dice; monster count is random;
  party AC comes from a per-class constant table, not from equipment; every attacker has THAC0 20;
  initiative is a DEX sort; movement is Manhattan steps; loot is a random pick from the item
  records. The `MON*CHA`/`MON*ITM`/`MON*SPC` monster archives are not used. `approximate.html` is
  simpler still: flat 1d8 damage, fixed monster HP, coin-flip retaliation.
- **Placeholder systems.** Spells are literal string lists chosen by class name; casting prints a
  message; REST adds a flat 3 HP; party start position is found by a heuristic corridor search
  rather than the script's placement; levels are named "GEOn #id" with no area identity; wall
  faces use a texture tint keyed on the event plane (section 4, item 5); the class of each
  character is inferred by heuristics on stats and item names.
- **Art pipeline caveat.** Pixel data is decoded correctly (4-bit chunky), but image dimensions
  are obtained by per-family hacks instead of the 17-byte header (section 4, item 8).

Net: the reconstruction is a validated art and map viewer with a demo combat layer. It is not
evidence about game rules, encounter design, or script behavior, and nothing from it should be
ported as a behavioral contract.

## 6. What this means for the roadmap

- **Stage 0.** Use the hashes in section 1 as the oracle identity. The kit's structural
  fingerprints match, so its overlay offsets can be imported as hypotheses keyed to these hashes.
- **Stage 1.** Port the DAX reader and RLE from Gold Box Explorer or the kit (both MIT, both
  verified here). Take the GEO plane semantics, the 17-byte image header, the WALLDEF slice
  layout, the ECL entry-jump header, the 0x9900 address base, and the 0x80-length string framing
  from Gold Box Explorer, not from the kit. Expect one-sided walls; do not "repair" them.
- **Stage 2.** The kit's overlay unit map, VM loop, dispatcher, 61 handler addresses, and
  call-graph anchors are byte-verified and can seed the address ledger. Its opcode names must be
  re-derived handler by handler, cross-checked against Gold Box Explorer's 62-entry table, and
  ideally against live traces. Drop the "resident helper, needs an emulator" framing: the operand
  helpers are in overlay unit 0x25. Replace the far-call model with one built from the fixup
  tables.
- **Stage 3 onward.** Nothing in the JS reconstruction's combat, encounter, rest, or spell code
  should be reused.

## 7. How the checks were run

Python 3 against the files in this copy, with the kit's `kit/dax.py` and
`analysis/ecl_textcodec.py` imported unchanged, capstone 5 in a scratch virtual environment for
the 16-bit disassembly, and plain byte scans for everything else. Counts in this document are
reproducible from the descriptions above; no derived data files were kept in the repository.
