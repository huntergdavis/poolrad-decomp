# One original-DOS level-byte edit

Changing the first tested fighter's byte `0x98` from `01` to `02` changes
the original game's displayed LEVEL from **1 to 2** and THAC0 from **20 to
19**. STR stays **16**, EXP stays **32**, and HP stays **11**. This is one
observed edit/load case, not a coherent level-up implementation. General
legal levels, class-to-slot mappings, experience requirements, hit-point
advancement, and spells remain UNVERIFIED.

## Evidence and reproduction

The static trace in [DOS_CHARACTER_DISPLAY_FIELDS](DOS_CHARACTER_DISPLAY_FIELDS.md)
establishes eight ordered level-display operands at `0x96..0x9D` through
DOS unit `0x77`, `+0x0314..+0x03B4`. Only index two (`0x98`) is nonzero in
this character. Its sheet identifies a fighter; this single example does
not establish the class mapping for all eight slots.

The control is the completed two-member slot-B STR-16 run
`resources/oracle-runs/chared-t174-party-edit/`. The new run is
`resources/oracle-runs/chared-t174-level-edit/`. The control completed
before submission of the level experiment. Both use the original disk-load
sequence `Return`, `l`, `b`, `v`, with no emulator savestate restore. VP
authorized the separate isolated experiment in inbox `i26761`.

Each run has 160 game-installation files: 153 copied originals plus seven
assembled slot-B files. An independent post-run comparison found exactly
one differing file, `CHRDATB1.SAV`, and exactly one differing byte:

| Offset | Before | After | Reason |
| --- | --- | --- | --- |
| `0x98` | `01` | `02` | Test the existing active level-display slot |

The character hashes are:

- Before: `e29e16f7dfc4abb10ca25fc701166bab71f24e402276390fd6e202d8db5ccf5a`
- After: `92d1129e9b37bb26bd705ad6602a5eafad7d9e7aa9d86c8ce3e5559e8deefe6f`

All sidecars, world files and executables are unchanged between the runs.
All **153 original source-file hashes** still match the pre-experiment
copy receipts. The original save files were never edited.

The actual isolated process executable and mount arguments were recorded.
The owner DOSBox-X 2026.10.01 executable SHA-256 is
`a11d55f0cb9bf10e36e69d8df27c3bd6bc303f5aacde32f3e7cdc96c3ebfe967`.
The pinned original `GAME.OVR` hash is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and `START.EXE` is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The level run used its own Xvfb display, tmux socket, copied writable game
tree and the heavy-job semaphore; it exited zero. It did not control the
owner's desk, Play pane, or live game.

## Visual observations

The private control sheet is `party-edit-character-sheet.png`, SHA-256
`c673e4526283a31f983cc0136e8febc2b2511164cad5c6707c0a3353a066c0e2`.
The private edited sheet is `level-edit-character-sheet.png`, SHA-256
`d6ad6140df6d78a63ebdc0606371102790a1d8b92edc1ca3662b82b9d73301c8`.
Both are inside their respective run directories above.

| Sheet value | Control | Edited |
| --- | ---: | ---: |
| STR | 16 | 16 |
| LEVEL | 1 | 2 |
| EXP | 32 | 32 |
| HP | 11 | 11 |
| THAC0 | 20 | 19 |

The other five displayed abilities remain 12, 12, 17, 16, 15. No spell slots
are visible on this fighter sheet. No spell menu or casting test was run;
unchanged sidecar bytes do not prove spell correctness. The THAC0 change is
an observed sheet effect, not a combat-roll test. Unchanged EXP and HP
explicitly prevent claiming that this edit performs a complete level-up.

## Non-display consumers and limits

Prior recovered-code evidence also shows non-display uses of the eight
bytes. [U9B_ENTRY_ZERO_COMPOSITION](../re/U9B_ENTRY_ZERO_COMPOSITION.md)
describes entry `0x0000` reading `record[0x96 + index]`, indexing DS tables,
and updating other record fields, including `0x2D`, `0x73`, `0xA1`, and
`0xB2..0xB4`. [U6C_RECORD_TIER_3FB3](../re/U6C_RECORD_TIER_3FB3.md)
describes a sum/count computation over the same eight bytes. These static
consumers and the observed THAC0 change rule out describing the level
block as only cosmetic. They do not establish the full advancement rules
or attribute this particular THAC0 change to a specific instruction path.

Private `resources/chared-t174/level-game-proof.json` records the complete
comparison result and observations. The run's
`chared-level-edit-receipts.json` records each installed file, before/after
hashes, the changed offset/bytes, and the build that loaded the copy.
Process identity, input JSONL, intermediate screenshots and final captures
remain under ignored `resources/`; no original bytes or character names
are committed here.

`ran run_media_checks.sh: not run`; this documentation-only slice uses the
160-file comparison, 153 original hash checks, actual process identity,
and paired original-game screenshots. It does not validate arbitrary
levels/classes, XP/HP/spell recalculation, combat, web export, or C parity.
No function-accounting credit is requested.
