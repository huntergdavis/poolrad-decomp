# Fourth saved-view RES_0464:+28D9 pre-call gate

## Scope and prior work

This bounded C model evaluates +28D9 CMP/JE and the saved +28E0
arithmetic and stack setup. It stops **before** the +291C far call to
runtime FillChar. When AL is not 1, it stops before +28DD. Neither
successor nor the runtime call is executed by this model.

Before implementation, deja was run with query "Poolrad fourth
RES_0464 2632 28D9 saved run014 first external call branch". The
35-second query timed out. The preceding selector stop and its
hash-locked CPU/RAM prestate were reused from
[RESIDENT_464_2632_FOURTH_SELECT.md](RESIDENT_464_2632_FOURTH_SELECT.md).
The previously reviewed first-call +28E0 prelude and the runtime
FillChar dependency were reused as architectural evidence from
[RESIDENT_464_2632_RUN014_COMPOSED.md](RESIDENT_464_2632_RUN014_COMPOSED.md)
and `src/recovered/resident_464_2632_run014.c`; this gate independently
checks fourth-call inputs and effects.

## Address and instruction evidence

The ignored START.EXE SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
File 0x6F52..0x6F95 maps to loaded 0C88:+28D9..+291C.
Both 67-byte spans have SHA-256
99cefaacb29da0a174c631b7e7f3f38832f8d8ba2b6b383ec54259ff82ea6192.
The saved +28D9 AL is 1, so +28DB JE reaches +28E0.

The saved prelude reads six stack locals, copies/scales three words,
computes one count word, and pushes the far address of BP-0x1C,
count 4, and value 0. It reaches +291C with SP 0x3E54 and DI
0x3E64. The call instruction at +291C targets original runtime
0AF8:16A8; its effects remain outside this gate. At the +291C stop,
12 bytes differ from the input RAM, all in the local stack frame and
outgoing argument area. The ordered CPU tuple
CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS is:

    0C88 291C 0000 0000 0068 0000 2612 3E64 3E80 3E54 14A0 3DE4 1D16 7202

The input 640 KiB RAM SHA-256 is
95c91ee38f22f5d3808a851ddfdb055aa2733b70bfd7bf3ba897ed8bed7f621c;
the original-byte +291C RAM SHA-256 is
3ef5853b7c52f12f7ec562b8c9f98e91274bfd5c1dcaec4152130033a7b66ba6.
The public replay locks the file and loaded spans and verifies
seven original-byte checkpoints: +28DB, +28E0, +28FC, +2906,
+2910, +2914, and +291C. Their changed-byte counts are respectively
0, 0, 1, 2, 4, 6, and 12. Generated snapshots stay in ignored
`resources/`. The C model matches **all 640 KiB RAM and all 14 CPU
fields at all seven stops**, and follows all 27 intervening
instruction boundaries. Synthetic tests check the alternate +28DD
branch, every changed local and pushed word, invalid host input, and
unsupported saved-path source locals without mutation.

## Reproduction and limit

Use `PYTHONPATH=/tmp/poolrad-unicorn` with
`tools/replay/resident_464_fourth_precall_unicorn.py`, the ignored
saved pre-RAM image, `--snapshot-dir` under `resources/`, and
`--branch-other`. The focused C test accepts that pre-RAM path and
snapshot directory as its two private arguments. Normal CTest uses
synthetic RAM only.

These are original-byte replay versus C results. Direct same-run
native +28D9/+291C intermediate capture remains pending with pD.
The separate fourth-return 18-byte IRQ-like residue prevents a
full-RAM return, complete-function, or strict-count claim.
