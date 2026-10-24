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

## `OVL_007F_0A88` (210 B, `[0A88,0B5A)`), evidence PR #1484

I re-decoded both the caller and the body from the original unit `7Fh` bytes for this section. The checks below cite
those instructions.

**Factor 1: PASS, through a near call from a `game_evidenced` caller.** The one site is `+0F25 push cs` / `+0F26 call 0A88`,
inside `OVL_007F_0CEB` (`game_evidenced`, grade A, near-call evidence `decoded_validated`; extent `[0CEB,0F7E)`). There is no
fixup-backed site and no entry slot.

**Factor 2: PASS on two independent grounds.** 0A88 USES documented game state rather than forwarding it.

The caller's pushes (`+0F15..+0F24`), in 0A88's frame (`RETF 8`):

| 0A88 slot | What 0CEB passes |
| --- | --- |
| `[bp+0Ch]:[bp+0Ah]` | `[5D94]:[5D92]`, the **active-character pointer** ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md)) |
| `[bp+8]` | `[bp-116h]`, the amount `0294` returned (`+0F11`). 0294 was given `DS:[67F4h+4i]` as its maximum at `+0F00` |
| `[bp+6]` | the index byte `[bp-2]` |

1. **The DS:67F4 pool.** At `+0AC5..+0ADA`, 0A88 reads the dword `DS:[67F4h+4i]/[67F6h+4i]`, compares the amount with it
   (`cmp dx,bx; jg/jl`, then `cmp ax,cx; jbe`), and clamps the amount to the pool (`+0AE6..+0AEA`). It then writes
   `pool - amount` back (`+0B00..+0B1E`, a `sub`/`sbb` pair, then stores to `67F4h+4i`/`67F6h+4i`). Two docs define these dwords:
   [U7F_RECORD_SUBTRACT_0020.md](U7F_RECORD_SUBTRACT_0020.md), where record `+88h` words are added into them, and
   [U7F_NUMBER_0294_CPU.md](U7F_NUMBER_0294_CPU.md), at the same 0CEB site.
2. **The active-character record.** At `+0B28..+0B42` it does `les di,[bp+0Ah]`, then reads, adds the amount to and writes back
   `es:[di+88h+2i]`, a read-modify-write of the active character's `+88h[i]` word. Before that, it passes the same
   record and amount to `0054` (`+0A8E..+0A9D`), the `+102h` threshold test (see 0054 above). When the test is over the
   threshold, it shows a message through `0AF8:0B0B` and `00BA:1693`, and does nothing else. At the end it passes the record and
   amount to `003A` (`+0B47..+0B51`).

**Role line:** moves up to the amount from the `DS:67F4` pool slot `i` into the active character's `+88h[i]` word. The amount
is clamped to what the pool holds, and the move is refused with a message when 0054's `+102h` threshold would be passed.

**Limits:**
- the meanings of `+88h[i]`, `+102h` and the pool are not named. The identity rests on the dereference and on the
  documented `67F4` dwords, not on their meaning;
- 0CEB's class is from the function-accounting queue export, which this note did not regenerate;
- `003A` is not traced here.

## `OVL_007F_063A` (967 B, `[063A,0A01)`, `RETF`), evidence PR #1506

I re-decoded all three callers and the body from the original unit `23h`, `1Eh`, `20h` and `7Fh` bytes for this section.

**Factor 1: PASS, through three relocated far calls.** Each of these is `lcall 008F:002F`, fixup-backed, with
`008F:002F` being unit `7Fh`'s public stub for entry slot 3 (`+063A`):

| Site | Caller | Condition |
| --- | --- | --- |
| `0023:+0663` | `OVL_0023_053E` | key `AL = 53h` |
| `001E:+0DF4` | `OVL_001E_0D09` | `cmp al,53h; jne` at `+0DF0` |
| `0020:+106A` | `OVL_0020_0ECD` | `cmp al,53h; jne` at `+1066` |

All three callers are `game_evidenced` in the regenerated classification (codex-poolrad3, pin `972d878e`).
[U7F_05E0_NATIVE_REACH_FULLC_GATE.md](U7F_05E0_NATIVE_REACH_FULLC_GATE.md) documents the same three sites.

**Factor 2: PASS. The body walks and updates the party list and the global pools itself.**
- **The party list.** It loads the party head `DS:5D96/5D98` (GAMEPLAY_PATH.md, "non-nil party head `[0x5D96]`") twice, at
  `+0640/+0643` and again at `+08A8/+08AB`, once per pass. For each node it:
  - tests `es:[di+84h]` against `80h` (`+0732`);
  - reads, adds to and writes back the indexed word `es:[di+88h]` (`+0772` read, `+0791` write; also `+07F5`, `+0835`, `+090E`,
    `+0967`), the money words that U7F_NUMBER_0294 and U7F_RECORD_SUBTRACT_0020 cite;
  - subtracts `es:[di+102h]` (`+08CD`), the field that 0054's threshold uses;
  - follows the link `es:[di+104h]/[di+106h]` (`+087B/+0880`, `+0991/+0996`).
- **The pools.** It reads the `DS:67F4h+4i / 67F6h+4i` dwords (`+068D..+06E5`), which U7F_RECORD_SUBTRACT_0020 defines, and writes
  the residuals back at `+09D6/+09DA`.
- **A global byte.** It clears `DS:6DC0` (`+09AD`).

**Role line:** a two-pass distribution over the party. It moves amounts from the `DS:67F4` pools into each eligible member's
`+88h[i]` words, with members' `+84h` status gating eligibility, subject to 0054's `+102h` threshold. It writes the residuals back to
the pools.

**Limits:**
- the meanings of `+84h`, `+88h[i]`, `+102h`, the pools and `DS:6DC0` are not named; the identity rests on the documented party
  head and the `67F4` dwords;
- the callers' `game_evidenced` class comes from a regenerated export, not from a credit;
- the exclusions in #1506 (a self-linked list in 05E0; aperture nodes) are evidence matters for that PR, not identity matters.
