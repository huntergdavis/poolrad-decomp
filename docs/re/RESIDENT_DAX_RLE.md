# Resident DAX RLE decompressor at `0x0802:0x050F`

This is the independently decoded DOS v1.3 routine for
[issue #82](https://github.com/huntergdavis/poolrad-decomp/issues/82).
`pr_rec_res_0802_050f` is a **path-complete bounded model**, not a strict full
translation. Its terminating instruction paths and both called runtime byte
operations are represented; host memory faults, an iteration bound, and a
status for a DOS nonterminating path make the public API deliberately different
from the original. The routine's game versus linked-library origin is also
unresolved, so it adds neither a full game-C translation nor a verified
game-identity count.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Entry, provenance, callers, and frame

The hash-locked `resident_loader_map.unpack` places the entry at expanded
image `0x0852F`, or `0x0802:0x050F` using the reader's code-segment base
`0x08020`. Its `0x0852F..0x085EA` body is **188 contiguous copied bytes**
from packed file `0x08566..0x08621`, without fill provenance. Capstone 5.0.7
decodes 83 instructions, seven direct branches whose targets are instruction
starts inside the body, two far calls, and one exit: `0x085E8 RETF 0x10`.
The next prologue is the separately owned file-open helper at image `0x085ED`.

The [resident callable ledger](resident_callable_ledger.csv) lists one decoded
near caller and no relocated far caller. The caller is the
[reviewed block reader](RESIDENT_ECL_DAX_BLOCK.md) at `0x08954 PUSH CS;
0x08955 CALL 0x0852F`, after its file-close call at `0x0893D` and before
freeing the packed temporary at `0x08961`. The explicit `PUSH CS` gives the
near call a far return frame. The caller passes four far addresses; the
callee pops 16 argument bytes:

| Callee frame | Passed variable | Use |
| --- | --- | --- |
| `[BP+0x12]` | far address of packed-buffer far-pointer variable | Reloaded before every control and again inside either copy path. |
| `[BP+0x0E]` | far address of output-buffer far-pointer variable | Reloaded before each write run. |
| `[BP+0x0A]` | far address of packed-size word | Reloaded after each iteration for an unsigned termination comparison. |
| `[BP+0x06]` | far address of output-size word | Passed and popped, but never read by this body. |

There is no output-capacity check. The two stack-local word indices at
`[BP-2]` and `[BP-4]` start at zero and wrap modulo 65536; `[BP-5]` holds
the current control or positive repeat count. Far pointer offset additions
also wrap at 16 bits. The C API reads the pointed-to variables through passive
memory callbacks each time the DOS instructions reload them, so overlapping
buffers and changed pointer variables can be represented.

## Exact branch and runtime effects

| Loaded address | Observed operation |
| --- | --- |
| `0x0853F..0x0854D` | Dereference the packed-pointer variable, add the input index to its offset, read the control byte. This happens **before any packed-size check**. |
| `0x08550..0x08592` | Signed comparisons select `0x00..0x7E`. Call runtime `0xAF8:0x1685` Move with `control+1` bytes from input index + 1 to output index. Then add `control+2` to the input index and `control+1` to the output index. |
| `0x08594..0x085D4` | Signed comparisons select `0x81..0xFF`. Negate the signed byte to count `1..127`; load the following input byte; call runtime `0xAF8:0x16A8` FillChar at the output index. Then add two to the input index and count to the output index. |
| `0x085D7..0x085E8` | Reload packed-size word; return if input index is **unsigned greater than or equal** to it, otherwise read another control. |

Both runtime callees were decoded at their loaded physical addresses, not
inferred from names. Move at image `0x0C605..0x0C627` is 35 bytes and 16
instructions. It clears DF, compares **SI and DI offsets only**, and copies
backward with `STD; REP MOVSB` when `SI < DI`; otherwise it copies forward.
It restores DS and returns with ten argument bytes popped. This distinction
matters when different segment:offset pointers alias the same physical memory.
FillChar at `0x0C628..0x0C63B` is 20 bytes and seven instructions: it clears
DF, writes AL forward with `REP STOSB`, and pops eight argument bytes. The
model reports the resulting DF value in its state. Both runtime spans also
have copied-byte loader provenance.

Controls `0x7F` and `0x80` enter neither copy path, leaving both indices and
DF unchanged. They re-read the packed-size word. **If the unchanged input
index is below that word, the DOS routine loops on the same control; if the
word is zero at entry, it can return after reading one control.** The model
returns `STALLED` for the former case, rather than hanging, and implements
the latter size-zero behavior. This narrows the unconditional stall wording
in the earlier [block-reader note](RESIDENT_ECL_DAX_BLOCK.md), which was a
lead rather than this independent body trace. Other malformed input can make
the original read or write outside the intended buffers; the C model returns
a host memory-fault status only if the supplied memory callback fails. It
does not claim a DOS status code. An iteration limit bounds cyclic or mutable
input without inventing DOS termination.

## Synthetic and private verification

Synthetic tests cover literal controls `0x00` and `0x7E`, repeat controls
`0x81` and `0xFF`, mixed runs, first-control execution with zero size,
`0x7F/0x80` with zero and nonzero size, both Move directions, same-segment
overlap, far aliasing where offset order differs from physical order, offset
wrap on source and destination, 16-bit output-index wrap, writes that change
the source-pointer variable or packed-size word before the next reload, DF,
iteration bound, callback fault, and argument validation. No game bytes are
embedded in tests.

A read-only local audit uses the existing [portable DAX reader](../../src/formats/dax.c)
as a comparison oracle for `ECL1.DAX` through `ECL8.DAX`. On the owner's
ignored files it reports **8 archives, 29 blocks, 29 exact byte matches, and
176,265 decoded bytes**. The ordered-file digest is
`979544f439388e7a353f6ae1729fbd5a5973ceb4308a8920d69de7bd005f871e`
(SHA-256 of the eight individual file SHA-256 digests concatenated in numeric
order). No bytes, text, or media from those files are printed or committed.

```sh
cmake -S . -B /tmp/poolrad-issue-82-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-82-build
ctest --test-dir /tmp/poolrad-issue-82-build --output-on-failure
/tmp/poolrad-issue-82-build/poolrad_resident_dax_rle_audit \
  resources/DOS/POOLRAD/ECL{1,2,3,4,5,6,7,8}.DAX
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR --near \
  --json /tmp/poolrad-issue-82-loader.json
```

`deja "poolrad resident DAX RLE decompressor 0x0852F issue 82"` ran before
implementation and found the assignment, not a prior full decode. The
[issue #74 reader note](RESIDENT_ECL_DAX_BLOCK.md) supplied the control-range
lead, the merged loader map supplied hash-locked expansion, and the portable
DAX reader supplied private corpus comparison. This work independently
verified the segment:offset, complete body and callee instructions, frame,
loop order, and alias-sensitive runtime effects. The adjacent `0x085ED`
file-open helper remains outside this branch and with the other resident
worker.
