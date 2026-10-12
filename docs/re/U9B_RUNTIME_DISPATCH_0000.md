# Saved u6F insertion reaches u9B entry zero through INT 3Fh

The first saved slot-A insertion reaches unit `0x9B:+0x0000` through
the resident INT `3Fh` handler. This native trace resolves the
runtime dispatch left open by [PR #271's static transfer audit](U6F_U9B_TRANSFER_1574.md).
The separate `+0x01AF` leaf is **not reached** on this insertion.

Before debugging, `deja "Poolrad saved runtime INT3F u9B 0000 DS43F0 dispatch native capture"`
timed out during index refresh; the same MCP recall query returned no
matching session. This work reused [PR #271's hash-locked callsite and
entry map](U6F_U9B_TRANSFER_1574.md), [PR #267's pre-transfer SS
frame](U6F_PARTY_INSERT_1446_SS.md), and the
[resident loader map](RESIDENT_LOADER_MAP.md). The ignored DOS images
and native RAM milestones were checked independently.

| Source or span | Check |
| --- | --- |
| START.EXE | SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| u9B `+0x0000..+0x01AE`, GAME file `0x2AA87..0x2AC35` | 431 bytes, 159 instructions, SHA-256 `260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71`; loaded code has the same hash |
| Resident `0xAA2:0x02E6..0x0337`, START image `0xAD06..0xAD57` | 82 bytes, SHA-256 `6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c` |
| Resident `0xAA2:0x0338..0x03E8` loader subpath | 177 bytes, SHA-256 `58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991` |

## Reached dispatch

The native first insertion stopped at caller `2A8E:1574`, whose far
call reached stub `08D0:0025`. The INT `3Fh` vector entered resident
`12C6:02E6` (static `0xAA2:0x02E6`, shifted by `0x824` paragraphs).
The handler read the stub descriptor through `+0x02F7/+0x02FA`,
rewound its saved IP to `0025` at `+0x02FD`, and called the loader at
`+0x0314 -> +0x0338`. The `+0x0314` snapshot is **before** that call.

At handler `12C6:032A`, `DS=14A0` and `DS:43F0` held
`12C6:02E5`. That target is a `RETF` immediately preceding the INT
handler; the indirect far call returns directly to `+0x032E`.
The loader had patched stub `08D0:0025` into a far jump to
`2CCD:0000`. After register restoration and `IRET` at `+0x0337`,
the rewound stub IP executed that jump. The native debugger then
stopped at `2CCD:0000`; the loaded 431-byte body matched the ignored
GAME.OVR span hash above.

| Native stop | Stack and route evidence |
| --- | --- |
| `2A8E:1574` before call | `SS=1D16`, `SP=379A`, `BP=37AE`; far argument `449A:0008` at `SS:379C/379A` |
| `12C6:02E6` handler entry | `SP=3790`; saved stub IP/CS at `SS:3790/3792` are `0027/08D0`, original return IP/CS at `SS:3796/3798` are `1579/2A8E` |
| `12C6:0314` before loader | Saved stub IP at `SS:3790` is `0025` |
| `12C6:032A` after loader | `DS:43F0 -> 12C6:02E5`; stub `08D0:0025` jumps to `2CCD:0000` |
| `2CCD:0000` entry | `SP=3796`; original far argument and return words remain at `SS:379A..379C` and `SS:3796..3798` |
| `2CCD:0009` first record store | `BP=3794`, `SP=378C`, `ES:DI=449A:0008` |
| `2CCD:01AC` return instruction | `SP=3796`; `RETF 4` then reaches caller `2A8E:1579` with `SP=379E` |

## Reached record paths

At `+0x0009`, the first store clears record byte `+0x2D` from
`0x28` to zero. The eight-index loop subsequently recomputes it to
`0x28` before the debugger stops at `+0x0191`. The saved record's
index-zero `+0x96` is zero. The `+0x00A7..+0x00AD` signed test thus
skips the `+0x00EC -> +0x01AF` call; this field is not written by the
entry-zero body. The debugger reached `+0x0191 -> +0x0255`, then
`+0x01AC`. Record `+0x9C` is zero after `+0x0255`, so the signed
`+0x0197..+0x019D` test bypasses `+0x01A6 -> +0x0329`.

The captured first record has no net change over its first `0xC0`
bytes between `+0x0000` entry and `+0x01AC`, despite the reached
stores. Fourteen RAM bytes differ between those snapshots, all in
the transient `SS:3780..3795` stack region. These are observations
of this saved insertion, not a general no-op contract for u9B.

The existing `pr_u9b_record_incr` leaf remains a complete translation
of its isolated `+0x01AF` body. Its use as a stand-in for the direct
`+0x1574` transfer remains provisional: this saved insertion instead
enters the 431-byte `+0x0000` body and calls `+0x0255`. A full C
translation of that body must preserve the DS table reads, the
conditional `+0x01AF/+0x0329` paths, the reached `+0x0255` path,
and stack/RAM aliases across all supported inputs. This PR adds
runtime evidence only; strict full-C and identity counts do not change.

## Reproduction

Prepare the saved oracle using the ignored DOS resources, run the
heavy DOSBox-X debugger under a terminal, and restore slot A. Break
at `2A8E:1574`, `12C6:02E6`, `12C6:0314`, `12C6:032A`,
`2CCD:0000`, `2CCD:0009`, `2CCD:000E`, `2CCD:0191`,
`2CCD:01AC`, and `2A8E:1579`. `+0x00EC` and `+0x01A6`
breakpoints were armed but did not fire on this insertion. At each
listed milestone, use `EV` for registers and `MEMDUMPBIN` for a
private 640 KiB RAM capture. The ignored capture directory for this
run is `resources/oracle-runs/u9b-entry-zero-runtime-002`.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 tools/overlay/u9b_runtime_dispatch_audit.py \
  --native-dir resources/oracle-runs/u9b-entry-zero-runtime-002
```

The verifier rechecks the original image hashes and all nine private
snapshot hashes before testing the dispatch frame, patched jump,
record gate inputs, first store, and return. Without `--native-dir`,
it checks the original code spans only. No executable bytes, RAM
captures, save data, or media are committed.
