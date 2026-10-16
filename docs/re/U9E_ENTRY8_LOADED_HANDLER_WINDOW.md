# U9E entry8: loaded INT 3Fh handler identity

This source-only packet extends
[U9E_ENTRY8_LIVE_STUB_CAPTURE_CONTRACT.md](U9E_ENTRY8_LIVE_STUB_CAPTURE_CONTRACT.md)
with an exact check of the handler present in the retained Up-run RAM.
The retained stops did not reach U7F `+0FDE`; U9E `08D4:0048` stayed in
its cold `CD 3F E3 0D 00` form. They cover 640 KiB each, with no same-run
CPU14/1 MiB seam or observed loaded U9E body CS.

`START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
expands to original handler `0AA2:02E6..0337`, 82 bytes, SHA-256
`6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c`.
At all three retained stops, IVT vector 3Fh is `12C6:02E6`, and the
loaded 82-byte handler there has SHA-256
`5c69a0e6a0a9141fae2d0381a4ebe4252981d54cbad01d335a3f5f73380fd6ef`.
Only its DS immediate differs from the original: `+02F2/+02F3` change
`7C 0C` to `A0 14`. This identifies the installed handler and its
relocated data segment; it does not show any U7F-to-U9E transfer.

The next reached-caller gate needs a single run with CPU14 and 1 MiB RAM
at the loaded U7F `+0FDE` call, the `08D4:0048` INT 3Fh stub before and
after dispatch, the `12C6:02E6` handler and IVT, and the loaded U9E
`+0DE3` body entry and return. Record the actual CS:IP, SS:SP, flags,
registers, stack words, target overlay/body CS, and transfer path. The
current handler identity and cold stub establish only preconditions.
Heavy native capture remains on PM hold; strict full-C count remains 82.

Run the focused checker with the original `START.EXE` and the three ignored
`slot-a-key-route-live-013` RAM files:

`python3 tests/recovered/check_overlay_u9e_entry8_loaded_handler_window.py --exe resources/DOS/POOLRAD/START.EXE --pre-input-ram <pre-call-3395-ram.bin> --post-up-ram <loop-gate-33bd-ram.bin> --later-ram <pre-random-026c-ram.bin>`
