# Fourth saved-view runtime FillChar return

## Scope and prior work

This bounded C model starts at resident `0C88:+291C`, executes its
relocated far call to runtime `131C:16A8`, and stops at resident
`0C88:+2921` after `RETF 8`. It models only the saved fourth-call
input, value zero and count four. It does not continue the resident
caller or claim a complete routine or a strict translation count.

Before implementation, `deja "Poolrad fourth RES_0464 291C 131C
16A8 FillChar 2921 native original byte parity"` was run; the
bounded query timed out after 35 seconds. The reviewed +291C prestate
and address mapping were reused from
[RESIDENT_464_2632_FOURTH_PRECALL.md](RESIDENT_464_2632_FOURTH_PRECALL.md).
The runtime FillChar calling pattern was reused from
[RESIDENT_464_2632_RUN014_COMPOSED.md](RESIDENT_464_2632_RUN014_COMPOSED.md)
and `src/recovered/resident_464_2632_run014.c`. This gate checks the
fourth-call inputs and effects independently.

## Address and instruction evidence

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The five-byte far call at file `0x6F95`, loaded `0C88:+291C`, has
original-file SHA-256
`adc5985dcb761563534658cb03a15d9afdf2305d7e2b82f800875d921f91ee3a`.
The relocated loaded call SHA-256 is
`2776d6ebc232e7277e121d58341055aaa57b341e24b5edb8a4d116e421da1db3`;
its resolved target is `131C:16A8`. The loaded 20-byte runtime span
`131C:16A8..16BC` has SHA-256
`f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7`.

The runtime uses `BX=SP`, loads the far destination and count from
the caller's stack, loads the byte value, clears DF, repeats byte
stores with `STOSB`, then returns far while discarding eight argument
bytes. On the saved input, the destination is `1D16:3E64`, count four,
and value zero. The first three target bytes change to zero; the
fourth was already zero. The call also writes four return-frame bytes
at physical `0x20FB0..0x20FB3`. Thus seven bytes differ between the
+291C and +2921 RAM images. No EGA or object byte changes in this
bounded call.

The ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`
at entry is:

    0C88 291C 0000 0000 0068 0000 2612 3E64 3E80 3E54 14A0 3DE4 1D16 7202

At return it is:

    0C88 2921 0000 3E50 0000 0000 2612 3E68 3E80 3E5C 14A0 1D16 1D16 7202

The private input RAM SHA-256 is
`3ef5853b7c52f12f7ec562b8c9f98e91274bfd5c1dcaec4152130033a7b66ba6`;
the original-byte return RAM SHA-256 is
`4b02a5589727a60b13ab8e62ef21904c5257f9583015f7e8b902d7292f5dcc8c`.
The public replay verifies both call forms, the loaded runtime span,
and 13 sequential instruction checkpoints, including five `REP STOSB`
visits. The C model matches **all 640 KiB RAM and all 14 CPU fields at
all 13 checkpoints**. Synthetic tests verify the return frame, four
byte stores, return CPU, unsupported count/entry without mutation,
and invalid host inputs.

## Reproduction and limit

With `PYTHONPATH=/tmp/poolrad-unicorn`, run
`tools/replay/resident_464_fourth_fillchar_unicorn.py` on the ignored
saved +291C RAM image. Optional `--snapshot-dir` must point beneath
ignored `resources/`. The focused C executable accepts that private
pre-RAM path and snapshot directory as two arguments; normal CTest
uses synthetic RAM only.

These checkpoints are from original-byte replay. Direct same-run
native +291C/+2921 capture remains pending with pD. The separate
fourth resident return has an 18-byte IRQ-like residue; this gate
does not erase or mask it, and makes no full-call or count claim.
