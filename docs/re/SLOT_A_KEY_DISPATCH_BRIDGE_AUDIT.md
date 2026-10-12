# Slot-A saved-turn key-dispatch bridge: boundary audit

This audit follows the saved world-state transfer toward the first gameplay
key call, unit `0x1C:+0x3395` to unit `0x68` entry 2. It identifies the first
missing C state bridge after `pr_slot_a_native_restore`. It does **not** run
one Up in C, claim DOS/native parity, or add a full-C routine or identity.
The existing `PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH` is a staged adapter
result, not evidence that every instruction before the key call has run.

## Prior work and pinned inputs

Before this audit, `deja "Pool of Radiance saved turn key dispatch unit0x1C
32F8 unit0x68 entry2 bridge save restore"` found no direct session. A shorter
`deja "slot A one Up key dispatch"` returned broader slot-A save and route
sessions (Claude `3e3ff494-f86` and `a8e5fad4-c70`); this audit reuses their
documented transfer and gameplay leads through [the save-frame map](../formats/DOS_SAVGAM.md),
[the party-input audit](SLOT_A_PARTY_INPUT.md), and [the gameplay path](GAMEPLAY_PATH.md).
The C boundary and callee contracts come from [the save adapter](SLOT_A_NATIVE_SAVE_ADAPTER.md),
[the gameplay slice](U1C_LOOP_3387.md), [the entry-2 Up slice](U68_ENTRY2_UP_SLICE.md),
and [the native prompt return](SLOT_A_ONE_UP_NATIVE_PROMPT_ABI.md). Addresses
and spans below were checked again against the ignored originals.
The later, independently reviewed [same-invocation native key call and return](SLOT_A_NATIVE_KEY_CALL_RETURN.md)
provides live call, return and first gate values; it supplies no C state
transitions.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| slot-A world save | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |

Capstone 5 in 16-bit mode decoded the complete unit `0x1C:+0x32F8..+0x3507`
as 528 bytes and 164 instructions (SHA-256
`a6f44cd9d3397ea0d96b03f45dfb82fa10c44071a9023a57d8e4979e2260ea79`).
Its **unmodelled initialization prefix** `+0x32F8..+0x3386` is 143 bytes and
40 instructions (file `0x403E..0x40CC`, SHA-256
`963ed7d35580d2e4ab0af891d6edcc273fe9d3da1758e1f22d0d3585fdf6657b`).
The eight bytes at `+0x3395..+0x339C` hash to
`7a341933af7259a4938e01d1f664c65d986784038dc5223a2c5787f5d54069d0`:
`lcall 0x72:0x2A`, then store its `AL` at local `[BP-1]`. Unit `0x68`
entry 2's code `+0x0989..+0x0D4B` is 963 bytes and 374 instructions,
SHA-256 `6b79d88f95710c9ceb1f9d6d05a669d9125efd1c0ca34ec9f0911c35dc9e00f8`.
Its five-byte prompt call at `+0x09D0` hashes to
`369af068023cb73db52eb83f2d63d9a97804ff2e707e69f66c165bcd1575c5d3`.

## Ordered boundary and effects

| Reached code | Inputs and effect | Current C coverage |
| --- | --- | --- |
| unit `0x6F:+0x17D5..+0x18DA` | Ten transfers populate windows 0/1/2, script, party bytes, selector, mode, count and name slots. The last transfer call is at `+0x18DA`. | `pr_slot_a_native_restore` copies and checks all ten spans. |
| unit `0x6F:+0x18DF..+0x1925` | Close the save handle at `+0x18E5`; split window-0 word `+0x1FE` into DS bytes `[0x49F6]` (shift right 1) and `[0x49F5]` (low bit); copy window-0 byte `+0x1F8` to `[0x49E2]`; clear window-1 word `+0x67C`; copy the local character count to the loop bound. | The adapter retains the source bytes but has no destination DS fields or post-transfer execution. The decoded `+0x18DA..+0x1936` span is 93 bytes, 27 instructions, SHA-256 `b1855ef72da2dc6c7e379594719ef7cd5d0160a0d2aba5038e1470506ab6f054`. |
| unit `0x6F:+0x1926..+0x19F4`, then later loader work | Load character files in slot order, clear stale pointers, append party nodes and call further post-load routines. Later `+0x1BC1..+0x1BCA` replaces `[0x5376]` with window-1 byte `+0x624`. The menu sets restored flag `[0x49F8] = 1` on its successful load return. | `pr_u6f_party_build` models a bounded record chain, but it does not execute sidecar loads or `+0x02AE` after each insertion; the save adapter invokes none of these. The saved file does not contain the menu's restored flag. |
| unit `0x1C:+0x32F8..+0x3386` | Initialize seven DS bytes; derive map ID `[0x84DC]` from window-0 byte `+0x1E4`; possibly change mode to 3 for map IDs `0x19..0x1B` when word `+0x1CC` is zero; skip ECL block load only if live `[0x49F8] != 0`; initialize script state; run initial entry `[0x49EB]`; conditionally call resident `0xBA:0x27D9`; refresh view. | `pr_u1c_loop_3387` starts **after** this prefix. The raw save adapter has neither the prefix model nor the runtime state produced by these calls. |
| unit `0x1C:+0x3387..+0x339C` | Write map ID to window-0 word `+0x1E4`, call unit `0x68` entry 2, store its returned key `AL`. | The bounded gameplay model has an `entry2_key` hook. The existing Up slice accepts a separate prompt event and required host hooks. No host adapter connects these to a completed restored state. |

The first omitted *in-memory state bridge* after the ten copied spans is
`0x6F:+0x18EA..+0x1919`, which derives three DS bytes and clears a window-1
word. The runtime close at `+0x18E5` precedes it. Even if those simple writes
were added, the party loop, sidecars, later loader calls, menu return, and
gameplay initialization still precede the key call. A raw saved `(x,y,facing)`
or window value is therefore not a substitute for executing those transitions.

## Key call and return once its prerequisites exist

On the bounded selector-1 extended-Up path, unit `0x68` entry 2 first clears
window-1 word `+0x592`, prepares the prompt, and calls the input routine
`0x3C5:0x002A` at `+0x09D0`. Its two outputs are key `AL` and a byte at
`SS:[BP-4]` indicating an extended key. The native exactly-one-Up watch
measured `AL=0x48` and that byte equal to 1 at the return. This **observed
input event** selects preflight `+0x068D`, then resident `0xBA:0x290B`;
the exit path copies the current selector to `[0x49F9]`, conditionally calls
`0x3F1:0x0137` and sets `[0x84E4]`, and returns the saved `0x48` in `AL`.
Those prompt, resident and exit effects remain required callbacks in
`pr_u68_entry2_up_slice`; the measured event alone cannot supply them.

Back in unit `0x1C`, the returned `0x48` is saved at `[BP-1]`, the active
character pointer `[0x5D92]:[0x5D94]` is copied to `[0x4436]:[0x4438]`,
and window-0 word `+0x1E4` is refreshed. At `+0x33B9`, the live window-1
word `+0x594`, exit flag `[0x49FF]`, and subsequent script effects decide
the branch. The [native key-call trace](SLOT_A_NATIVE_KEY_CALL_RETURN.md)
observed the same invocation return to `+0x339A` with `AL=0x48` and window-1
word `+0x594=0` at `+0x33BD`; at its later pre-RANDOM stop the party and
clock were `(14,4,6)` and `(1,5,10)`. These are DOS observations, not outputs
of this C adapter. The merged [gameplay slice](U1C_LOOP_3387.md) records the later
movement bookkeeping; this audit does not assign a party tuple, window word,
script result, or coordinate-writer effect from the key alone.

## Concrete C integration blocker

`pr_slot_a_native_state` represents the ten file transfers and two observed
lookups. It has no complete post-load DS state, character/sidecar objects,
initial script-run effects, or host implementations for the entry-2 prompt
and resident calls. A faithful key-dispatch bridge must first make those
effects explicit and ordered, then pass the resulting live windows, DS fields,
party/map inputs and callbacks to the bounded entry-2 and gameplay models.
Returning a moved party or a successful key from the current adapter would
invent the missing state. The earliest useful implementation target is the
loader's post-transfer state and party completion; the next target is the
40-instruction gameplay initialization prefix. The current one-Up blocker
should remain until those prerequisites and reached callbacks are connected.

## Reproduction

The local audit read only ignored files. Hash-check `GAME.OVR`, `START.EXE`
and the slot-A save against the table, then decode `GAME.OVR` with a 16-bit
x86 decoder at file spans `0x403E..0x40CC` (unit `0x1C` prefix),
`0x40DB..0x40E2` (key call and local store), and `0x17054..0x17416`
(unit `0x68` entry-2 code). Unit `0x6F` starts at file `0x1DFE6`; its
`+0x18DA..+0x1936` span maps to file `0x1F8C0..0x1F91C`. Compare lengths,
instruction counts and hashes above. No original bytes or private capture
are committed.
