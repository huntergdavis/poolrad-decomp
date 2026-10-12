# Unit 0x1c +0x0450: ECL LOAD MONSTER (0x0B), static dependency map

Read-only static analysis; **no C implementation, no strict credit, no route advancement** (PM assignment after PR #210). Opcode `0x0B` (LOAD MONSTER,
reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), 3 operands) dispatches to unit `0x1c` `+0x0450..+0x0816`, file `0x1196..0x155C` (unit base `0x0D46`): **967 bytes,
306 instructions**, one `retf` at `+0x0816`, a 0x156-byte frame. SHA-256 `bf1903765b12e98cfc5749630f31e2bedcabc607346255cdfae6b641759716cd`
(GAME.OVR, 206,105 bytes, `872964a3…7928614`; matches the PM). The next routine is `+0x0817` (APPROACH). `deja` timed out (PM's and mine) with no hit. Reused:
[U1C_SETUP_MONSTER_03B4.md](U1C_SETUP_MONSTER_03B4.md), [U1C_CLEARMONSTERS_125D.md](U1C_CLEARMONSTERS_125D.md), [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md),
[DOS_CHARACTER.md](../formats/DOS_CHARACTER.md) (the 285-byte record and its `+0x104` party link).

## Entry facts at the supplied-state stop

In the private supplied-state route (not DOS evidence) the run reaches PC `0xB1B0` right after CLEARMONSTERS, which has just set `[0x4435] = 0`, `[0x84E7] = 0`, `[0x6F83] = 8` and zeroed
`DS:0x67F4..0x680F`. The saved operands are `01 80 6E | 01 08 98 | 01 80 6E`: slot 1 = class-1 word `0x6E80`, slot 2 = window-2 word `0x9808`, slot 3 = class-1 word `0x6E80`
(plain load-hook addresses, see [ECL_LOAD_HOOK_CLASSIFIER.md](ECL_LOAD_HOOK_CLASSIFIER.md)).

## Main control flow (from the bytes; the inner copy loops are summarized, not enumerated predicate by predicate)

| Range | Behavior |
| --- | --- |
| `+0x0457..+0x0464` | `saved := far [0x5D92]:[0x5D94]` (the active-character pointer, restored at `+0x0806`); `[bp-4] := 1` |
| `+0x0468..+0x046B` | loader(3) (`0x2B:0x2A`) |
| `+0x0470..+0x0477` | **`[0x4435] >= 0x3F` -> return at once** (a 63-record cap; `[0x4435]` is the loaded-record counter CLEARMONSTERS zeroes); at the stop it is 0, so the body runs |
| `+0x048A..+0x0492` | `id := AL` of slot value(1) |
| `+0x0495..+0x04AA` | `0x2B:0x39` (unit `0x25` entry 5, `+0x0411`) with `id` and three far var pointers (`[bp-0x10]` record pointer, `[bp-0x142]` and `[bp-0x14e]` list heads) |
| `+0x04AF..+0x04BE` | `0xAF8:0x0A3F` = `Move(src = far [bp-0x10], dst = local buffer [bp-0x131], 0x11D)`: a **285-byte (`0x11D`) record copy** |
| `+0x04C3..+0x04DF` | save the two list heads (`[bp-0x142]` -> `[bp-0x14A]`, `[bp-0x14E]` -> `[bp-0x156]`) |
| `+0x04E3..+0x04F8` | `count := AL` of slot value(2); **`count = 0` -> `count := 1`** (`ja`) |
| `+0x04F8..+0x0500` | `[bp-3] := AL` of slot value(3) (an unsigned byte parameter) |
| `+0x0503..+0x051F` | `cur := far [0x5D96]:[0x5D98]` (the **party list head**); copy the code-segment constant `cs:0x044B` into a local string (`0xAF8:0x0B25`, maximum 12) |
| `+0x0524..+0x0532` | `0x3E9:0x34` (unit `0x3ce` entry 4, `+0x01C8`) with the local string, `[bp-3]` and `[0x6F83]` |
| `+0x0537..+0x0559` | walk the party list to its **tail** through the link at `+0x104/+0x106` (`or` of both words) |
| `+0x055B..+0x059B` | the first monster: `tail.link := [bp-0x10]` (the record `0x2B:0x39` returned **joins the party list as the new tail**); `cur := new tail`; `cur[+0xBF] := [0x6F83]`; clear its `+0x104/+0x106`; `[0x4435]++`; `[bp-4]++` |
| `+0x05A2..+0x05B4` | **`[bp-4] > count` or `[0x4435] >= 0x3F` -> `+0x07FD`** (finish) |
| `+0x05B7..+0x0637` | each further copy: `GetMem(0x11D)` (`0xAF8:0x023F`) linked at the tail, `Move` of the 285-byte local copy into it, `+0xBF := [0x6F83]`, clear `+0x104/+0x106`, `+0x7F..+0x82`, `+0xC8..+0xCB`; `[bp-4]++`, `[0x4435]++` |
| `+0x063E..+0x070D` | if list head 1 (`[bp-0x142]`) is non-nil: for each node (link at `+0x2A/+0x2C`) append a `GetMem(0x3F)` + `Move(0x3F)` copy to the new record's chain at `+0xC8/+0xCA` (63-byte units, the size of the `.ITM` sidecar unit) |
| `+0x0710..+0x07E7` | if list head 2 (`[bp-0x14E]`) is non-nil: the same with `GetMem(9)` + `Move(9)` nodes (link at `+5/+7`) on the chain at `+0x7F/+0x81` (9-byte units, the size of the `.SPC` unit) |
| `+0x07EA..+0x07FA` | restore list head 2 and loop to `+0x05A2` |
| `+0x07FD..+0x0816` | `[0x6F83]++`; `[0x84E7] := 1`; `far [0x5D92]:[0x5D94] := saved`; return |

## Call map (targets and what is known)

| Site(s) | Target | Identity | Known |
| --- | --- | --- | --- |
| `+0x046B` | `0x2B:0x2A` | unit `0x25` loader | merged lane-A model |
| `+0x048D`, `+0x04E6`, `+0x04FB` | `0x2B:0x25` | unit `0x25` slot value | merged lane-A model (class-1 reads may stop at HOOK) |
| `+0x04AA` | `0x2B:0x39` | unit `0x25` entry 5 `+0x0411` (113 bytes) | decoded: zeroes its far var, `GetMem(0x11D)`, **`lcall 0x7C:0x3E`** with the new far object and the byte id, then returns the object's `+0xC8` and `+0x7F` pointers through two far vars |
| inside `+0x0411` | `0x7C:0x3E` | unit `0x6f` entry 6 `+0x0EC4` (file `0x1EEAA`; stub arithmetic calibrated on `0x7C:0x57` = entry 11) | **not decoded here**: the earliest unresolved boundary (it fills the 285-byte object from its id) |
| `+0x04BE`, `+0x05FB`, `+0x0684`, `+0x06DC`, `+0x0763`, `+0x07B7` | `0xAF8:0x0A3F` | Pascal `Move` (image `0xB9BF`, `les di`/`lds si`/`rep movsb`, `retf 0xA`) | decoded |
| `+0x05BB`, `+0x065E`, `+0x06B6`, `+0x073F`, `+0x0793` | `0xAF8:0x023F` | Pascal `GetMem(size)` (image `0xB1BF`, 21 bytes; near call to the heap allocator `0xAF8:0x0308`, carry = runtime error 203 `0xCB`) | wrapper decoded; allocator not |
| `+0x051F` | `0xAF8:0x0B25` | Pascal `StrStore` (translated in the HORIZONTAL MENU model) | known |
| `+0x0532` | `0x3E9:0x34` | unit `0x3ce` entry 4 `+0x01C8` (file `0x31C34`), 3 arguments (string, byte, byte) | [Mapped with a bounded C model](U3CE_ENTRY4_01C8.md); its downstream effects remain explicit hooks. The string constant's content stays out of the repository. |

## DS and heap writes

DS: `[0x4435]` (`inc`), `[0x6F83]` (`inc`), `[0x84E7] := 1`, `far [0x5D92]:[0x5D94]` (restored), and through `[0x5D96]:[0x5D98]`'s tail the party list (`+0x104/+0x106` links).
Heap objects: the `0x2B:0x39` object (linked into the party list), per-copy `0x11D`-byte records, `0x3F`-byte and 9-byte sidecar chains. `[0x6F83]` is also the party slot byte
(`+0xBF`) given to each new record, as in the party insertion of unit `0x6f` (`+0x1446`, [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md)) which assigns slots `0..7` for the saved party; here CLEARMONSTERS set `[0x6F83] = 8`.

## Slot-value boundary, earliest unresolved native dependency, and unknowns

0. **Host slot-value boundary (separate from the native dependency below).** The three slot values are read at `+0x048D` (id), `+0x04E6` (count) and `+0x04FB` (byte parameter) through the
   lane-A slot-value model, so the first stop of a route can already be a class-1 load-hook `HOOK` there, **before `0x2B:0x39` is reached**, if an operand names a load-hook-effect address (a
   stop that the merged handlers report as CORE/HOOK at the read). In the supplied-state route the class-1 operands are the plain address `0x6E80` (no hook effect), so the reads pass and the
   route continues to the next item.
1. **After the slot values are supplied, `0x2B:0x39` -> `0x7C:0x3E` (unit `0x6f` `+0x0EC4`) is the earliest unresolved native monster-record dependency.** It is the first call after the slot reads and decides what record (monster id -> 285-byte object, its `+0xC8` and `+0x7F` sub-lists) is loaded; it is
   not decoded, so the content and any file I/O are unknown. Everything after depends on it (the list heads that gate `+0x063E` and `+0x0710`).
2. `0x3E9:0x34` (unit `0x3ce` `+0x01C8`) now has a bounded model with resident load, palette, and key-flush hooks; the Pascal heap allocator (`0xAF8:0x0308` behind `GetMem`/`FreeMem`) remains an untranslated boundary.
3. Whether the new records belong on the party list in the live game's sense (they are linked through `+0x104/+0x106` at the party list's tail, so later party walks such as PARTYSTRENGTH and
   PARTY SURPRISE would see them) is the static reading of `+0x055B..+0x059B`; no DOS observation is claimed.
4. The slot byte `[0x6F83]` is shared with the saved party's slot allocation (8 here), and the `[0x4435]` cap of 63 bounds the count of loaded records.
5. The string constant at `cs:0x044B` (not published) and the class-1 values of the two `0x6E80` operands are supplied-state facts, not DOS facts.

Nothing here claims DOS reachability, native search reachability, one-Up parity or any count.
