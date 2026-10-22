# Unit 0x7F: two-factor identity arguments (par-a, 2026-10-22)

These are arguments, not credits. Everything is re-derived from GAME.OVR (sha `872964a3…`) and the CSVs on main `132e1ea1`. The standard is in [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20); the argument-borne clause is in [RES_0802_IDENTITY_ARGUMENTS.md](RES_0802_IDENTITY_ARGUMENTS.md).

## `OVL_007F_0000` (32 B), the evidence PR is #1418

**Body, decoded:**

```
push bp; mov bp,sp; sub sp,2
push [bp+8]; push [bp+6]       ; the caller's far record pointer, forwarded unchanged
lcall 00BA:13AB                ; strict-credited resident
add ax, 5DCh                   ; 13AB's result + 1500
mov [bp-2],ax; mov ax,[bp-2]; mov sp,bp; pop bp; retf 4
```

**Factor 1: PASS (near calls, `game_evidenced` callers, none credited, the 11F3 basis).**
- Each site decodes as `0E E8` to `+0000` and lies inside its caller's `overlay.csv` bound:
  - `+045D` in `OVL_007F_044C` [044C,0508);
  - `+08C7` in `OVL_007F_063A` [063A,0A01).
- Two more sites (`+0061`, `+0080`) are in `OVL_007F_0054`, which is classified unresolved and is not relied on here.

**Factor 2: PASS, on a derived value. This is a new edge, flagged for the VP.**
- The body reads no memory itself, and the record pointer is only forwarded; forwarding alone is not use.
- What the body does use is the value strict-credited `RES_00BA_13AB` computes from that record. 13AB's evidence ([U9E_CHARACTER_PAIR_115C_CPU.md](U9E_CHARACTER_PAIR_115C_CPU.md)) shows it computing over the 285-byte character object ([DOS_CHARACTER.md](../formats/DOS_CHARACTER.md)) through `11EA`'s `+10h/+16h` fields.
- The body adds `1500` and returns the sum.
- Every caller compares the return with the character field `+102h`:
  - `063A+08CA` does `les di,[bp-4]; sub ax,es:[di+102h]`;
  - `0054+0083` does the same through `[bp+0Ch]`.

  The record each caller passes is therefore the character object; `+102h` is read by credited 0BB8 ([NEAR_ONLY_GAME_SEGMENT_IDENTITY.md](NEAR_ONLY_GAME_SEGMENT_IDENTITY.md)).
- **Why this is not 1484/1596's refusal:** those transformed pixel values that were never game state. Here the transformed operand is the output of credited code over documented game state, and the result is compared against a documented field of the same record.

**Role line:** returns a character-derived threshold, credited 13AB's value over the character record plus 1500, which the 007F callers compare with character `+102h`.

**Limits:**
- The meaning of `+102h` and of the constant 1500 is not named.
- Only `063A` and `0054` are shown comparing the result with `+102h`. `044C+045D` moves the result to DX before use, and that use is not traced here.
- This is the first identity resting on a *derived* value rather than a read or dereference. If the VP rules that derived values do not count as use, the verdict becomes FAIL on the forwarding rule. Nothing else here would change.
