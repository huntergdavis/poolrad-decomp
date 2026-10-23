# `OVL_0077_17ED`: two-factor identity argument (par-a, 2026-10-22)

This is an argument, not a credit. The evidence PR is #1430 (codex-poolrad2): composed real callees, 550 unmasked cases and 28 native. Everything here is re-derived from GAME.OVR (sha `872964a3…`) and the CSVs on main. The standard is in [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20); the argument-borne clause is in [RES_0802_IDENTITY_ARGUMENTS.md](RES_0802_IDENTITY_ARGUMENTS.md).

**Factor 1: PASS, thin (one near call, the 11F3 basis).** The single call site, `+1423` in `OVL_0077_0EF8` (`game_evidenced`, uncredited), is `push cs; call 17ED` inside 0EF8's `overlay.csv` bound. There is no fixup-backed caller.

**Factor 2: PASS. A named object, selected by credited code, passed by the direct caller and dereferenced.**

This chain was corrected after par-e's review (i21464). The first version said 0EF8 passes the list head; it passes the selected node.

1. In 0EF8, `+0F10..+0F20` loads the character's **`+C8h/+CAh` list head** into `[bp-35h]/[bp-33h]` (`les di,[bp-3Fh]; mov ax,es:[di+0C8h]; mov dx,es:[di+0CAh]`). That is the in-memory far-pointer slot of [DOS_CHARACTER.md](../formats/DOS_CHARACTER.md), and the list of 63-byte linked records walked by credited `RES_00BA_0BB8` ([U9E_CHARACTER_PAIR_115C_CPU.md](U9E_CHARACTER_PAIR_115C_CPU.md)).
2. At `+1285..+128F`, 0EF8 passes **`&[bp-35h]` as an out parameter** (with `&[bp-37h]` and `&[bp-2Dh]`) to U3B0 entry 4, `OVL_03B0_0D9A` (`lcall 03C5:0034`).
3. 0D9A calls credited **`OVL_03B0_0000`** at `+1089`. That routine walks the `+2Ah/+2Ch` links from the head and returns the node at the selected index, or null. At `+108C..+1092` 0D9A writes the result DX:AX through that out pointer.
4. 0EF8 rejects null at `+1297..+129F`, branches on the key, and on `'H'` (`cmp al,48h` at `+1418`) passes the **selected node** `[bp-35h]:[bp-33h]` to 17ED at `+141C..+1423`. The same node goes beside `DS:5D92` to resident `00BA:1509` on another key path (`+13EC..+13F6`).
5. 17ED dereferences that node at `+39h` (the count byte it halves), `+34h` and `+2Ah/+2Ch` (the links). It allocates a new `3Fh`-byte node with GetMem, copies it with Move and links it in. The node size and links are exactly the 63-byte records.

**Method note:** the first version traced `[bp-35h]` only through direct `mov` stores. A writer reached through `lea` plus a pushed far pointer (an out parameter) must be traced too.

**Role line:** splits the user-selected record of the character's `+C8h` sidecar list (selected through credited `03B0_0000`): halves its count at `+39h` and links a new 63-byte copy carrying the other half.

**Limits:**
- The gameplay meaning of `+39h` and `+34h` is not named.
- Only the 0EF8 path is traced.
- 0EF8's own `[bp-3Fh]` is taken as the character pointer from its use pattern (`+C8h/+CAh`, shared with credited 0BB8), not traced to `DS:5D92`.
- The selection index comes from 0D9A's menu; its meaning is not named.
