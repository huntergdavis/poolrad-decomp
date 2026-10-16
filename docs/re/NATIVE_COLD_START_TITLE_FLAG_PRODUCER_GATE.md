# Cold-start title branch flag: static producer and consumer

This source-only audit joins the resident startup tail that assigns
`DS:49D1` to the unit `0x17` title-display branch that reads it. It does not
observe either assignment or branch in a running guest. The title frame's
first native writer and the full cold-start control route remain open.

Before inspecting the bytes, `timeout 20s deja "Poolrad DS 49D1 producer writer
U17 title publisher display ID2 branch"` timed out without a result. This
audit reuses the loader mapping in [RESIDENT_LOADER_MAP.md](RESIDENT_LOADER_MAP.md),
the prior `DS:484C` adapter ambiguity in
[RESIDENT_VIDEO_INIT_4426.md](RESIDENT_VIDEO_INIT_4426.md), and the measured
U17 entry/return and title call bounds in
[NATIVE_COLD_START_TITLE_SOURCE_GATE.md](NATIVE_COLD_START_TITLE_SOURCE_GATE.md).
No new emulator was started while the heavy-native slot was held.

## Pinned inputs and mapping

| Ignored private input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| native U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

`resident_loader_map.unpack` maps the 51-byte resident tail at expanded image
`+460D..+4640` contiguously to START file `[0x04646,0x04679)`. It hashes
`3109ec3292b6260aad451bbbfc01e821dbfb3c896d29efe3cecef4f121e81cc1`.
In the retained native run, the load base was `0824`; the same tail at
`0824:460D..4640` hashes
`317642f0d564561040c35ecd67c5f33095f1236bf01c00a049e853866c837d8e`.
Six bytes differ, confined to the segment words of its three far calls.
The containing routine has a prologue at image `+4354` and ends with `RETF`
at `+463F`; the 51-byte tail, rather than its entire earlier body, is the
control-flow scope here.

The 19-byte compare and assignment span at image `+461F..+4632` maps to
START file `[0x04658,0x0466B)`. Original, expanded, and U17-entry RAM bytes
are identical, SHA-256
`c4c6daa1827c2b9bc7c74d4b86b1676fd3b6527f4a95f04fe37828332ede6700`.
Its loaded address is `0824:461F..4632` (physical `[0x0C85F,0x0C872)`).

## Bounded source control flow

| Expanded image site | Decoded action | Boundary |
| --- | --- | --- |
| `+460D..+4617` | Pass `DS:484D` to far callee `RES_0AF8:+0898` and store its returned `AL` at `DS:484C` | The callee and its returned value were not measured in this run. |
| `+461A` | Call `RES_0AF8:+081C` | Its side effects on data memory are not established here. |
| `+461F..+4626` | Compare byte `DS:484C` with `0x46`; unequal jumps to `+462D` | The compare is after the `+081C` call. |
| `+4626..+462B` | Equal arm writes byte `1` to `DS:49D1`, then jumps to `+4632` | Five-byte store SHA-256 `6285088d7f4e513fa29c94b647a4ea8b5732ad64bb86128d418b5c43bc6aa9a2`. |
| `+462D..+4632` | Unequal arm writes byte `0` to `DS:49D1` | Five-byte store SHA-256 `a946b63af2f93d8794153c837046d92c86d553142f6d1328f2201288cc4bbe2e`. |
| `+4632..+4640` | Pass `DS:484D` to `RES_0AF8:+05C1`, then restore the stack frame and far-return | No later direct `49D1` store in this bounded tail. |

The expanded START image contains two literal `D1 49` address occurrences,
both in those decoded immediate-byte stores. Original `GAME.OVR` contains
one occurrence, in U17's direct read. This is a direct-literal census only:
indirect pointers or block moves could also write `DS:49D1`.

At loaded unit `0x17:+0690..+0697`, the seven-byte compare/JNE reads
`DS:49D1`, compares it with zero, and jumps to `+06AD` on nonzero. Its
original `GAME.OVR` source is `[0x00764,0x0076B)`; original and loaded
bytes are identical, SHA-256
`349191955f710cbf09de2d115647653dc4ac17004d72946ee1268689d6d02031`.
The zero arm reaches `+06AA`, which jumps to `+0805`. The nonzero arm begins
at `+06AD`, passes block ID 2 to `RES_0464:+0111` at `+06CA`, and calls
`RES_0464:+1951` to display it at `+06EE`. These are decoded successors,
not a same-run record of the branch outcome.

At the retained stops before U17, at its entry, and after its return,
candidate segment `14A0` holds `DS:484C=0x46` and `DS:49D1=1`. The three
RAM hashes are respectively
`3de44fb40c3490f5e540d7ec1ffff7e764326eb6badcfc1638144ac08bfb45d6`,
`11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6`,
and `6e0ae646dc59e13992f760e0a899c7482d62b62b375f7d1f2934e514f4a4a3fe`.
The CPU `DS` at U17 entry was not retained, and there is no stop at the
producer. These endpoint values are consistent with the equal arm; they do
not prove how `0x46` was obtained or that either static store executed.

A future native run can stop at `0824:4617`, `+461F`, and `+4626/+462D`
with CPU `DS`, then at U17 `+0690/+06AD/+06CA`. That would test the producer
and branch without inferring them from a later memory snapshot. This audit
adds no C translation, no native first-writer claim, and no game-function
count.
