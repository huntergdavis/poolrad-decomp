# Unit 0x3ce entry 4 (+0x01C8), the LOAD MONSTER callee at +0x0532

GAME.OVR (206,105 bytes, SHA-256 `872964a3...7928614`), unit 0x3ce file base 203,372 (0x31A6C). Entry 4 is file 0x31C34 (+0x01C8 .. +0x04FB), 820
bytes, 353 instructions, run-time stub `0x3E9:0x34`, `retf 8`, SHA-256 `3046c9df720e9e30e4db427198b37ced3932f8a6cbdca6e96a59f11e6568095d`
(matches the ledger row `0x3ce:0x01c8`). Original bytes are not reproduced here.

## ABI

Frame `0x404` bytes. Arguments in push order: far Pascal string `[bp+0xA]`, byte `[bp+8]` (p3), byte `[bp+6]` (slot index). LOAD MONSTER
(+0x0524..+0x0532) pushes the local string copy of the length-4 code constant `cs:0x044B`, `[bp-3]` (slot value(3), low byte) and `[0x6F83]`.
The word pushes of `[bp+8]` leave AH undefined for the first load; only the low byte is read by the callee.

## Control flow

Unit constants (Pascal strings; text kept out of the repository; hash prefixes): `cs:0x1B0` len 5 `f7128df4230db857`, `cs:0x1B6` len 5 `b1e911e6db9d779b`,
`cs:0x1BC` len 6 `c4f73354a451599d`, `cs:0x1C3` len 4 `4f6175b1516b413c`. Runtime helpers used: `0xAF8:0x0B25` StrStore, `0x0B0B` StrLoad,
`0x0B98` StrCat (cap 255), `0x0B57` Copy (decoded: index <= 0 -> 1, count < 0 -> 0, shorter result when the string is short), `0x0C10` equality, `0x182A` UpCase
(decoded: only 'a'..'z'), `0xAF8:0x1366` numeric-to-string (unresolved, assumed Str(v:0, string[1])).

| Path | Condition | Effects (in order) |
|---|---|---|
| A | `Copy(s,1,5)` equals `cs:0x1B0` or `cs:0x1B6` | if `UpCase(s[len]) = 'T'` then p3 += 0x40; s := Copy(s,1,len-1); `load(s, p3, slot.first)`, `load(s, p3+0x80, slot.second)` |
| B6 | s equals `cs:0x1BC` | the same two loads, p3 unchanged |
| B4 | s equals `cs:0x1C3` | the two loads, then `palette(cs:0xCD2, cs:0xCF2, slot.first)` and `palette(..., slot.second)` |
| C | none of the above | suffix := Str([0x5376], max 1); `load(s+suffix, p3, first)`, `palette(0xCD2, 0xCE2, first)`, `load(s+suffix, p3+0x80, second)`, `palette(..., second)` |

`load` = `0x464:0x0111(name, p3 byte, 1, 0, DS 0x5E5A + 8*slot [+4 for the second])`, `palette` = `0x464:0x1E5C`, and every path then calls `0x899:0x0104`
(decoded: `while keypressed do read key`). The DS table at `0x5E5A` has 8-byte entries indexed by the slot byte; the loader writes its far-pointer words.

## First unresolved side effect

`0x464:0x0111` (resident image `0x4751`): StrStore of the name (max 255), releases the previous destination object via `0x464:0x2595` when its far
pointer is non-nil, StrLoad of a unit string `cs:0x010C`, then the file/block loader `0x802:0x0716` (the same first unresolved boundary as the unit 0x6F entry 6
note) and `0xAF8:0x1685`. Until that loader is decoded, the model keeps `0x464:0x0111`, `0x464:0x1E5C`, `0xAF8:0x1366` and `0x899:0x0104` as explicit hooks.

## Real saved inputs (private probe, fabricated hooks)

With the unit's four constants and the LOAD MONSTER code constant at `cs:0x044B` (length 4) as the name, p3 = 0 (saved slot-A slot value(3)):
the name equals none of the four unit constants, so **path C** runs: Str on `[0x5376]` (a DS byte not in the save), then load/palette/load/palette with p3 = 0x00 and 0x80, a
5-character name when the suffix is one digit, then the flush. `[0x6F83]` and `[0x5376]` are runtime DS bytes, not slot-A save contents.

## Claims

Bounded model (`overlay_u3ce_entry4_01c8.{h,c}`) of every path with explicit hooks at all effects; synthetic tests only. No strict full-C, count, identity, DOS-native or one-Up parity claim.
