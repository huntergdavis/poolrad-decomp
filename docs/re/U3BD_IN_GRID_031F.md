# U3BD internal +031F: complete grid bounds leaf, parity gate pending

`src/recovered/overlay_u3bd_in_grid_031f.c` is a standalone C model of the
complete DOS v1.3 `GAME.OVR` original `OVL_03BD_031F`. It is distinct from
the static in-grid copies inside the existing entry 2, 3, and 4 models and
from the bounded entry-5 caller model. **No new identity or strict full-C
count is claimed here.** PM review must promote this unique original after
the original-byte full-state gate.

Before source work, `deja "Poolrad U3BD +031F leaf saved frame strict C closure"`
was run with an eight-second bound and timed out without reusable output.
The existing [entry-2](U3BD_NIBBLE_ENTRY2.md),
[entry-3](U3BD_FIELD_ENTRY3.md), and [entry-4](U3BD_CELL_ENTRY4.md) analyses
supplied the signed grid rule and the near-call ABI. The
[saved entry-5 leaf call](SLOT_A_U3BD_LEAF_CALL_07F9.md) supplied a reached
native frame. The original bytes and saved loaded bytes were checked again
for this model.

## Original and loaded boundaries

The ignored `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x3BD` begins at file `0x2E8F7`. The body is
`+031F..+0350` exclusive, file `[0x2EC16,0x2EC47)`: **49 bytes, 18 aligned
8086 instructions**, SHA-256
`ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b`.
The next function begins at `+0350`. There are no call sites, loops,
indirect branches, global-memory accesses, or other exits. The sole exit is
`+034D RETF 4`.

The private saved-slot-A capture under ignored
`resources/oracle-runs/slot-a-u3bd-entry5-call-chain-p5-027` loaded this
body at `2D14:031F`. Its 49 loaded bytes have the same SHA-256 as the
original source; this body has no relocation differences. The three saved
640 KiB RAM captures have SHA-256 values `0bf854b0b7f7324779cb6905f7ef70ce39d22e2efa39a111f0c748637bd74ad5`
(before caller), `e76369ece75277656911157e71f8789566f5ed29f0d598c50fe3bc3efb236ef8`
(leaf entry), and `94656b315a93486dff325d530db006586b3e89b1c2fbc3b2d0903dc2c7d76e9e`
(before caller branch). They were rehashed read-only in the p5 evidence clone.

## Callers and ABI

All five aligned near calls are preceded by `PUSH CS`, creating a far-call
frame for this `RETF 4` leaf:

| Call instruction | Owning original | Context |
| --- | --- | --- |
| `+035F` | entry 3, `+0350` | field lookup |
| `+048A` | entry 2, `+047B` | nibble lookup |
| `+0589` | entry 4, `+057A` | cell lookup |
| `+06E4` | entry 5, `+05FD` | first wall path |
| `+0805` | entry 5, `+05FD` | saved slot-A leaf entry at `2D14:031F` |

The first three callers are independently verified game entries; entry 5
has saved-game route evidence. This caller chain and the 16-by-16 tile
coordinate rule support game ownership, but the queue currently classifies
the leaf as a provisional helper. PM promotion remains required.

At entry, `SS:SP` holds return IP, `[SP+2]` return CS, `[SP+4]` the
second argument, and `[SP+6]` the first argument. `PUSH BP` and
`MOV BP,SP` place the first argument's low byte at `[BP+8]` (`x`) and
the second at `[BP+6]` (`y`); the high argument bytes are ignored.
`SUB SP,2` reserves a local byte at `[BP-1]`. The leaf returns `AL=1`
exactly when both signed bytes are in `0..15`, otherwise `AL=0`. AH and
all other general and segment registers except returned CS persist. It
restores BP and returns to the stack IP:CS with SP advanced eight bytes
from entry. `PUSH BP` leaves its two word bytes in RAM, and the local
result byte remains at `[BP-1]`; no other RAM bytes are written.

The four signed comparisons execute in order: `x>15`, `x<0`, `y>15`,
`y<0`. Each failure jumps to the zero store; success stores one. The
last executed `CMP` determines CF, PF, AF, ZF, SF, and OF on return.
Every path executes at least one `CMP`, so `SUB SP,2` flags never reach
the return. Other flag bits persist. The C model retains the order and
re-reads RAM for each original memory operand. Segment offsets wrap to
16 bits before the physical address is formed; multi-byte word accesses
are physically contiguous with A20 disabled.

The saved native frame has x=13, y=4. From leaf entry to the post-call
`+080A` stop, the only changed RAM bytes are physical `0x20FF3`
(local byte `0x3E -> 0x01`) and `0x20FF4..0x20FF5` (pushed BP
`0x07F9 -> 0x3EB6`). The caller's `OR AL,AL` at `+0808` changes flags
but no RAM. The saved CPU values are manual debugger transcriptions,
without a retained machine-readable CPU transcript; this capture is one
in-range path, not a branch-complete leaf return oracle.

## Focused gate and remaining evidence

`tests/recovered/test_overlay_u3bd_in_grid_031f.c` checks every rejection
gate, accepted boundaries, fixed expected defined FLAGS, retained AH and
unrelated registers, all 1 MiB RAM against the three expected writes,
RETF4 cleanup, SP/operand offset wrap, A20 wrap, and invalid host inputs.
Build with strict C11 warnings and run only the focused test target while
the host load hold remains in force:

```sh
cmake -S . -B resources/build-u3bd-031f -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Werror -pedantic'
cmake --build resources/build-u3bd-031f --target poolrad_overlay_u3bd_in_grid_031f_tests
ctest --test-dir resources/build-u3bd-031f -R '^poolrad_overlay_u3bd_in_grid_031f_tests$' --output-on-failure
```

The focused saved comparison is `tools/replay/u3bd_in_grid_031f_saved_parity.py`.
It accepts `--original` and `--capture-dir` paths so the private inputs can
stay in their own ignored resources directories. It hash-locks the original,
all three captures, and the loaded body, compiles only this C file in a
temporary directory, then runs the C leaf from the saved `+031F` CPU/RAM
frame. The C leaf's 640 KiB RAM exactly equals the native `+080A` image;
the intervening caller `OR AL,AL` does not write RAM. The C leaf's return
tuple matches the manually transcribed post-OR CPU tuple for this one
input; the OR happens to leave the defined flags equal on this path.
That CPU evidence is not machine-readable. The comparison passed with the
original media in this clone and the private p5 capture directory.

The earlier `tools/replay/u3bd_leaf_call_07f9_audit.py` documents the
caller comparison. Its former `/tmp/poolrad-capstone` dependency is not
installed in this clone. A separate original-byte CPU/RAM comparison
across all five branch outcomes and an isolated native leaf return frame
remain open. No new native or heavy oracle run is permitted during the
current host PSI hold. The PM controls any identity and strict-C count
change after reviewing that evidence.
