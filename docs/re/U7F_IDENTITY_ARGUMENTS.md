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

**The derivation, link by link** (VP ruling i21629: a derived value is identity evidence only when each link is stated and evidenced):

| # | Link | Evidenced by |
| ---: | --- | --- |
| 1 | `0077_1CEE` far-calls `0054` at `+1E3C` through stub `008F:0025` | the fixup-backed site, decoded from GAME.OVR; inside 1CEE's ledger extent; 1CEE is `game_evidenced` |
| 2 | that caller pushes `[5D94]:[5D92]`, the active-character pointer, as 0054's record argument | the decoded pushes at `+1E28..+1E2C`; the pointer is documented in [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) |
| 3 | 0054 passes the same record unchanged to `0000` (`+005A..+0061`, `push [bp+0Eh]; push [bp+0Ch]; call 0000`) | the decoded body of 0054 (above) |
| 4 | 0000 passes the record to `RES_00BA_13AB` and adds `5DCh` to its result | the decoded body of 0000 (above); 13AB is strict-credited ([U9E_CHARACTER_PAIR_115C_CPU.md](U9E_CHARACTER_PAIR_115C_CPU.md)) |
| 5 | 13AB computes its result from the character object through `11EA`'s `+10h/+16h` fields | 13AB's and 11EA's credited evidence (115C doc; [NEAR_ONLY_GAME_SEGMENT_IDENTITY.md](NEAR_ONLY_GAME_SEGMENT_IDENTITY.md)) |
| 6 | the caller compares 0000's return with the character's `+102h` word (`0054+0069..+0071`) | the decoded body of 0054; `+102h` is read by credited 0BB8 |

**Limit:** a derived value is only as good as its weakest link. Here that is link 5: 13AB's result is credited as computed, but its gameplay meaning, like that of the constant 1500 and of `+102h`, is not named.

**Role line:** returns a character-derived threshold, credited 13AB's value over the character record plus 1500, which the 007F callers compare with character `+102h`.

**Limits:**
- The meaning of `+102h` and of the constant 1500 is not named.
- Only `063A` and `0054` are shown comparing the result with `+102h`. `044C+045D` moves the result to DX before use, and that use is not traced here.
- This is the first identity resting on a *derived* value rather than a read or dereference. If the VP rules that derived values do not count as use, the verdict becomes FAIL on the forwarding rule. Nothing else here would change.

## `OVL_007F_0054` (84 B), the evidence PR is #1421 (merged)

**Factor 1: PASS, through a relocated far call.** The `game_evidenced` function `OVL_0077_1CEE` (bound 612 B) far-calls `0054` through its stub at `+1E3C`: `lcall 008F:0025`, a fixup-backed site inside the caller's extent. 0054 also has near sites in the `game_evidenced` functions `0203`, `063A` (x2), `0A88` and `19B2` (x2).

**Factor 2: PASS, by dereferencing a documented game record.** At `+1E28..+1E3C` the caller pushes `[5D94]:[5D92]`, the **active-character pointer** ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md); the standard's own 0x7F example). That pointer becomes 0054's record argument `[bp+0Ch]`. 0054 then:
- dereferences it, `les di,[bp+0Ch]; mov ax,es:[di+102h]`, adds the amount argument `[bp+0Ah]`, and compares the sum with 0000's threshold;
- when the sum is over, sets its flag and stores `0000(record) - record.+102h` through the out pointer `[bp+6]`; otherwise it stores 0.

`+102h` is the character field that strict-credited 0BB8 also reads.

**Role line:** decides whether adding an amount to the active character's `+102h` word passes the character-derived threshold. It returns the flag and stores the difference.

**Consequence for 0000:** the record 0054 forwards to `0000` is the active character, so 0000's derived-value argument above has a documented subject on this path, independent of the 063A trace.

## `OVL_007F_0203` (145 B), evidence PR #1440

**Factor 1: PASS, through near calls from a `game_evidenced` caller.** It has two sites, `+1ED4` and `+2227`, both `0E E8` to `+0203`, inside `OVL_007F_19B2`, which is `game_evidenced` and uncredited (the 11F3 basis).

**Factor 2: PASS. The body DEREFERENCES the active-character record itself.** At `+0238..+023C` and `+0276..+027A` it does `les di,[5D92h]; add es:[di+90h],ax`: a read-modify-write of the active character's `+90h` word through the pointer `DS:5D92/5D94` ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md)). It passes the same pointer to `0054` (`+0209..+021A`; see 0054 above) to test the cap. When the cap is hit, the excess goes to the 32-bit global `DS:6804/6806` and a message is shown through `1693`.

**Role line:** adds an amount to the active character's `+90h` word, capped by 0054's threshold test. The excess goes to the global accumulator `DS:6804`, with a message.

**Limits:**
- the meaning of `+90h` and of `DS:6804/6806` is not named, and no document defines `DS:6804`;
- the identity rests on the active-character dereference, not on 6804.

## `OVL_007F_044C` (188 B), evidence PR #1453

**Factor 1: PASS, through a relocated far call.** The site is `0077_2156+2410` (`lcall 008F:0043`, fixup-backed), inside `game_evidenced` `OVL_0077_2156`'s extent.

**Factor 2: PASS. It dereferences the active-character record, passed as an argument.**

The caller's pushes (`+23FE..+240F`), in 044C's frame:

| 044C slot | What 2156 passes |
| --- | --- |
| `[bp+0Eh]` | `ds:5D92`, a far pointer to the active-character pointer cell |
| `[bp+0Ah]` | `ss:&[bp-4]`, 2156's local |
| `[bp+8]` | an amount word |
| `[bp+6]` | an index byte |

- 2156's `[bp-4]` is loaded from the global `DS:471A/471C` (`+215D..+2167`) and **then passed as an out parameter** to resident `00BA:2C48` (`+2182..+2187`). It is null-checked and written back to `471A`. So it holds a record chosen by that resident.
- 044C dereferences `[bp+0Eh]` twice (`les di,[bp+0Eh]; les di,es:[di]`) to reach the **active character**, and subtracts the amount from its word `+88h + 2*index` (`+047A..+049A`).
- It adds the amount to the same word of the record at `[bp+0Ah]` (`+04B6..+04D6`).
- That happens only after the cap test, which compares the destination's `+102h` plus the amount with `0000`'s threshold (`+0452..+0472`).

**Role line:** moves an amount of the active character's `+88h[index]` word to the record selected through `2C48`, capped by the destination's 0000 threshold, with a message through `1693` when capped.

**Limits:**
- the destination record is taken as a character object from its use of `+88h` and `+102h` (the 285-byte layout), not traced through `2C48`;
- the meanings of `+88h[index]` and `+102h` are not named.
