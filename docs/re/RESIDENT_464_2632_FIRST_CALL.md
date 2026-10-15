# Resident 0464:+2632 first saved-view call

## Scope and provenance

This extends the reviewed bounded `RES_0464:+2632` one-row composition
from PR #484 to the **first** saved-slot-A run014 call. The first call is
at overlay `2D14:0198`, before the one-row call at `2D14:01BE`.
The first path draws 43 rows and returns to `2D14:019D`; it is still
a bounded path model, not a full resident translation or a strict-count
increment.

Before decoding, `deja "Poolrad RES_0464 2632 first call 0198 019D run014
native entry return branch parity"` was run with a 30-second bound and
timed out without a result. Reused work: merged
[one-row composition](RESIDENT_464_2632_RUN014_COMPOSED.md) from PR #484,
and pD's local `U3BD_ENTRY5_05FD_NATIVE_FRAME` run014 numeric note.
Original media and RAM snapshots remain ignored.

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its resident `+2632..+2A0D` span has 987 bytes at file
`0x06CAB..0x07086`, SHA-256
`ee35f98f513d5144e92e55e2afafbe176bba9ab65acc905ea151c19c5a9d713a`.
The original five-byte far call at `GAME.OVR` file `0x2EA8F`
decodes to `0464:2632`, SHA-256
`a734c1740b95619bedad6325a3ac8d1413024a9cbb9302bbf3c15831cfb20cfe`.
The same-run loaded call at `2D14:0198` targets `0C88:2632`,
SHA-256
`0d18bd5c35579c163826a0665cd04cf9a8b7b054c3f930e7633f0c6bdc4352f4`.

## Reached shape and side effects

The `+0198` caller stack holds destination object `3DE4:0008`,
source `14A0:260A`, selection mask `0x000B`, 43 rows, 11
four-byte elements per row, starting row 16, horizontal index 2.
The object header gives width 21 and height 168. `DS:4A1E=1`
selects the `+28E0` branch. Table tests at `DS:25F6..25F9`
select three of four bytes in each four-byte copy. The outer loop
advances row 16 through row 58; `DS:70C7` ends at 11.

From the same run014 `+0198` caller pre-image to the `+019D`
return, exactly 1,730 RAM bytes change. The changed-address set
partitions without leftovers:

| Region | Changed bytes |
| --- | ---: |
| Offscreen object inside the 43×11 four-byte destinations | 1,419 |
| Dirty/extent metadata and final counter | 259 |
| Stack and local frame | 52 |

The 1,419 object changes equal 43 rows × 11 elements × three selected
bytes. No object byte outside those destination ranges changes. The
pre-image SHA-256 is
`76451936350f53c3cc9732cc1ec48fed834aa0e54a25292f76609280ecd0ca21`.
The native `+2A07` loop-exit and `+019D` caller-return images have
SHA-256
`0bc7e0b0b162c6b1768d22177b721917236df0b673c183612fa2b49f3d42a45b`.

## C comparison and boundary

The optional private mode in
`tests/recovered/test_resident_464_2632_first_call.c` takes the
unchanged `+0198` caller image, executes only its byte-verified far
call's two return-word pushes, then runs the bounded C composition.
Full 640 KiB RAM matches the same-run native image exactly at both
`+2A07` and `+019D`: zero differences. The focused synthetic test
checks 43 rows, the three selected bytes in each four-byte element,
per-row dirty/extent values, the return target, and an unsupported-mask
no-effect case. PR #484's one-row path remains exact at its 14 original
RAM/CPU checkpoints.

The optional
`tools/replay/resident_464_first_call_unicorn.py` checks the original
executable and call-site hashes, starts the loaded original bytes at the
same run014 `+0198` caller prestate, and executes 17,156 instructions
to `+019D`. It observes the first resident entry
`0C88:2632` with entry RAM SHA-256
`d6dcdcc658dab6ded31f07a2205bfb71c5b635dea416fbe7f6bff339e77f1a68`.
The original-byte replay matches the same-run native 640 KiB RAM at both
`+2A07` and `+019D`, and its full return CPU tuple equals C:

`2D14 019D 002A 3E4E 0000 14A0 3E68 0074 3E98 3E98 14A0 3DE4 1D16 7246`

That CPU comparison is against independently executed original bytes.
A direct DOS debugger CPU tuple at the first resident entry and return
is requested separately and is not claimed here. Other `+2632`
modes, clipping branches, and table shapes remain outside this model;
no full-function or game-identity credit is claimed.
