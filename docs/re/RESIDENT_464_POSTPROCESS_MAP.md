# RES_0464:02A5 block postprocessor map

This is an **evidence-only control-flow map**, not a C translation or proof
of live LOAD MONSTER execution. The merged
[block wrapper](RESIDENT_464_BLOCK_WRAPPER.md) calls this procedure after
copying the 17-byte block header into a constructed object. The bitmap
[constructor](RESIDENT_464_BITMAP_CONSTRUCTOR.md) and
[release](RESIDENT_464_BITMAP_RELEASE.md) notes describe separate routines.
No strict full-C or game-identity credit is proposed.

## Provenance and extent

The required deja searches `Poolrad RES_0464:02A5 bitmap postprocessor
control flow return extent` and `0464:02A5` found no matching prior session.
The merged wrapper and resident ledgers supplied the entry and caller lead;
all bounds below were independently checked against ignored `START.EXE`,
SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Procedure | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0464:02A5` | `0x048E5..0x04F14` | `0x0491E..0x04F4D` | 1,583 | `1802227461346a3e8a0b9595513d31f820eb4c134903699706f557f5a4f92610` |

The audit checks contiguous unpacker source provenance, then recursively
decodes each branch successor from `0x048E5`. Its **604 nonoverlapping
instruction starts cover all 1,583 bytes**. The 84 branch sites stay inside
that extent. There is one return, `RETF 0x0E` at `+0x08D1`, and the next
prologue starts at `+0x08D4`. Thus the complete reachable body ends at
`0x04F14`; this end was established from the control-flow graph and return,
not inferred solely from the next ledger prologue. There are nine direct
calls: runtime `Move` (`0x0AF8:1685`) at three sites, near
`RES_0464:0000` at two, and near `RES_0464:007B` at four. These near
targets are distinct procedures.

## Caller frame and route gates

The wrapper calls at its `+0x0286` site. The far frame reads destination
object pointer `[BP+6]`, raw selector byte `[BP+0x0A]`, mutable source
offset WORD `[BP+0x0E]`, and far source-block pointer `[BP+0x10]`.
The wrapper initializes that offset to `0x11`. The pushed byte at
`[BP+0x0C]` is not referenced in this procedure; it must remain a raw,
uninterpreted argument in any later model. `RETF 0x0E` consumes the frame.

| Unit offset | Gate or reached behavior |
| --- | --- |
| `+0x02AB..+0x02B6` | A zero destination far pointer jumps directly to the common return. No video byte is read. |
| `+0x02C9..+0x02D3` | Read DS byte `[0x4A1E]` once. Zero selects route 0. |
| `+0x05EF..+0x05F6` | Value 1 selects route 1. |
| `+0x07AC..+0x07B3` | Value 2 selects route 2; every other value returns without processing. |
| `+0x02D3..+0x05EC` | Route 0 loops over object header dimensions and count, reads the source block, calls both near helpers and runtime `Move`, and may update primary and auxiliary data. |
| `+0x05F6..+0x07A9` | Route 1 has its own dimension loops, source copy, helper reads, and object writes. |
| `+0x07B3..+0x08CE` | Route 2 loops over the object count, reads source bytes and the object stride at header `+0x11`, and writes object data. |

The modes above are **raw numeric video-state values**. Their display
meaning, live value on the saved route, far-memory validity, and runtime
`Move` effects are not inferred here. Object header fields `+0/+2/+8/+0x11`
bound loop lengths and strides. Auxiliary pointer fields `+0x13/+0x15`
are read conditionally. Routes 0/1 read DS tables at `+0x25E5`,
`+0x25F6`, and `+0x25FA`; the table contents are private.

Thirteen direct ES writes occur at
`+0x03AC,+0x0421,+0x0477,+0x04BE,+0x0521,+0x057A,+0x05C0,
+0x068C,+0x070D,+0x0776,+0x084E,+0x0871,+0x08A0`.
The four at `+0x03AC,+0x068C,+0x084E,+0x0871` address conditional
auxiliary data; the other nine address primary object data after header
`+0x17` with a computed index. The three runtime `Move` calls can also
write buffers. The source-offset frame WORD is incremented at
`+0x05C7` and `+0x0784` by four, and at `+0x08C0` by the object's
`+0x11` stride. These are separate route sites, not a fixed sequence on
one invocation. There are no direct DS writes in this procedure.

## Adjacent procedures and next slice

| Separate span | Loaded image, exclusive end | Bytes | SHA-256 | Boundary |
| --- | --- | ---: | --- | --- |
| `RES_0464:0000` | `0x04640..0x046BB` | 123 | `3e7b81eaa0e3e8109456fbdb6bd3ad59390b961eceb5f84fa2858055728e21a1` | Call-free bitmask helper; `RETF 6`. |
| `RES_0464:007B` | `0x046BB..0x0470F` | 84 | `d16625eb920138f29792a8d6587b923b03737a7d832585b9c0567492fbdce5d7` | Nibble-selection helper; one runtime `Move` call, `RETF 6`. |
| `RES_0464:00CF` | `0x0470F..0x0473F` | 48 | `1291e63d47f0d18e3c3708159546250c155647778751c8e8a2123f8bcc44afb3` | Independent runtime-facing procedure. |
| `RES_0464:00FF` | `0x0473F..0x0474C` | 13 | `cec981142e63bb53b622852e0f77455255fae44bc55ea80b0e47105e05e18ef6` | Independent procedure. |

Five private data bytes begin at `+0x010C`, before the wrapper prologue
`+0x0111`. They are not part of any helper body and are not published.
The `+0x0000` call-free helper is a separate bounded C candidate. Its
complete packed-byte contract and tests require a separate review; this
map does not expand the `+0x02A5` postprocessor into a speculative model.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_postprocess_map.py \
  --exe resources/DOS/POOLRAD/START.EXE
```

The script checks the owner-supplied executable hash, each reported span
hash, CFG coverage, calls, return, direct writes, video read, frame update
sites, and next prologue. It emits only offsets, counts, and hashes. No DOS
bytes, game media, or decoded private data enter the public repository.
