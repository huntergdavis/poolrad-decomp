# Unit 0x25: two-factor identity arguments (par-a, 2026-10-23)

These arguments follow the two-factor standard used in [U7F_IDENTITY_ARGUMENTS.md](U7F_IDENTITY_ARGUMENTS.md). Each claim
below was checked against a fresh decode of the original unit `25h` bytes from GAME.OVR.

## `OVL_0025_07BC` (696 B, `[07BC,0A74)`), evidence PR #1492

The body is already translated inline as `load_hook()` in the strict model `overlay_u25_load_word_0e0c.c`. #1492 exposes
it as `pr_u25_load_hook_07bc` with its own checker. This section covers identity only; whether a body pinned inside a strict
closure is creditable as its own row is par-e's call.

**Factor 1: PASS, through a near call from a STRICT caller.** The only site is `+0E49 push cs` / `+0E4A call 07BC`, inside
`OVL_0025_0E0C`. 0E0C is strict-credited, so this caller is stronger than `game_evidenced`. There is no fixup-backed site
and no entry slot.

The caller's pushes (`+0E41..+0E48`) map onto 07BC's frame (`RETF 6`) as follows:

| 07BC slot | What 0E0C passes |
| --- | --- |
| `[bp+0Ah]` | `[bp+6]`, 0E0C's address word. 07BC rebases it as `key := address - 6B00h` (`+07C9..+07CF`) |
| `[bp+6]` | `ss:&[bp-4]`, the found flag. 07BC sets it to 1 at `+07C5`, and clears it to 0 for an unmapped key at `+0A64..+0A67` |

0E0C keeps the returned AX (`+0E4D`). It reads the window-1 word `es:[[49D6]+2*address+2A00h]` only when the flag is 0
(`+0E50..+0E66`). [ECL_LOAD_HOOK_6B9B_RECORD.md](ECL_LOAD_HOOK_6B9B_RECORD.md) documents the range `+0E41..+0E69`.

**Factor 2: PASS. 07BC dereferences documented game state itself; it does not just forward it.**
1. **The active-character record.** Each mapped key arm does `les di,[5D92h]` and reads a field of the record: the
   active-character pointer ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md)), with 19 such loads between `+07DA` and `+09AD`.
   Two examples:
   - key `18h` returns `es:[di+14h]` (`+07DA..+07E4`);
   - key `9Bh` returns `es:[di+6Eh]` (`+07EF..+07F9`), the field
     [ECL_LOAD_HOOK_6B9B_RECORD.md](ECL_LOAD_HOOK_6B9B_RECORD.md) documents.

   par-d2's mapping also covers the signed `+73h` ([ECL_LOAD_HOOK_6BA0_SIGNED.md](ECL_LOAD_HOOK_6BA0_SIGNED.md)), the
   money words `+88h..+90h` ([U7F_RECORD_SUBTRACT_0020.md](U7F_RECORD_SUBTRACT_0020.md)) and the class byte `+15h`,
   which is mapped through a table at `+09AD..+0A3B`.
2. **The ECL file number.** Key `312h` returns the byte `DS:5376` (`+0A47..+0A4C`). GAMEPLAY_PATH.md section 4 names it
   "the ECL file number used by the block load".
3. **The window-1 record.** Key `33Eh` returns `es:[[49D6]+67Ch]` (`+0A56..+0A5F`). This is the same window-1 record
   pointer `DS:49D6` that the ECL load and store hooks use
   ([ECL_LOAD_HOOK_6DCA.md](ECL_LOAD_HOOK_6DCA.md), [ECL_STORE_HOOK_6E79.md](ECL_STORE_HOOK_6E79.md)).

Its two calls to `0756` push `[5D94]:[5D92]` (`+0978`, `+0991`). That is forwarding, and the argument does not count it.

**Role line:** the ECL load-word hook. It maps an ECL address in the `6B00h` window onto a field of the active character,
the ECL file number, or a window-1 record word, and sets the caller's found flag. An unmapped key clears the flag, so 0E0C
falls back to the window-1 word.

**Limits:**
- not every key's field is named here; the identity rests on the `5D92` dereferences and on `5376`/`49D6`, not on each
  field's meaning;
- `0756` is not traced in this note.
