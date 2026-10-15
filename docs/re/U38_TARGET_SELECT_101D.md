# Unit 0x38 near helper: character target selection at +0x101D

## Gate and provenance

PM queued U38 public entry 100 `+0x2AE6` behind this near-called helper.
The bounded `deja "poolrad U38 +2AE6 entry100 +101D near helper active
character strict C parity"` query timed out without a prior-session
result. This work reused and checked the merged
[resident target selector](RESIDENT_CHARACTER_TARGET_SELECT.md) and its
[character-record context](../formats/DOS_CHARACTER.md). The old overlay
classification labels `+0x101D` as game-or-library unresolved; this
candidate supplies code and ownership evidence for PM adjudication.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The **complete original helper** is `GAME.OVR` file `[0xFC78,0xFCE1)`,
unit `0x38:[+0x101D,+0x1086)`: 105 contiguous bytes / 36 instructions,
SHA-256
`632b609ad14b0fff262cd578a81e144d5dbeab6418ee8ef68303ff9e60cea33a`.
Public entry 35 starts at `+0x1086`. The sole call is the fixup-confirmed
`LCALL BA:2E8B` at `+0x1045` (segment fixup `+0x1048`).

Eight decoded near-call sites target this helper from seven U38 public
entries: entry 35 at `+1092,+10A2`; entry 78 at `+25CB`; entry 80 at
`+265A`; entry 99 at `+2A96`; entry 100 at `+2AF5`; entry 103 at
`+2B8C`; and entry 107 at `+2D97`. These callers pass the active
character pointer from `DS:5D92/5D94`. Entry 100 uses the result to
test target byte `+32` before halving the game state byte `DS:6818`;
entry 99 tests the target's indexed record and likewise changes `DS:6818`.
The helper itself reads the character's `+CC/+CE` far link and invokes
the game-verified resident character selector. This specific record,
selector, and game-effect caller chain supports game ownership; PM has not
yet reclassified the ledger or credited this helper.

## Exact behavior and C boundary

The helper takes a far character pointer at `[BP+6,+8]`, returns a far
target pointer in `AX:DX`, and uses `RETF 4`. It zeroes a four-byte local
output cell. If the character's `+CC/+CE` link is null, it returns zero
without a resident call. Otherwise it passes the character and local
cell to `BA:2E8B`. If that selector returns `AL=0`, or returns `AL!=0`
with a still-null output cell, the helper rereads and returns the original
character link. For `AL!=0` and a nonnull output cell, it returns the
selected pointer. The fallback reread is observable when the callback
changes the character record.

`pr_u38_target_select_101d` models the complete 105-byte wrapper in a
1 MiB RAM and 14-field CPU state. Its callback enters at the actual
BA:2E8B far-call frame: return IP `104A`, return CS, local output far
pointer, and character far pointer. The model writes the call return
words, preserves callback CPU/RAM effects, and emulates `RETF 8`. The
focused C adapter composes the merged `pr_rec_res_00ba_2e8b` through a
RAM-backed IO interface. The callback remains responsible for resident
register and flag effects that the merged high-level API does not expose.

## Verification and limits

The original-byte Unicorn checker executes all 105 bytes and the real
far call before intercepting at BA:2E8B. Across 24 cases it compares
the entire 1 MiB RAM, all 14 CPU fields, and the six-word callee-entry
frame. Cases cover null link, selected link, AL-zero and empty-output
fallback, both alternate target slots, DS/SS alias, and 16-bit stack
wrap. Focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTests
pass for this helper and the merged resident selector, including an
aborted callback.

Four DOSBox-X normal-core synthetic frames execute an actual far entry
into the original helper and its far call to a deterministic resident
stub. All 14 returned CPU words and 34 stack bytes match C. Generated
COM, copied body, logs, and capture stay under ignored
`resources/u38-101d-parity/native/`. Capture SHA-256:
`4cdbe4d44f17b3552002fb61954a4edfdfd2a2fe42e2c3663982c46fc9a13f43`.

The native frames are synthetic and relocate the resident target. The
Unicorn callback supplies selected resident effects; the independent
merged resident C model is exercised by the focused test. There is no
reached game frame for this helper or entry 100. This is a provisional
full-C candidate for PM review, with no identity or count claim.
