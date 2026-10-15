# RES_0709:0768 printer helper first-copy gate

This bounded C transition executes `RES_0709:0768` through its first
external call, `0AF8:0B25`, and stops before the callee body. Entry frames
are synthetic; overlay call operands are pinned but their caller paths are
not executed here. No strict full-C or game-function credit is claimed.

## Pinned source and entry frame

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `resources/DOS/POOLRAD/START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `resources/DOS/POOLRAD/GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

The expanded prefix `[0x077F8,0x07813)` corresponds to
`0709:0768..0783`; its 27 bytes have SHA-256
`bd3a9fabc69274f2866c5d9ee448b805191310bf7fe81c9d693935d6c8b31765`.
Capstone finds no local branch before `077E CALL FAR 0AF8:0B25`. The
relocation-backed inventory identifies **27** `GAME.OVR` far call operands
to `0709:0768`; each five-byte operand has SHA-256
`505ad54ab9e5390450c330a9decab8bab5ee9f07d44dc475f2414fa8afc6f9ea`.
Two calls are the U1C PRINT/PRINTCLEAR arms at `1C:+0A69/+0A91`, file
offsets `0x17AF/0x17D7`. Their documented low-byte push order is
`1,11,26,16,0A,0-or-1`, followed by source `DS:70C8`.

The gate saves BP, allocates `0x126` local bytes, loads the source far
pointer from `[BP+6]`, and pushes source segment/offset, destination
`SS:BP-0x100`, and maximum length `0x00FF`. At `0AF8:0B25`, the stack
starts with return `0709:0783`, then maximum length, destination far
pointer, and source far pointer. The C transition reproduces 16-bit
`SUB SP,0x126` flags and real-mode address wrap.

The loader ledger's `0x07831` marker is **not** a proven function end:
the later `079B` guard jumps to `09DD`. This gate makes no claim about the
rest of `0768` or its return.

## Differential result

`tests/recovered/check_resident_0709_0768_copy_gate.py` checks the original
prefix in Unicorn against the C transition at the first copy-call entry.
It uses 27 synthetic entry frames varied across the pinned static caller
identities, including both U1C-style arm values, high-byte variation, and
source offset wrap. Three further stack seeds exercise carry, zero, and
overflow from `SUB SP,0x126`. **30/30 cases pass**, comparing all 14 CPU
fields (architecturally meaningful flags), every byte of 1 MiB RAM, and
the seven-word copy-entry stack. The C compiles with `-std=c11 -Wall
-Wextra -Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_0768_copy_gate.py
```

These synthetic frames establish local prefix parity, not parity of all
27 overlay caller setups or reachability. The runtime copy, later control
branches, printer effects, and return remain unresolved. The separate
`0709:0626` post-copy first-control branch is queued as another printer
follow-on once runtime copy effects can be bounded.

Before implementation, bounded `deja "RES_0709:0768 printer first runtime
copy gate original full state"` timed out. No past-session assertion was
reused. The merged U1C PRINT note supplied frame leads; the source hash,
call identities, instruction boundary, and full state were independently
checked against the ignored original inputs. No media or RAM is published.
