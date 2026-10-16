# U7D +03A3: loaded U1C E-decision window

This source-only packet extends the pre-input RAM guard in
[U7D_UPDATE_03A3_PREINPUT_RAM_GUARD.md](U7D_UPDATE_03A3_PREINPUT_RAM_GUARD.md).
The retained slot-A run pressed **Up**. It did not press E or reach a U7D
`+03A3` call. The three available RAM stops are 640 KiB each and do not
contain a same-run CPU14/1 MiB caller frame.

`GAME.OVR` SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
places U1C at file base `0x0D46`. Original `+33BD..+33EA` is 46 bytes,
SHA-256 `cfade238c89c0ca9be90dbd2581b61cd8c3fcffd651a544c719d4fd947ba71f7`.
The corresponding loaded `3582:33BD..33EA` span has SHA-256
`75a88ab71babe0b9689c459186b3ff1082d0f3f5cd2e69c47a590bbfa46b5a21`
at pre-input `+3395`, post-Up `+33BD`, and later `+026C` stops. The
only four byte differences are the segment words of two far calls:
`+33CC/+33CD` and `+33E0/+33E1`, each `F8 0A` originally and
`1C 13` loaded. Both `CMP AL,45h` tests (`+33CE`, `+33E2`) remain
byte-identical.

The next natural caller gate needs one continuous E/R/I-or-D run. Capture
CPU14 and 1 MiB RAM at the actual loaded U1C E decision, the reached
R/I-or-D transition, and the reached U7D `+03A3` call and return. Record
the loaded caller CS:IP, branch direction, input byte, DS/window pointer
and guard words, arguments, and full entry/return state. The static loaded
code identity here cannot establish those transitions. Heavy native capture
remains on PM hold; strict full-C count remains 82.

Run the focused source checker with the original `GAME.OVR` and the three
ignored `slot-a-key-route-live-013` RAM files:

`python3 tests/recovered/check_overlay_u7d_03a3_loaded_e_window.py --ovr resources/DOS/POOLRAD/GAME.OVR --pre-input-ram <pre-call-3395-ram.bin> --post-up-ram <loop-gate-33bd-ram.bin> --later-ram <pre-random-026c-ram.bin>`
