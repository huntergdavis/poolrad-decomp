# Unit 0x3B6: two-factor identity arguments (par-a, 2026-10-23)

Same standard as [U7F_IDENTITY_ARGUMENTS.md](U7F_IDENTITY_ARGUMENTS.md). Every claim was checked against a fresh decode of the
original unit `3B6h` and `3B4h` bytes from GAME.OVR.

## `OVL_03B6_0000` (519 B, `RETF 4`), evidence PR #1508

**Factor 1: PASS, through a relocated far call from a `game_evidenced` caller.**
- **Site:** `3B4:+0293` is `lcall 03CD:0025`, fixup-backed (`push ax` at `+0292` passes the record index). It is inside
  `OVL_03B4_016D`, which is `game_evidenced`, grade A. `03CD:0025` is unit `3B6h`'s stub for entry `+0000`.
- **Not relied on:** unit `75h` `+06B3` also far-calls the same stub, at sites `+06CA`, `+07D9` and `+08DC`, but `OVL_0075_06B3` is
  `game_or_other_library_unresolved`.

**Factor 2: PASS. The body reads two documented game-state structures itself.**
1. **The 44-column map at `DS:3648`.** At `+0049`, `mov al,[di+3648h]` reads the class byte `DS:[3648h + 2Ch*y + x]`, the map
   that [U33_GRID_DRIVER_11B4.md](U33_GRID_DRIVER_11B4.md) documents (lines 33 and 50).
2. **The window-0 ECL memory at far `DS:49D2`.** The body loads it with `les di,[49D2h]` and reads six words:
   - `+318h` at `+00BD`;
   - `+3A4h` at `+00E7`;
   - `+2F8h` at `+00F3`;
   - `+33Eh` at `+0103`;
   - `+340h` at `+012C` and `+0174`;
   - `+366h` at `+01A4`.

   [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) (line 227) documents the window: ECL addresses `4900h..4CFFh`, through `[49D2h]`,
   allocated by `GetMem(800h)` in unit `36h`. Records 3..9 of the body's dispatch are gated on these words.

**Role line:** a per-record dispatch over the tile and map state. It classifies a map cell from `DS:3648` and gates records 3..9 on
window-0 ECL words. Its one far call (`+01E1`, `lcall 03E9:002A`) goes to unit `3CEh`'s stub; the composed body is `03CE_00F1`.

**Limits:**
- the meanings of the window-0 words are not named; the identity rests on their being documented ECL memory, not on their meaning;
- `03CE_00F1` is `game_or_other_library_unresolved` and stays evidence and readiness only, as #1508 states;
- `RES_0464_08D4` is uncredited.
