# Cold-start credits first printer: static caller and glyph boundary

This source-only gate follows the first credits string call after the two
box calls in U17's local credits procedure. It records the caller's exact
operands, the printer's copy and first glyph-call boundaries, and static
return offsets. It neither executes the printer in a native guest nor
claims a live framebuffer writer or full-frame parity. The separate
[credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md) and
[credits renderer](COLD_START_CREDITS_SYNTHETIC_RGB.md) own source-to-frame
composition; the separate first-border audit in PR #602 ends before
this call.

Before this audit, `timeout 20s deja "Poolrad cold-start credits first
RES_0709 0626 printer U17 +0206 33 calls font glyph first caller source"`
timed out without a result. The merged
[printer source map](FONT_8X8D_201.md),
[printer prefix gate](RESIDENT_0709_0626_FIRST_COPY_GATE.md), and
[post-copy guard gate](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) were reused
as leads. The spans below were checked again against ignored DOS v1.3
originals and retained U17-entry RAM. No emulator was started under the
host's heavy-native hold.

## Pinned first caller

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 `+0253` is the second box call; its static return is `+0258`.
The following 29-byte first-printer setup and call, U17
`+0258..+0275`, maps to GAME.OVR `[0x0032C,0x00349)`.
Original SHA-256 is
`8187a27c7f9b29080533939d06aecf7a864b0241cdffff36c8bd584268ca8415`;
loaded `3909:+0258..+0275` SHA-256 is
`b0b117bbd263c18504e75d9b7380a74cb836278b90dbeec4bbdb59df0c88bb2a`.
Its decoded low-byte operands are column 5, row 1, color 10. It passes
inline Pascal source `CS:+004B` and stack-local destination
`SS:BP-0x15` through SYSTEM `+0B0B` at `+026B`, then calls
`RES_0709:+0626` at `+0270`. The original five-byte printer call at
GAME.OVR `[0x00344,0x00349)` hashes
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`;
the relocated loaded call hashes
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`.
Its static return offset is U17 `+0275`.

The source at U17 `CS:+004B` maps to GAME.OVR `[0x0011F,0x00134)`:
one Pascal length byte plus 20 data bytes. The 21-byte original and
loaded RAM spans are identical, SHA-256
`e897e2280b05c12e41e8a07a06fd0d659631d827632b20f09c3c96203486c551`.
No game text is reproduced here. The source length is within the
printer's 255-byte copy cap, and the starting column plus 20 cells is
within the 40-column display bounds. These are source operands and
bounds, not a captured call or per-cell native result.

## Printer and first glyph call

`RES_0709:+0626..+06A7` is 129 bytes, original START file
`[0x076EF,0x07770)` SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`;
loaded `0F2D:+0626..+06A7` SHA-256
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
It receives a far string pointer, color, row, and column on the stack.
At `+063C` it calls SYSTEM `+0B25` to copy the Pascal source into a
255-byte local buffer, returning to `+0641`. Its entry guards compare
column and row with 39; source operands 5 and 1 take the accepted branch
if the preceding copy returns normally. At `+0690..+0694`, `PUSH CS;
CALL +01DF` sends one character with count 1 and upcase flag 1 to the
glyph drawer. That four-byte call is identical in original and RAM,
SHA-256
`c6612c7266fcdf0defe91d5cf5d45a77c0d2e3d1056fcf31a0da6a76f3d9d4cf`.
The static glyph return offset is printer `+0694`; the printer's
`RETF 0x0A` at `+06A4` returns to U17 `+0275` after its loop.

The first glyph's source byte, CPU `DS`, EGA mode/page, and any display
write remain unobserved at this call. The existing glyph model describes
an EGA drawing branch, but this note does not assert that the branch
executed in the cold-start native run. It adds no C translation,
game-function count, or native/full-frame parity credit.

## Separate follow-ons

1. U17 `+028D` (GAME.OVR `0x00361`) is the **second** credits printer
   call with a different setup and source. Its operands and source path
   can be audited independently of the first call and the border note.
2. U17 `+0619..+061D` (GAME.OVR `[0x006ED,0x006F1)`) is the terminal
   `PUSH CS; CALL +0000` transfer to its local wait/flush routine
   after the printer sequence; the call instruction starts at `+061A`.
   Its timer and display effects are a separate control-route gate.

Both follow-ons require separate ownership screening before publication.
Any native glyph/page stop waits for the heavy-native hold to lift.
