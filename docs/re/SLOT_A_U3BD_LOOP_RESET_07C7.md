# Saved slot A: U3BD entry-5 loop reset at +07C7

This is a bounded art-route slice, **not** a full translation of entry 5 or
proof of a complete frame. It begins at loaded `2D14:07C7` and stops before
the first successor instruction at `2D14:07E9`.

Before decoding, `deja "Poolrad saved slot A U3BD +07C7 +07E0 +07E4 loop
reset first successor C parity"` timed out after 15 seconds. The merged
[view-nine old-wall note](SLOT_A_U3BD_VIEW9_OLD_WALL.md) supplied the earlier
entry-5 context through `+0795`; the subsequent `+0795` loop-exit capture was
PM reviewed in the isolated art lane. This note independently pins the later
`+07C7` frame and its reached successor.

## Original boundary and behavior

The ignored `GAME.OVR` input has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x3BD` file `[0x2F0BE,0x2F0E0)` is 34 bytes, SHA-256
`d9ba05c7b1926e3ec4fbd0be0e22de72e110c76470f325ab1fd8759993683c5f`.
The same span appears at loaded `2D14:+07C7..+07E9`; Capstone 5.0.7 decodes
11 aligned 16-bit instructions. `+07E9` is the first successor and is not
executed by this model.

| Address | Reached effect |
| --- | --- |
| `+07C7..+07D0` | Copy saved words `[BP-5]` and `[BP-7]` to `[BP-D]` and `[BP-F]`. |
| `+07D3..+07DC` | Zero loop byte `[BP-10]`, AX, word `[BP-12]`, and byte `[BP-17]`. |
| `+07E0..+07E6` | Compare the just-zeroed loop byte with 4; unsigned `JB` at `+07E4` reaches `+07E9`, so the `+07E6` jump to `+08E8` is not reached from this entry. |

## Same-run native gate

Private run `slot-a-u3bd-loop-reset-07c7-p5-022` loaded saved slot A and
stopped at both boundaries. Its before and after 640 KiB RAM SHA-256 values
are `68c6e52f51227c3cf75f347a076f84ddc81417ac91f0e8800fd7e1c687696bf7`
and `a577d7c11da8a602e9ba5cdf722f5c50a68bdde00b61ba6fb985d21c2c2ce104`.
The 13 CPU fields plus IP were:

| Boundary | AX BX CX DX | SI DI BP SP | DS ES SS CS | FLAGS IP |
| --- | --- | --- | --- | --- |
| `+07C7` | `0008 0000 0004 000D` | `3E68 0004 3EB6 3E9E` | `14A0 3DA1 1D16 2D14` | `7246 07C7` |
| `+07E9` | `0000 0000 0004 000D` | `3E68 0004 3EB6 3E9E` | `14A0 3DA1 1D16 2D14` | `7297 07E9` |

Only physical RAM addresses `0x21004..0x21007` differ between the two
captures. Other reached writes restore values already present in this frame.
The C model matches the full 640 KiB after image, all 13 fields, and IP from
the captured before image. This establishes this frame's reached path; it
does not establish how later entry-5 iterations or the full frame complete.

## Reproduction

Keep original game media and captures under ignored `resources/`.

```sh
PYTHONPATH=/tmp/poolrad-capstone \
  python3 tools/replay/u3bd_loop_reset_07c7_audit.py
cmake -S . -B resources/build-u3bd-loop-reset-07c7 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Werror -pedantic'
cmake --build resources/build-u3bd-loop-reset-07c7 \
  --target poolrad_overlay_u3bd_loop_reset_07c7_tests
ctest --test-dir resources/build-u3bd-loop-reset-07c7 \
  -R '^poolrad_overlay_u3bd_loop_reset_07c7_tests$' --output-on-failure
```

The synthetic test covers coordinate restoration, loop reset and unsigned
branch flags, real-mode offset wrapping, and invalid host inputs. The audit
also checks the original span hash and the same-run native-to-C state delta.
