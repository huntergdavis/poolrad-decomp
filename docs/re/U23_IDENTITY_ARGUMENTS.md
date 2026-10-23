# Unit 0x23: two-factor identity arguments (par-a, 2026-10-23)

Same standard as [U7F_IDENTITY_ARGUMENTS.md](U7F_IDENTITY_ARGUMENTS.md). Every claim was checked against a fresh decode of the
original unit `23h` and `20h` bytes from GAME.OVR.

## `OVL_0023_0239` (code `[0239,034F)`, `RETF 8`), evidence PR #1490

**Factor 1: PASS, through two near calls from a `game_evidenced` caller.** The sites are `+03DD/+03DE` and `+0426/+0427`, both
`push cs; call 0239`, inside `OVL_0023_0361` (`game_evidenced`, grade B, extent `[0361,053E)`).

There is also a far site in unit `20h`, at `+0C60`: `lcall 0028:002A`, fixup-backed. It is inside the code that follows the
picker call to `20:0B14` at `+0C3B`. The function holding `+0C60` is not separately listed in the queue export used here, so
this argument does not rely on it.

The near caller pushes (`+03D2..+03DC` and `+041B..+0425`). In 0239's frame:

| 0239 slot | What 0361 passes |
| --- | --- |
| `[bp+0Ch]:[bp+0Ah]` | `[bp-2]:[bp-4]`, the item record 0361 holds. 0361 reads its `+3Ah` word at `+03B0..+03B7` |
| `[bp+6]` | `ss:&[bp-0Fh]`, a refusal flag (out-parameter) |

**Factor 2: PASS. The body dereferences the active-character record itself.**
- At `+027B..+028C` it does `les di,[5D92h]` and reads the dword at `+C8h`/`+CAh`. That is the head of the active character's item
  list, through the active-character pointer ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md)).
- **Empty list:** if the head is nil, it allocates 3Fh bytes (`0AF8:023F`, GetMem) and copies the item into them (`0AF8:0A3F`,
  Move, `+02A6..+02B4`). It clears the copy's link `+2Ah`/`+2Ch`, then stores the copy as the new head through `[5D92h]`
  (`+02CC..+02D5`).
- **Non-empty list:** it walks the `+2Ah`/`+2Ch` links to the tail (`+02DC..+02FA`), then appends a fresh copy the same way
  (`+02FC..+0338`).
- Before any of this it passes `[5D94]:[5D92]` and the item to `0085:004D` (`+023F..+024D`). When that returns non-zero, it
  shows a message (`0AF8:0B0B`, `00BA:1693`), sets the refusal flag, and returns.
- After an append it passes the active-character pointer to `00BA:0BB8` (`+033C..+0344`). That is the resident that reads the
  character's `+102h` field (see the `0054` section of the U7F document).

**Role line:** appends a 63-byte copy of the given item record to the active character's item list (head at `+C8h`, links at
item `+2Ah`). When `0085:004D` refuses, it shows a message and sets the caller's refusal flag instead.

**Limits:**
- the far site `20:0C60` is cited, not relied on;
- `0085:004D` is not traced here;
- the meanings of item `+3Ah` and of the refusal test are not named;
- the identity rests on the `+C8h` list dereference through `[5D92h]`.
