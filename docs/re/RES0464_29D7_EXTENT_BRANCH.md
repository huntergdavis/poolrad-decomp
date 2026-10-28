# Bounded RES0464 `+29D7` extent branch

This source-only gate starts at a separately supplied `0C88:29D7` Move-loop
exit frame. It executes the loaded `MOV AX,[BP-16]`, `DEC AX`,
`MOV DI,[BP-1E]`, `SHL DI,1`, `CMP AX,DS:[DI+4D70]`, and JBE through
`+29E4`. It stops at `+29E6` when the local end exceeds the stored
extent, or `+29F3` otherwise. Neither successor's store is executed.

The ignored original `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The loaded 15-byte span at `0C88:29D7` is
`[scrubbed: span=RES0464_29D7_EXTENT_BRANCH-1 sha256=e83bac0b43d054343210ce1c9210898d5438d06e5b988bb258b9f9c6af0dc7ba bytes=14 offset=UNRESOLVED]0D`, SHA-256
`77c99f2b8a4298ab945d6692895d264a293029f10415df8bfa25d33b4bfef5b7`.
This was checked in the ignored run015 source image and the earlier run014
`+29D7` checkpoint image, SHA-256
`5e242c554a9131338bd458e70abb2f6c844e0c76499baf21bffc6004c0cfd91e`.
No image is committed.

The supplied frame matches the existing run014 checkpoint in
`tests/recovered/test_resident_464_2632_run014.c`: `CS:IP=0C88:29D7`,
`AX=000B BX=3E4E CX=0000 DX=14A0 SI=3E68 DI=13AF BP=3E80
SP=3E5C DS=14A0 ES=3DE4 SS=1D16 FLAGS=7246`. Its return residue
and five Move argument words remain at `SS:3E4E..3E5A`, the count and
counter are both `0B`, the local end is `SS:3E6A=1390`, and the row is
`SS:3E62=003B`. The earlier source model
`src/recovered/resident_464_2632_run014.c` supplies the 16-bit CMP flag
rules used here.

`AX` becomes `138F` and `DI` becomes `0076`. The saved extent at
`DS:4DE6` selects the successor: zero gives `FLAGS=7202`, `IP=29E6`;
`138F` gives `FLAGS=7246`, `IP=29F3`; and `1400` gives `FLAGS=7283`,
`IP=29F3`. The gate writes no RAM. Strict C11 ASan/UBSan synthetic tests
compare all CPU fields and 640 KiB RAM for all three cases and check
rejection without side effects for changed bytes and frame fields.

This does not assert that the fourth run015 path naturally reaches
`+29D7`, nor does it model either successor after its first instruction.
It adds no full resident or native parity credit. The required earlier
bounded `deja` attempt timed out; indexed recall remains inconclusive.
