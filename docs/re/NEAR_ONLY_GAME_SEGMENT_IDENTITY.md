# Near-only game-segment residents: two-factor identity verdicts (par-a, 2026-10-21, VP i20391)

These are the 17 game-segment near-only residents that [REVIEWED_NEAR_ONLY_ALIAS.md](REVIEWED_NEAR_ONLY_ALIAS.md) lists without an argument: 00BA 9, 0464 5, 0709 3. Each is judged under the [two-factor standard](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20) and, where a chain is needed, the [t30 chain ruling](RES_0802_IDENTITY_ARGUMENTS.md#rulings-vp-i20361-2026-10-21).

**How this was measured** (main `1bf35dde`):
- The bodies come from `resident_callable_ledger.csv`: the near-only rows are those with 0 overlay far sites, 0 loader far sites and at least 1 decoded near site, 39 in all. Segment is assigned by image offset.
- Every span was decoded from the loader-expanded START.EXE (`resident_loader_map.unpack`), using the first return that no earlier branch jumps past.
- Every near `call` in every ledger body's span was attributed to its containing body. That body's status was read from `strict_full_c_addresses.csv` and `resident_inventory.GAME_SPECIFIC_TARGETS`.

This page is an argument, not a credit. par-e lands any credit.

**Result: 8 PASS, 9 BLOCKED, 0 FAIL.**

## PASS: the eight nested bodies of credited `RES_00BA_0BB8`

**Factor 1 is direct, not a chain.** Each body has a near site inside a **strict-credited** resident, and each site lies inside that caller's span: `0BB8` [0BB8,1037) and `13AB` [13AB,148B), both credited through [U9E_CHARACTER_PAIR_115C_CPU.md](U9E_CHARACTER_PAIR_115C_CPU.md). That document already pins all eleven bodies of this resident closure by span and SHA-256. These eight were never credited only because they are near-only.

**Factor 2 is the character object, passed as an argument and used.**
- `0BB8` dereferences its own far-pointer argument `[bp+6]` as the 285-byte (`0x11D`) character object of [DOS_CHARACTER.md](../formats/DOS_CHARACTER.md). It reads `+2Dh`, `+102h`, `+110h`, `+119h`, `+11Ch`, `+CCh/+CEh`, and its list head `+C8h/+CAh`, which DOS_CHARACTER documents as an in-memory far-pointer slot.
- It passes that pointer unchanged (`push [bp+8]; push [bp+6]`) to each body below.
- To `01FB` and `0282` it also passes the current record X of the `+C8h` list. Those are the "63-byte linked records" (`+2Ah/+2Ch` links) that the credited 115C doc names.
- Every field offset these bodies use is below `0x11D`.

| Body (ledger id) | Bytes | Site(s) in credited caller | Role line (game state touched) |
| --- | ---: | --- | --- |
| `RES_00BA_0000` (`RPRO_00BA0`) | 507 | `0BB8+0ED4` | recomputes character bytes `+2Dh..+32h`, `+110h..+119h` and `+CCh` through the DS tables `558Bh..5590h`, follows the object's `+F8h/+FCh` pointers, and calls `1168`, `1282` and `1325` |
| `RES_00BA_01FB` (`RPRO_00D9B`) | 135 | `0BB8+0F09` | applies one flagged (`+34h`) linked record X to the character's `+2Eh`, `+32h`, `+37h`, `+72h` and `+11Ch`, through DS table `5582h` |
| `RES_00BA_0282` (`RPRO_00E22`) | 286 | `0BB8+0F23` | reads linked record X and character `+101h` through DS tables `5582h/5588h`, and returns two bytes through 0BB8's locals (`retf 10h`: two locals, X, character) |
| `RES_00BA_03A0` (`RPRO_00F40`) | 141 | `0BB8+0F8B` | updates character `+11Ch` from `+102h`, then calls credited `13AB` |
| `RES_00BA_10E6` (`RPRO_01C86`) | 130 | `0BB8+0E77` | derives a byte from character `+13h`, which 0BB8 keeps in its local |
| `RES_00BA_11EA` (`RPRO_01D8A`) | 152 | `13AB+13B8` (credited); also `1282+1293` and `1325+1336` | reads character `+10h/+16h`, the pair fields the credited 115C doc names |
| `RES_00BA_1282` (`RPRO_01E22`) | 163 | `0BB8+0E93`; also `0000+00A6` | clears character `+AAh`, then calls `11EA` |
| `RES_00BA_1325` (`RPRO_01EC5`) | 134 | `0BB8+0EB4`; also `0000+00C7` | clears character `+AAh`, then calls `11EA` |

**Limits:**
- The role lines name the fields touched, not their gameplay meaning; DOS_CHARACTER.md does not name most of these bytes.
- The DS lookup tables `5582h..5590h` are read as tables, not documented by meaning.
- Where the evidence bar stands is not judged here. The existing 115C audit (3,441 cases, 33 native captures, with its stated RAM-subset and normal/386 limits) is par-e's to re-derive.

## BLOCKED: each is called only from an uncredited resident whose own identity is not established

The chain ruling needs the entry's identity to be **direct**. None of the entries below has a two-factor argument, so these nine are blocked on a named prerequisite. They do not fail.

| Body (ledger id) | Bytes | Only callers (sites) | Blocked on |
| --- | ---: | --- | --- |
| `RES_00BA_272A` (`RPRO_032CA`) | 170 | `00BA_27D9` x5 (`2817`, `2840`, `287C`, `289E`, `28C0`) | `RES_00BA_27D9` identity. It has 28 overlay far sites, and GAMEPLAY_PATH §3 shows the gameplay loop calling it, so it is a promising direct entry. |
| `RES_0464_0000` (`RPRO_04640`) | 123 | `0464_02A5` x2 | `RES_0464_02A5` identity (1 overlay far site) |
| `RES_0464_007B` (`RPRO_046BB`) | 84 | `0464_02A5` x4, `0464_1E5C` x2, `0464_2632` x1 | `02A5`, `1E5C` or `2632` identity. **1E5C is on the parked 0482 chain**, so `02A5` or `2632` is the route. |
| `RES_0464_1484` (`RPRO_05AC4`) | 133 | `0464_15C7` x2 | `RES_0464_15C7` identity (1 overlay and 1 loader far site) |
| `RES_0464_1509` (`RPRO_05B49`) | 141 | `0464_15C7` x2 | the same (reads `DS:25E5..25F3`) |
| `RES_0464_1596` (`RPRO_05BD6`) | 49 | `0464_15C7` x2 | the same |
| `RES_0709_0000` (`RPRO_07090`) | 113 | `0709_00B7` x1 | `RES_0709_00B7` identity (loader-called only, 2 sites) |
| `RES_0709_06A7` (`RPRO_07737`) | 109 | `0709_0768` x3 | `RES_0709_0768` identity (27 overlay far sites). The body reads `DS:49E2`, `4A00` and `5E24/5E25`. |
| `RES_0709_0714` (`RPRO_077A4`) | 58 | `0709_0768` x1 | the same |

**Unblocking order by reach:** `27D9` (1 body), `15C7` (3), `0768` (2), `02A5` or `2632` (2), `00B7` (1). Each entry needs its own direct two-factor argument first. Its nested bodies then need role lines under the chain ruling.

## On the 0040 ruling (VP i20361, ruling 2)

**Agreed, on a sharper ground than the one stated.** Decoded from the media:
- `RES_0464_2A0D` (61 B) reads **no** DS state. It writes the Registers record from its two arguments (a palette index, plus 8 at 8 and above, and a colour) and calls `Intr($10)` with AH=10h: a thunk.
- `RES_0802_0040` (197 B) reads the game's draw and active pages `DS:4A1A/4A1B` and the EGA flag `DS:4A1E`. All three are set by the game's video init, with native stops in [RESIDENT_VIDEO_INIT_4426.md](RESIDENT_VIDEO_INIT_4426.md). It maps the attribute through the colour table `DS:25D5`, which strict-credited `RES_0709_01DF`'s doc documents, and repeats the draw on the second page when the flag is set.

"Uses game-supplied arguments" is **not** a sufficient ground on its own: 2A0D also uses its arguments. Resting the ruling on that clause would reopen 2A0D. The operative difference is that 0040 reads game-maintained display state and 2A0D reads none.

## Pass 2: the blocking entries, then their nested bodies (par-a, 2026-10-21, VP i20396)

Measured on main `1bf35dde` and later.
- **Far callers** come from `callgraph.json` edges of kind `far_fixup_confirmed` with `resolution: resident`, attributed by `src_func`. Classification and credit are read from `classification.csv` and the two credit CSVs.
- **Spans** come from the loader-expanded START.EXE.

### Entries

| Entry | Bytes | Factor 1 (fixup-backed overlay callers) | Factor 2 (documented subject) | Verdict |
| --- | ---: | --- | --- | --- |
| `RES_00BA_27D9` | 281 | 28 sites in 22 functions, including **`game_verified` `001C_0C74` (+0DD4), `001C_179B` (+18EF) and `001C_2E10` (+2E4F)**, and the gameplay loop `001C_32F8` (+3378) | Branches on the game mode byte `DS:49F3` ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) §2 and §3; [RESIDENT_ROUTE_290B.md](RESIDENT_ROUTE_290B.md)). Reads the ECL-saved words `DS:49FD/49FE` ([ECL_HOOKS_6E7D.md](ECL_HOOKS_6E7D.md)). Loads the active-character pointer `DS:5D92/5D94`, which it passes to `0767`. | **PASS.** [RESIDENT_LOAD_ROUTE_27D9.md](RESIDENT_LOAD_ROUTE_27D9.md) left game-versus-library ownership open; this settles it. That doc's model is bounded, so credit still needs a full translation. |
| `RES_0709_0768` | 635 | 27 sites in 16 functions, including **`game_verified` `001C_0A09` (x2), `001C_0F08`, `001C_1EF8` (x6) and `001C_2775`** | Reads and writes the active-window cursor bytes `DS:5E24/5E25`, which [ECL_CONTRACTS.md](ECL_CONTRACTS.md) (`0x33 PRINT RETURN`) and [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) (function 10) document | **PASS** (the game's text-window printer). It is port-bound, and `0DD3`, `06A7` and `0BF5` are not yet modelled. |
| `RES_0464_02A5` | 1,583 | **One** site, `03B9_00A2+0400` (`game_evidenced`, inside its bound of 1,005 B) | Reads the EGA flag `DS:4A1E`, which the game's video init sets ([RESIDENT_VIDEO_INIT_4426.md](RESIDENT_VIDEO_INIT_4426.md)), and the pixel-word table `DS:25E5`, which strict-credited `RES_0709_01DF`'s doc documents. This is the 0040 ground: it reads game-maintained display state. | **PASS, thin.** There is a single caller. The tables `25F6` and `25FA` are not documented. |
| `RES_0464_15C7` | 906 | One site, in `03CE_04FC`, which is classified `game_or_other_library_unresolved` | n/a | **BLOCKED on `OVL_03CE_04FC` ownership.** It has no game-code caller today. |
| `RES_0464_2632` | 987 | Six sites, in `006C_34DF` and `03BD_0174`, both `game_or_other_library_unresolved` | n/a | **BLOCKED on the ownership of those two callers.** |
| `RES_0709_00B7` | 186 | Loader-called only (2 loader sites) | n/a | **Not assessed in this pass.** |

### Nested bodies, under the chain ruling

| Body | Entry | Verdict |
| --- | --- | --- |
| `RES_00BA_272A` | 27D9 | **FAIL, factor 2 (06FB's shape).** It reads only a 0/1 flag that 27D9 passes as a constant, loads four constant strings through `0AF8:0B0B` and prints them through `03F1:02E8`. There is no DS state and no dereferenced game argument. |
| `RES_0709_06A7` | 0768 (its only caller, 3 sites) | **PASS.** Role: draws the next character of 0768's message buffer through glyph `01DF` at the window cursor `DS:5E24/5E25`, advances `[5E24]`, and applies the `[49E2]*10` delay when `[4A00]` is set. |
| `RES_0709_0714` | 0768 (its only caller) | **BLOCKED.** It only skips blanks in 0768's local copy of its string argument. No document says which text the game passes to 0768, unlike 01DA's documented key list. Unblock by documenting one `game_verified` caller's text. |
| `RES_0464_0000` | 02A5 (its only caller) | **FAIL, factor 2.** It does pure nibble arithmetic on stack values: a bitmask of which of the eight nibbles at `[bp+6..9]` equal `[bp+0Ah]`. There is no DS state, so it is a display computation in the 2A0D domain. |
| `RES_0464_007B` | 02A5, 1E5C, 2632 | **BLOCKED.** It fails chain condition (b): it is also reachable from `1E5C` (parked 0482 chain) and `2632` (blocked). |
| `RES_0464_1484`, `1509`, `1596` | 15C7 | **BLOCKED** on 15C7, which is blocked on `03CE_04FC`. |
| `RES_0709_0000` | 00B7 | **BLOCKED** on 00B7, which is not assessed. |

**Pass-2 tally for the original nine:** 1 PASS (`06A7`), 2 FAIL (`272A`, `0464_0000`), 6 BLOCKED.

**Entries:** three now carry identity arguments (`27D9`, `0768`, `02A5`), but none is a credit yet. Each still needs a full translation at the bar.

## Pass 3: the ownership of `OVL_03CE_04FC`, 15C7's second entry, and 0714 (par-a, 2026-10-21, VP i20399)

### Entries

| Entry | Bytes | Factor 1 | Factor 2 | Verdict |
| --- | ---: | --- | --- | --- |
| `OVL_03CE_04FC` (entry slot 5) | 324 | 10 fixup-backed sites. **6 are in `game_evidenced` functions:** `03B4_0000` (+00A9), `03C6_0127` (+0239, +02CA), `03C6_02D5` (+0395), `03C6_0996` (+0A71) and `03C6_0AF8` (+0BD7). The other 4 are in unresolved `006C_34DF`. | Branches on the EGA flag `DS:4A1E` at `+05B7` (the 0040 ground: game-maintained display state, [RESIDENT_VIDEO_INIT_4426.md](RESIDENT_VIDEO_INIT_4426.md)). It also forwards the offscreen-object pointer `DS:4A20/4A22`, which [SLOT_A_RES464_SECOND_FLUSH.md](SLOT_A_RES464_SECOND_FLUSH.md) measures natively; forwarding alone is not counted. | **PASS** |
| `RES_00BA_1892` | 332 | 8 fixup-backed sites in `game_evidenced` `005E_26B0` (x5) and `005E_3558` (x3) | Branches on `[4A1E] == 1` at `+18A9`, and dereferences the bitmap object at far pointer `DS:6E36` (`+11h`, `+13h`, `+17h`; [RESIDENT_464_BITMAP_RELEASE.md](RESIDENT_464_BITMAP_RELEASE.md)) | **PASS** |

### Nested bodies

| Body | Entry | Verdict |
| --- | --- | --- |
| `RES_0464_15C7` (906 B) | Its **only** callers are `04FC` (+0592, overlay fixup) and `1892` (image `0x24A1`, loader-relocated `lcall`). Both are argued above, so chain condition (b) holds. | **PASS.** Role: mirrors a bitmap object's pixel data (`es:[di+13h/15h/17h]`), choosing the method by the EGA flag `DS:4A1E` at `+1604`. |
| `RES_0464_1484` (133 B) | 15C7 | **FAIL, factor 2 (2A0D domain).** It reverses the bits of its byte argument; no DS state. |
| `RES_0464_1509` (141 B) | 15C7 | **FAIL, factor 2.** It reverses the 2-bit groups of its word argument, using the constant masks `DS:25E5..25F3`. That is a static mask table, not game-maintained state. |
| `RES_0464_1596` (49 B) | 15C7 | **FAIL, factor 2.** It swaps the nibbles of its byte argument; no DS state. |
| `RES_0709_0714` (58 B) | 0768, its only caller | **PASS, with a limit (the 01DA shape).** It skips blanks in 0768's local copy of its string argument. For the `game_verified` caller `001C_0A09` (+0A69, +0A91), that argument is far `DS:70C8`, the ECL **string buffer 1** ([FUNCTION_LEDGER.md](FUNCTION_LEDGER.md); [U1C_PRINT_0A09.md](U1C_PRINT_0A09.md) records `0709:0768` with far `DS:0x70C8`). **Limit:** only that caller's text is documented; `001C_1EF8` passes stack buffers and a CS constant, which are not claimed. |

**Still blocked:**
- `RES_0464_007B`: chain condition (b). It is also reached from `1E5C` (parked) and from `2632`, whose callers `006C_34DF` and `03BD_0174` remain unresolved.
- `RES_0709_0000`: behind `00B7`, which is not assessed.

**Running tally for the original 17:**

| Verdict | Count | Bodies |
| --- | ---: | --- |
| PASS | 11 | the 0BB8 eight, `06A7`, `0714`, `15C7` (15C7 is not one of the 17 but now argued) |
| FAIL | 5 | `272A`, `0464_0000`, `1484`, `1509`, `1596` |
| BLOCKED | 2 | `007B`, `0709_0000` |

Counted strictly over the 17: **10 PASS** (the eight, `06A7`, `0714`), **5 FAIL** and **2 BLOCKED**.

**Entries with identity arguments, still needing translation:** `27D9`, `0768`, `02A5`, `04FC`, `1892` and `15C7`.
