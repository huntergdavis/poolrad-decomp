# Slot-A key-dispatch return: bounded C state step

`pr_slot_a_key_return_step` applies the nine memory-effect instructions in
DOS v1.3 `GAME.OVR` unit `0x1C:+0x339A..+0x33B8`, immediately after unit
`0x68` entry 2 returns. The host supplies the live returned `AL`, map ID,
active-character far pointer, and window-0 frame. The step stops before
`+0x33B9` loads the window-1 pointer. It does not execute the input callee,
the subsequent key gate, or one Up from a saved file.

Before implementation, `deja "Poolrad slot A saved one Up key dispatch
0x1C 3395 unit 0x68 entry 2 0989 BLOCKED_KEY_DISPATCH C native replay"`
found no direct bridge implementation. The merged
[key-dispatch boundary audit](SLOT_A_KEY_DISPATCH_BRIDGE_AUDIT.md),
[gameplay-loop model](U1C_LOOP_3387.md),
[entry-2 Up slice](U68_ENTRY2_UP_SLICE.md), and
[same-invocation native call and return](SLOT_A_NATIVE_KEY_CALL_RETURN.md)
supplied the source and live-state leads. The span and numeric RAM delta
below were checked independently against ignored, hash-pinned inputs.

| Private input or source span | Size / instruction count | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| slot-A `SAVGAMA.DAT`, used by the native watch | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Unit `0x1C:+0x339A..+0x33B8`, GAME file `0x40E0..0x40FE` | 31 bytes / 9 contiguous 16-bit instructions | `9526e20e6f32a143fc5647e2dd73efa5408bc67d1ab8e5c7004e25cbab650784` |
| Native `+0x339A` return RAM | 655,360 bytes | `5ab1d3fa57949ce7c3e2b4dbd0ff1d6d2f96787f0dd13bc8cbc10b44713bda37` |
| Native `+0x33BD` gate RAM | 655,360 bytes | `38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3` |

## Ordered source effects and native comparison

| Original instruction offsets | Effect represented by the C step |
| --- | --- |
| `+0x339A` | Store returned `AL` at caller local `SS:[BP-1]`. |
| `+0x339D..+0x33A7` | Read the live active far pointer at `DS:5D92/5D94` and copy its two words to `DS:4436/4438`. |
| `+0x33AB..+0x33B8` | Zero-extend live map byte `DS:84DC`, load the live window-0 far pointer, and write that word at window-0 `+0x1E4`. |

The following `+0x33B9..+0x33BC` instruction loads a far window-1 pointer
into registers and changes no RAM; the native gate snapshot is stopped at
`+0x33BD`, before its first comparison. The hash-locked snapshots differ
in exactly **five physical RAM bytes**. Four are the two copied pointer
words at `DS:4436..4439`: `4000:05AF` becomes `449A:0008`, matching the
unchanged active pointer. The fifth is local `SS:3EF7` (`SS=1D16`,
`BP=3EF8`), where `0xA0` becomes returned key `0x48`; this same physical
byte is addressable as `DS:C657` (`DS=14A0`). The source wrote the map
word, but its live value was already `0x0014`, so window 0 is byte-identical
at the two stops. Window 1 `+0x594` remains zero. These are one saved
native invocation's net effects, not a general key-input result.
The original 31-byte span is byte-identical in both loaded RAM images at
`3582:339A`.

The native comparison constructs the C state from the **live return RAM**,
passes `AL=0x48` from the observed register stop, applies the step, and
compares the local key, pointer words, and all 0x800 window-0 bytes with
the gate RAM. It also checks that those five addresses account for every
changed RAM byte between the two snapshots. The C API accepts other keys
and map IDs without hardcoding the saved result. Null or undersized host
window inputs return before any state change.

## Remaining C-native dependency

The raw `pr_slot_a_native_restore` state is only ten saved-file transfers.
The loader's later character/sidecar work, menu restored flag, unit
`0x1C:+0x32F8..+0x3386` initialization, and reached entry-2 prompt,
resident, and exit hooks must produce the **live** host inputs before this
return-side step is reachable. `pr_slot_a_native_one_up` therefore still
returns `PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH` without changing its state.
This step neither calls the coordinate writer nor assigns a party or clock
tuple. It adds no full-C routine, identity, or parity count.

`bash tests/replay/check_slot_a_key_dispatch_bridge.sh` runs synthetic
valid and rejected inputs under strict C11 warnings. Supplying the ignored
`GAME.OVR`, return RAM, and gate RAM paths additionally verifies all source
and snapshot hashes, the exact 31-byte span, and the five-byte C/native
projection. Original media and captures remain under ignored `resources/`.
