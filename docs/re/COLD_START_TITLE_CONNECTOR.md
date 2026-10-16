# Cold-start title file connector

This is a bounded portable C connector for one title frame. It reads an
explicit `asset_root/TITLE.DAX`, calls the separate title renderer once, and
creates an explicit output file containing 640×400 top-down RGB24 bytes. The
output path must be absent. It does not launch DOS, advance the startup route,
handle input, or render the code-word prompt or menu.

Before this work, `timeout 35s deja "Poolrad title startup connector static
title frame portable C call seam issue 511"` reached its timeout without a
result. The existing [U17 picture route](NATIVE_COLD_START_U17_PICTURE_ROUTE.md)
and [continuous screen sample](NATIVE_COLD_START_SCREEN_GAPS.md) supplied the
native bounds reused here. A no-argument start reaches resident image
`+0x011D`, enters unit `0x17` at `3909:0627`, and returns at image `+0x0122`.
The separate title frame appears in continuous native video at sample slots
359–433 with RGB24 SHA-256
`3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8`.
That sample does not identify its first writing guest instruction.

The connector receives asset and output paths from its caller. It accepts a
`TITLE.DAX` file up to 1 MiB, passes its unmodified bytes to
`pr_cold_title_rgb24`, requires a successful 768,000-byte frame, and creates
the output exclusively. Asset load, renderer, and output failures have
separate statuses. A missing asset or unsuccessful renderer leaves the output
untouched. The renderer owns archive block selection, image decoding, EGA
palette, and scaling; see its separate `cold_start_title_rgb` contract.

The focused test uses a four-byte synthetic asset and a fake renderer linked
against the committed renderer header. It checks one renderer call, output
length, failure boundaries, and exclusive output creation. A strict C11
configure/build ran this test and the separate renderer test with 2/2 passes:

```sh
cmake -S . -B resources/build-p9-title-connector \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-p9-title-connector \
  --target poolrad_cold_start_title_connector_tests \
           poolrad_cold_start_title_rgb_tests --parallel 2
ctest --test-dir resources/build-p9-title-connector \
  -R '^poolrad_cold_start_title_(connector|rgb)_tests$' --output-on-failure
```

A private scratch driver linked this connector, the real renderer, and
`poolrad_formats`. On ignored `TITLE.DAX` SHA-256
`7944ab87afbe0002d39479d138c78c7b6b219bd6bbc3fb6844d361f5cd4dba2b`,
the connector returned success and wrote exactly 768,000 RGB24 bytes with
SHA-256 `3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8`.
The scratch driver and frame remain under ignored `resources/`. This is
asset-to-frame parity through the connector, not execution of the native
startup route or a full C startup claim.
