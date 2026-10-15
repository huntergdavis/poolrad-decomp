# Saved slot-A: view-three loop after the first view-four zero gate

The reviewed C object at unit 0x3BD:+0x0AA9 has SHA-256
e2a8d70f54501d664f3d47e50e0b614d563f36dbc3cde6eb15de6210d1c0ce94.
An independent saved-A native run reaches the first view-four zero gate
at 2D14:+0x0AD8 with the same 14,135-byte object hash. These captures
are from **different runs**; object equality does not establish all-RAM
or CPU equality.

Within the second run, the next measured stop is the third scan
iteration at 2D14:+0x0AD5, before the view-four placement call. The
original loop takes two more view-three placement paths between these
stops. A bounded C prefix emits their 24 calls to the reviewed
group-zero and group-two tile models. On one persistent C object, the
result equals **14,135/14,135** native object bytes at +0x0AD5. The
first **modeled** value change is call 4, a group-two tile. There is no
immediate native stop at that call, so this is not direct proof of the
first actual native value-changing instruction. The original masked
store on the supported resident tile path is RES_0464:+0x0C6A.
That instruction was directly watched on a separate later view-four
route in the [first offscreen writer audit](SLOT_A_POST_VIEW4_FIRST_OFFSCREEN_WRITER.md);
this two-placement run has no per-call native write watch.

This prefix ends at the measured offscreen-object stop. It does not
model the next view-four placement, return to its caller, visible EGA
page, or palette/RGB. It adds no strict full-C or game-identity count.

Before this implementation, deja with the query
"Poolrad U3BD entry5 +0AA9 successor first view three group two
object-to-page 615 byte writer" timed out after 30 seconds. Reused
and checked the merged [view-three placement](SLOT_A_U3BD_VIEW3_PLACEMENT.md),
[view-four gate](SLOT_A_U3BD_VIEW4_PLACEMENT.md),
[group-zero tile effect](SLOT_A_U40C_FIRST_TILE_PLANE.md), and
[group-two tile effect](SLOT_A_U40C_GROUP2_TILE_PLANE.md). A fresh
private PTY run named slot-a-view3-view4-join-p5-002 entered an unintended
codeword input. Its ignored artifacts supply **no** saved-A parity and
are excluded from this audit.

## Original path and exact bounds

Ignored DOS v1.3 GAME.OVR SHA-256 is
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614;
START.EXE is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Unit 0x3BD original file base is 0x2E8F7.

| Role | Half-open original file span | Length | SHA-256 |
| --- | --- | ---: | --- |
| Scan and view-three/view-four gates | GAME.OVR [0x2F368,0x2F3CF), unit +0x0A71..+0x0AD8 | 103 | f9b268663cd179e399a25f72b62ec4fdd98b1a1f75c5941e3e3c50ff23fc9219 |
| Reached loop update | GAME.OVR [0x2F3CF,0x2F3FD), unit +0x0AD8..+0x0B06 | 46 | 575f649bcdaff7feb5a4aae000a2363f7ab2a5023cf63abd7e8eff1d342b8f3b |
| Local piece walker | GAME.OVR [0x2EB08,0x2EC16), unit +0x0211..+0x031F | 270 | b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03 |
| Resident tile callee | START.EXE [0x4F4D,0x53EA), 0464:+0x08D4..+0x0D71 | 1,181 | 9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe |
| Supported resident masked-store path | START.EXE [0x52E3,0x52E7), 0464:+0x0C6A..+0x0C6E | 4 | f1798d82c73cbb1d10f18e8d3d5f89265a778c5e2ea9cdfaca48e0dd4d68ec90 |

At +0x0AD8, the original increments [BP-0x10], adds three to
[BP-0x12], and signed-adds DS:[0x2736+direction] and
DS:[0x273F+direction] into the sample x/y words. It then jumps to
+0x0A71. On this saved frame BP=0x3EB6, direction index zero, scan
index zero, sample (14,6), and lateral offset -6. The two updates
produce samples (14,5) and (14,4) with lateral offsets -3 and zero.
The hash-locked GEO lookup returns view-three walls 9 and 14, and
view-four walls zero and 15. The zero branch at +0x0AC0 skips the
second view-four placement; the nonzero third gate reaches +0x0AD5,
where this prefix stops.

Both nonzero view-three gates call local +0x0211 at +0x0AA6. The
local walker sends positive cells through +0x02FB to unit 0x40C entry 2.
All 24 reconstructed positive items use group zero or group two, with remaps one
and 116. Flag one and video state one select resident 0464:+0x08D4.
Its +0x0C6A instruction writes (old AND row_mask) OR source_byte to
a destination plane byte. The C prefix leaves those effects behind a
callback; the audit dispatches that callback to reviewed C tile models.

## Measured native/C boundary

Both 640-KiB RAM images are from ignored
resources/oracle-runs/slot-a-art-view4-p5-006/:

| Native stop | Full-RAM SHA-256 | Offscreen-object SHA-256 |
| --- | --- | --- |
| First view-four zero gate, +0x0AD8 | a5080458fedcb618628e6387c692141a24b1b1073b9cac7b86fdc86a76040fa7 | e2a8d70f54501d664f3d47e50e0b614d563f36dbc3cde6eb15de6210d1c0ce94 |
| Third iteration before view-four call, +0x0AD5 | 50f0dfe340697048d8fefeb0b89bab79b68e343fabe79acb20260ac15ab576e3 | f54d5c29e66e399b7bf9ceae7685c1b83e2a00fd15cb55a7dd771a48a9c48e3d |

Native full RAM differs at 330 addresses: 283 inside the offscreen
object and 47 outside. The C prefix performs 24 callbacks; the tile
models report 768 plane-byte stores and reproduce the whole native
object at +0x0AD5. The first three modeled calls have no value change;
call 4 changes 16 C object bytes. Applying only this C object result
to a copy of the first 640-KiB RAM image leaves exactly 47 native
differences: 46 within the stack frame and one timer byte at physical
0x46C. This is a full-RAM comparison with an explicit residual,
**not** full-RAM parity. Dirty metadata and transient same-value
stores between stops are unproved.

The synthetic CTest exercises both view-four gates, ordered callbacks,
signed movement, and callback failure after an earlier call. It uses
invented GEO and piece data.

## Reproduce

Run the hash-locked audit with
python3 tools/replay/u3bd_view3_next_writer_audit.py.
Configure with CMAKE_C_FLAGS=-Wall,-Wextra,-Wpedantic,-Werror, build
poolrad_overlay_u3bd_view3_next_writer_tests, and run that CTest target.
