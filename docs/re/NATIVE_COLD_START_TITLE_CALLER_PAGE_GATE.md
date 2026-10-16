# Cold-start title display caller frame and EGA page boundary

This is a source-only continuation of the [two-image U17 audit](NATIVE_COLD_START_TITLE_SOURCE_GATE.md).
It identifies the return frame that would distinguish the two calls to
`RES_0464:+1951`, and the display routine's EGA plane and output-segment
controls. The original native run stopped at U17 entry and after return,
never at either display call or its framebuffer store. The first guest
writer of the visible title remains unmeasured.

Before this audit, `timeout 18s deja "Poolrad RES 0464 1951 1AA2 EGA plane
page caller return 0688 06F3 title"` timed out without an indexed result.
The [resident video-init gate](RESIDENT_VIDEO_INIT_4426.md) supplies prior
page-state semantics; the [U17 source audit](NATIVE_COLD_START_TITLE_SOURCE_GATE.md)
supplies the loader, archive and caller bounds. All new spans below were
checked against ignored `START.EXE` and the same U17 entry-stop RAM. No
original game bytes or captured pixels are published.

## Hash-locked display body

| Private input | SHA-256 |
| --- | --- |
| `START.EXE`, 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| U17-entry 640-KiB RAM | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

`RES_0464:+1951..+1B51` is 513 bytes and ends in `RETF 8`. The original
START file span `[0x05FCA,0x061CB)` hashes
`d64bb18e4503a5679481894c5ddaad44b3c0f703b00d1810afbbba8e9f358de3`;
the loaded `0C88:1951` span hashes
`b297f688470803b952d1304f5a0ba3c905d1ed63944e84a71a1810cbaf9d1d44`.
Only four relocation-operand bytes differ. The EGA setup through direct
store, `+1A29..+1AA4` (124 bytes), is byte-identical between source file
`[0x060A2,0x0611E)` and RAM, SHA-256
`01750fdc56f4ae3309b85e9b82fe3e5340d7f748008e4669f4851f2f2792d5a1`.
The three-byte store at `+1AA2` hashes
`7cc107bb3b15e14c0a2b41083f41a5748ab0fb6ac0bbd0122030d1da88bb4d92`.

## Far caller frame discriminator

The callee begins with `PUSH BP; MOV BP,SP` and retains BP through its loop.
For either far call, `[SS:BP+2]` is the return IP and `[SS:BP+4]` is the
return CS at a stop on `0C88:1AA2`:

| U17 call | Image ID in preceding wrapper | Expected return IP at the display store | Expected return CS in the pinned run |
| --- | ---: | --- | --- |
| `3909:0683` | 1 | `0688` | `3909` |
| `3909:06EE` | 2 | `06F3` | `3909` |

The original file callsites are `GAME.OVR` `0x00757` and `0x007C2`;
each five-byte original call hashes
`6cc3bf5965381d41d3fb6117d07aba4d8e740e591ee4d4decca3cdf8e3690898`.
Their loaded calls hash
`ac9be7058d9e3c62f4aa199563a8df0cfdaf35358460b3f3b89e17579be87346`;
the two changed bytes are relocation words. A future run may load U17 at a
different segment, but its unit-relative return offsets distinguish the
calls. These frame values are **static expectations**, not captured CPU or
stack state at a title-writing instruction.

## EGA plane and output segment

The display callee tests `DS:4A1E` at `+196B`; value 1 selects its EGA arm
at `+19F9`. Within that arm, `+1A29..+1A2E` selects sequencer index 2
through port `3C4`; `+1A64..+1A75` reads a plane-mask table at `DS:25F6`
and writes the selected mask to port `3C5`. The table bytes and device
response are outside this source-only note. The arm loads the constant
segment `A000` at `+1A83`, computes `DI` from a row-offset table at
`DS:5050` and the loop coordinate at `+1A8F..+1A9E`, then writes the
bitmap byte to `ES:[DI]` at `+1AA2`. It restores mask value `0F` through
port `3C5` at `+1ABE..+1AC3` before the far return. That six-byte restore
span is identical in source and RAM, SHA-256
`dc63d8b648aa2e43faa89e6af8880f774aa0e2e58757448932ef3cb2e80f4e88`.
No `DS:4A16` or
`DS:4A18` page pointer is read in this routine's decoded body; the routine
itself uses `A000` directly. The visible page and hardware plane state
therefore require a runtime observation.

At the three retained RAM stops (before U17, on U17 entry, and after its
return), **candidate** data segment `14A0` has `DS:4A16=A000`,
`DS:4A18=A000`, active/draw page indexes zero, `DS:4A1E=1`, and its
`DS:5050` first row-offset word zero. The CPU `DS` at U17 entry was not
retained and no intermediate page or port state was captured. These numeric
snapshots support a breakpoint plan, not a page-state assertion during the
ID-2 display.

Once the heavy-native hold is lifted, stop at U17 `+0690` to record CPU DS
and its branch byte; then at `+06CA`, `+06EE`, and `0C88:1AA2`. At the store,
record `[SS:BP+2/+4]`, CPU DS/ES/DI, selected plane and page state, and
before/after framebuffer or RGB hashes. A DOSBox-X `BPM` reports changed
values and can miss same-value stores, so the code breakpoint is needed to
test the first-write claim. No emulator was launched for this audit. There
is no C parity or game-function-count claim.
