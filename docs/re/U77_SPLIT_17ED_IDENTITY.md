# `OVL_0077_17ED`: two-factor identity argument (par-a, 2026-10-22)

This is an argument, not a credit. The evidence PR is #1430 (codex-poolrad2): composed real callees, 550 unmasked cases and 28 native. Everything here is re-derived from GAME.OVR (sha `872964a3…`) and the CSVs on main. The standard is in [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20); the argument-borne clause is in [RES_0802_IDENTITY_ARGUMENTS.md](RES_0802_IDENTITY_ARGUMENTS.md).

**Factor 1: PASS, thin (one near call, the 11F3 basis).** The single call site, `+1423` in `OVL_0077_0EF8` (`game_evidenced`, uncredited), is `push cs; call 17ED` inside 0EF8's `overlay.csv` bound. There is no fixup-backed caller.

**Factor 2: PASS. A named object, passed by the direct caller and dereferenced.**
- In 0EF8, `+0F10..+0F20` does `les di,[bp-3Fh]; mov ax,es:[di+0C8h]; mov dx,es:[di+0CAh]` into `[bp-35h]/[bp-33h]`. That is the **`+C8h/+CAh` list head of the character object**: an in-memory far-pointer slot in [DOS_CHARACTER.md](../formats/DOS_CHARACTER.md), and the list credited `RES_00BA_0BB8` walks ([U9E_CHARACTER_PAIR_115C_CPU.md](U9E_CHARACTER_PAIR_115C_CPU.md), "its 63-byte linked records").
- `+141C..+1423` passes that pointer to 17ED as its only argument `[bp+6]`.
- The same local is passed beside the active-character pointer `DS:5D92` to resident `00BA:1509` at `+13EC..+13F6`.
- 17ED dereferences the node at `+39h` (the count byte it halves), `+34h` and `+2Ah/+2Ch` (the links). It allocates a new `3Fh`-byte node with GetMem, copies with Move, and links it in. The node size and links are exactly the 63-byte records.

**Role line:** splits one record of the character's `+C8h` sidecar list: halves its count at `+39h` and links a new 63-byte copy carrying the other half.

**Limits:**
- The gameplay meaning of `+39h` and `+34h` is not named.
- Only the 0EF8 path is traced.
- 0EF8's own `[bp-3Fh]` is taken as the character pointer from its use pattern (`+C8h/+CAh`, shared with credited 0BB8), not traced to `DS:5D92`.
