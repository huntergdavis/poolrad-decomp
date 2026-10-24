# The RES_00BA_16EC closure: two-factor identity arguments (par-a, 2026-10-24)

These arguments follow the two-factor standard (VP ruling recorded in #1195): factor 1 is a call site inside a
`game_evidenced`, `game_verified` or strict function, and factor 2 is the body operating on documented game state. They
cover the keystone `RES_00BA_16EC` (t24) and the three uncredited resident callees in its closure that par-d2 named on
main `6d369fe3`. This note decides identity only. Credit comes with each body's own strict evidence.

| Body | Factor 1 | Factor 2 | Verdict |
| --- | --- | --- | --- |
| `RES_00BA_16EC` | 63 sites in 53 `game_evidenced` functions | reads `DS:49F3`, the saved-game mode byte | **PASS** |
| `RES_0709_0768` | 27 sites, 4 of them in `game_verified` functions | reads and writes the text cursor `DS:5E24/5E25` | **PASS** |
| `RES_0709_0BF5` | 10 sites, 2 of them in `game_verified` functions | argument-borne: the save-disk prompt built from `DS:537A` | **PASS** |
| `RES_03F1_0000` | 3 sites, all in `game_evidenced` functions | no game cell; coordinates, style and a constant tile base | **FAIL** |

## Reproduction

Each command reproduces one factor:

```
python3 tools/overlay/far_call_sites.py 00BA:16EC     # factor 1; likewise 0709:0768, 0709:0BF5, 03F1:0000
python3 tools/overlay/resident_ds_operands.py 00BA:16EC:17E3 0709:0768:09E3 0709:0BF5:0C3F 03F1:0000:0137   # factor 2
```

- `far_call_sites.py` (#1510) prints every relocated `LCALL` in GAME.OVR, keyed by stub paragraph and printed in hex.
- `resident_ds_operands.py` is new here. It decodes the hash-checked, loader-expanded START.EXE image (`a42a57e6…`) and prints
  each body's DS-absolute operands and calls. `test_resident_ds_operands.py` pins the exact cell sets.
- Owner classes come from `overlay_function_ledger.csv` (body ranges) and `overlay_classification.csv`.
- The spans are the measured ones: 16EC from par-d2's decode (i23380), and 0768 from
  [RES_0709_PRINTER_0768_CPU.md](RES_0709_PRINTER_0768_CPU.md). 0BF5 is `resident_0709_msgbox_0bf5_cpu.h`'s span and
  03F1:0000 is [RES_03F1_BOX_0000_CPU.md](RES_03F1_BOX_0000_CPU.md)'s.

## `RES_00BA_16EC` (247 B, `00BA:[16EC,17E3)`, `RETF 0Ch`)

**Factor 1: PASS.** 63 relocated sites. Every owner is `game_evidenced`, across units 1Eh, 2Fh, 31h, 38h (21 handlers),
5Eh, 6Ah, 77h, 86h and 9Eh. Examples: `OVL_009E_1344` (2 sites), `OVL_006A_18E5` (4) and `OVL_005E_02F9` (4). No
resident far site exists. par-d2 found four resident near-call byte patterns inside 00BA (0A1A, 208E, 218F, 239A), which
are not confirmed as reached code and not needed here.

**Factor 2: PASS. 16EC reads the saved-game mode byte itself.**
- `+1707 cmp byte ptr [49F3h], 5` chooses between the two window layouts.
- [DOS_SAVGAM.md](../formats/DOS_SAVGAM.md) documents `DS:49F3` as the mode byte, loaded from SAVGAM offset `0x3207`.
- [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) shows the program body writing it (`[49F3] = 4` at `+0x32FE..+0x331C`, `= 3` for
  maps 19h..1Bh) and gating the saved-game return on it.
- `+1757 cmp byte ptr [6DDAh], 0` selects the box row (12h or 11h). `DS:6DDA` is not documented, so the argument does
  not use it.

**Role line:** the game's message window. It copies the caller's string into its frame. For mode 5 it clears the
window, positions with `1816` and prints with `0768`. Otherwise it also draws the tile box (`03F1:0000`) when
`[6DDA] = 0`, and uses rows 11h/12h. When the delay flag `[BP+6]` is set, it waits (`0709:0DBE`) and clears (`17E3`).

## `RES_0709_0768` (635 B, `0709:[0768,09E3)`, `RETF 10h`)

**Factor 1: PASS.** 27 relocated sites in 16 functions, including the `game_verified` `OVL_001C_0A09`, `0F08`, `1EF8`
and `2775`. 16EC's own two sites (00BA:1750, 17C9) and 00BA:0AFB are resident far sites. They are not counted.

**Factor 2: PASS. 0768 reads and writes the game's text cursor.**
- 18 operands touch `DS:5E24/5E25` and no other DS cell. Examples: `+079E` reads the column, `+095B inc byte ptr [5E25h]`
  wraps to the next line, and `+0958/+0979` store the column.
- [ECL_CONTRACTS.md](ECL_CONTRACTS.md) (`0x33 PRINT RETURN`) and FUNCTION_LEDGER function 10 document this cursor as
  the ECL script's print position, as [RES_0709_PRINTER_0768_CPU.md](RES_0709_PRINTER_0768_CPU.md) cites. This is state
  that the game's scripts drive.

**Role line:** the game's text-window printer. It word-wraps a Pascal string into the window `(x1,y1)..(x2,y2)` at
the cursor `DS:5E24/5E25`, and calls the message prompt `0BF5` when the window fills.

## `RES_0709_0BF5` (74 B, `0709:[0BF5,0C3F)`, `RETF 6`)

**Factor 1: PASS.** 10 relocated sites. The owners are the `game_verified` `OVL_001C_19B0` and `OVL_001C_2775`, plus
`game_evidenced` functions in units 20h, 25h and 6Fh (`01B8`, `02AE`, `069E`, `0EC4`, `1D58`). One site,
`OVL_03D2_0004`, is in an unresolved owner, and the argument does not use it.

**Factor 2: PASS, on the argument-borne clause only (#1363/#1365).** The body reads no DS cell. It copies its message
argument, clears row 18h (`05EB`), prints the message (`0626`) and returns a key (`0899:0059`). The clause needs the
passed object named for one direct-entry caller, plus the document that defines it:
- **Caller** `OVL_006F_01B8` (`game_evidenced`), site `+0234`, builds the message at `+0206..+0231` as
  `'Put save disk in '` (`CS:01A4`) + the byte `[537Bh]` (`+021C mov al,[537Bh]`, converted by SYSTEM `0AF8:0C3B`) + `':'` (`CS:01B6`).
  Three more unit-6Fh sites (`+031A`, `+08A4`, `+1E58`) push `[537Bh]` into the same SYSTEM `0C3B`/`0B98` concatenation before calling 0BF5.
- **Document.** [SLOT_A_NATIVE_1983_FILENAME.md](SLOT_A_NATIVE_1983_FILENAME.md) measures `DS:537A` as the live first
  component of the save filename, a Pascal string. [SLOT_A_NATIVE_19E7_LOOP_GATE.md](SLOT_A_NATIVE_19E7_LOOP_GATE.md)
  pins its length byte. So `[537B]` is that path's first character, the save drive.

The `game_verified` 001C sites, and the 20h and 25h sites, pass compiled `CS:` literals. Under the constants rule they
do not carry factor 2.

**Role line:** the message-line prompt. It shows one message on row 18h and waits for a key, which it returns in AL.

**Limit:** this identity rests on one game cell carried in the message. It does not rest on anything the body
dereferences.

## `RES_03F1_0000` (311 B, `03F1:[0000,0137)`, `RETF 0Ah`): FAIL on factor 2

**Factor 1 passes.** It has three relocated sites, in the `game_evidenced` `OVL_006A_04A1`, `OVL_0077_004A` and
`OVL_0077_0EF8`, plus 16EC's resident site 00BA:177B.

**Factor 2 fails.**
- The body has no DS-absolute operand.
- Its arguments are five coordinate and style bytes. Its tile base is the constant `114h + 3*style`. All eight calls go
  through stub `0428:002A` to `OVL_040C_015D`.
- This is the 06FB precedent: a display service with no memory operand, where constants are not game state and argument
  use alone is never enough.
- 16EC's one call passes only constants: `(0, 16h, 26h, 11h, 1)`.

**Consequence.** 16EC reaches `03F1:0000` on the in-domain branch `[49F3] <> 5` with `[6DDA] = 0`.
- Under the strict-scope rule (par-e, i22846), a stop there makes a 16EC model incomplete.
- The readiness tooling counts an uncredited callee as unready.
- So 16EC cannot be strict while 03F1:0000 is unresolved.

The VP was asked (i23386) whether a fully modelled, non-game display helper may compose like a ready dependency without
credit. That would be a sibling of [DRIVER_INTERNAL_CLASS.md](DRIVER_INTERNAL_CLASS.md), whose membership the code
restricts to the 08AE driver. This note records the refusal under current precedent, and the ruling is still open.

A 16EC model counts toward REPLACED whatever the identity ruling. par-d2 measured 16EC arrivals in par-b's observer
tables (i23399): 109 per pinned R3 run, 109 on R1, 2 on R2, and 0 on R4 and the walks.
