# RES_0464:0000 four-byte nibble bitmask

This is a complete portable C translation of the call-free
`RES_0464:0000` helper's return behavior. It is a separate procedure
called by the merged [postprocessor map](RESIDENT_464_POSTPROCESS_MAP.md)
at `+0x0358` and `+0x067B`. It does not translate the postprocessor or
prove live LOAD MONSTER execution. Strict full-C credit is proposed for
this helper only, subject to PM parity review; no identity count is changed
in this PR.

## Provenance and extent

Before implementation, deja queries `Poolrad RES_0464:0000 bitmap bitmask
helper` and `0464:0000` found no matching prior session. The merged
postprocessor map provided the helper boundary and caller lead. The
instructions, pushes, branches, and return below were independently decoded
from ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Procedure | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0464:0000` | `0x04640..0x046BB` | `0x04679..0x046F4` | 123 | `3e7b81eaa0e3e8109456fbdb6bd3ad59390b961eceb5f84fa2858055728e21a1` |

Capstone 5 decodes 47 complete instructions, ending in `RETF 6` at
`+0x0078`. The four branches at `+0x001F,+0x0033,+0x0054,+0x006A`
stay in the span. There are no calls or external-memory writes: all ten
memory write instructions address the local `[BP-negative]` frame. The
next independent helper has a prologue at `+0x007B`.

## Frame, branches, and result

Both callers push a selector WORD, then two packed WORDs, then `CS` before
the near `CALL`. The callee's far frame reads four bytes
`[BP+6]..[BP+9]` in little-endian order and the **low byte** of selector
WORD `[BP+0x0A]`. The upper selector byte is ignored. `RETF 6` removes
the two packed WORDs and selector WORD; AL carries the result.

| Offset | Decoded behavior | C translation |
| --- | --- | --- |
| `+0x0006..+0x001B` | Clear result, compute low byte of `selector << 4`, set bit to `0x80`, index to zero. | `high_selector` casts the shifted WORD to byte; result and bit start at the same values. |
| `+0x0024..+0x003B` | Read packed byte at current index. If its high nibble equals shifted selector, add current bit to result; then shift bit right once. | First comparison per byte. |
| `+0x0045..+0x0066` | Read the same packed byte. If its low nibble equals the **unmasked** selector byte, add current bit; then shift bit right once. | Second comparison per byte. |
| `+0x0066..+0x0078` | Continue until index 3 was processed, then return result through AL. | Exactly four iterations and eight distinct bit positions. |

The result's bits 7, 6, then 5, 4, and so on correspond to high then low
nibbles of successive packed bytes. Every matching bit is added once, so
there is no carry between positions. A selector above `0x0F` can still
match a high nibble after the 8-bit shift truncates, but cannot match a low
nibble, because the low comparison uses the raw selector byte. Tests cover
that boundary in addition to mixed hits, all hits, no hits, and low-only
hits. This raw-byte behavior is preserved rather than validating a
putative nibble range.

The first caller stores AL in a local before its auxiliary-data branch;
the second moves AL into DL for an auxiliary byte write. Neither reads
processor flags after the call. The helper has no allocator, I/O, runtime
call, global data access, or side effect that needs a host callback.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_bitmask_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

The read-only audit hash-locks the executable and helper span, contiguous
source mapping, decoded extent and branch sites, local-only writes, two
caller push sequences, and return. It emits only addresses, counts, and
hashes. Tests contain only synthetic packed bytes; no game media enters
Git.
