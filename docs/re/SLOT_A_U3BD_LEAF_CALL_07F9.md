# Saved slot-A U3BD entry-5 call through the local in-grid leaf

This bounded state slice starts at the reached return from U3BD entry 2,
loaded `2D14:+07F9`. It executes the entry-5 caller through its local
`+031F` in-grid leaf and `OR AL,AL`, then stops **before** the `+080A JNE`.
It contributes **zero** full-routine translations and **zero** new verified
identities. It does not model the rest of entry 5 or a complete art frame.

Before decoding, `deja "Poolrad U3BD entry5 +047B return +031F call +080A
branch slot A"` timed out after 30 seconds. Reused, and independently
checked against original bytes: the merged [entry-2 nibble lookup](U3BD_NIBBLE_ENTRY2.md)
and [entry-4 cell reader](U3BD_CELL_ENTRY4.md) establish the shared
`+031F` leaf's signed bounds rule. The merged [+07C7 loop reset](SLOT_A_U3BD_LOOP_RESET_07C7.md)
establishes the preceding saved-route entry-5 boundary. The merged
[first-call handoff](SLOT_A_U3BD_FIRST_CALL_07E9.md) from PR #392 establishes
the immediate `+07E9..+07F9` transfer; this bounded model begins after it.

## Original-byte boundaries

The ignored DOS `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Both spans match loaded memory in the saved slot-A before image.

| Span | File range | Size and decode | SHA-256 |
| --- | --- | --- | --- |
| Entry-5 caller, `+07F9..+080A` | `[0x2F0F0,0x2F101)` | 17 bytes, 8 instructions | `f432c36103980b020edc3c7322183327367001f5b1c399631e3fe2d6b08b95c4` |
| Local leaf, `+031F..+0350` | `[0x2EC16,0x2EC47)` | 49 bytes, 18 instructions | `ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b` |

At `+07F9`, the caller writes entry-2 result `AL` to `SS:[BP-14]`.
It pushes bytes `SS:[BP-0D]` and `SS:[BP-0F]` as full `AX` words, then
`CS` and return IP `+0808`, and calls `+031F` at `+0805`. The leaf reads
the low bytes as signed x and y, stores 1 when both lie in `0..15` and 0
otherwise, and returns with `RETF 4`. `+0808 OR AL,AL` sets the defined
status flags. The model stops at `+080A`; no JNE instruction is executed.

## Same-run native frame

Private run `slot-a-u3bd-entry5-call-chain-p5-027` was prepared with
`tools/oracle/prepare_run.sh`, loaded saved slot A under an isolated
debug-capable DOSBox-X (binary SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`),
and captured 640 KiB of RAM at three successive stops. The original
slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
All media, RAM and screenshots remain ignored under `resources/`.

| Boundary | RAM SHA-256 | `AX`, `DI`, `BP`, `SP`, `FLAGS` |
| --- | --- | --- |
| Return `+07F9` | `0bf854b0b7f7324779cb6905f7ef70ce39d22e2efa39a111f0c748637bd74ad5` | `000E 004D 3EB6 3E9E 7202` |
| Leaf entry `+031F` | `e76369ece75277656911157e71f8789566f5ed29f0d598c50fe3bc3efb236ef8` | `0004 004D 3EB6 3E96 7202` |
| Pre-branch `+080A` | `94656b315a93486dff325d530db006586b3e89b1c2fbc3b2d0903dc2c7d76e9e` | `0001 004D 3EB6 3E9E 7202` |

At every stop, the other fields are `BX=0 CX=4 DX=000D SI=3E68
DS=14A0 ES=3DA1 SS=1D16 CS=2D14`. The reached leaf-entry stack holds
x=`000D`, y=`0004`, `CS=2D14`, and return IP=`0808`. The caller changes
five physical bytes at `0x20FF6..0x20FF9` and `0x21002`; the leaf changes
three at `0x20FF3..0x20FF5`. Same-value writes are absent from these
before/after deltas. At `+080A`, ZF is clear, so the pending `JNE +0824`
would be taken for this saved input. Its execution is outside this slice.

`tools/replay/u3bd_leaf_call_07f9_audit.py` hash-locks the original and
loaded spans and all three private RAM captures. It checks the reached
leaf frame, then runs the C model from the first capture and compares all
640 KiB RAM, 13 CPU fields, and IP against the pre-branch capture. The
saved-route comparison is exact. The `OR` instruction leaves AF undefined
on x86; the C model retains the host-provided AF bit for other synthetic
inputs and tests only defined flags there.

## Reproduce

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 tools/replay/u3bd_leaf_call_07f9_audit.py
cmake -S . -B resources/build-u3bd-leaf-call-07f9 -DCMAKE_C_FLAGS='-Wall -Wextra -Werror -pedantic'
cmake --build resources/build-u3bd-leaf-call-07f9 --target poolrad_overlay_u3bd_leaf_call_07f9_tests
ctest --test-dir resources/build-u3bd-leaf-call-07f9 -R '^poolrad_overlay_u3bd_leaf_call_07f9_tests$' --output-on-failure
```

The synthetic test covers in-range and all four signed-range failures,
preserved `AH`, retained stack writes, 16-bit stack offset wrap, and
invalid host inputs. The next boundary is the pending `+080A JNE`; this
model returns IP `+080A` without choosing its successor.
