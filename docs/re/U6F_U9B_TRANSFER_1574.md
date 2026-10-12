# Unit 0x6F +0x1574 transfers to unit 0x9B +0x0000

The far call at unit `0x6F:+0x1574` uses the overlay stub
`0x00AC:0x0025`. Its descriptor word is **`+0x0000`**. The adjacent
`0x00AC:0x002A` stub has descriptor `+0x01AF`. Earlier party-insertion
notes and bounded C models equated `0x0025` with the independently
translated `0x9B:+0x01AF`; that direct-target claim was wrong. The
`+0x01AF` leaf can be reached *inside* `+0x0000`, subject to a branch.

Before this audit, `deja "Poolrad u6F 1574 00AC 0025 overlay thunk u9B 01AF stack"`
found no exact session, while `deja "u9b 01af"` recovered the [PR #194
leaf translation](U9B_RECORD_INCR_01AF.md). This check reused the [PR #267
pre-transfer frame](U6F_PARTY_INSERT_1446_SS.md) and the [resident loader
map](RESIDENT_LOADER_MAP.md), then independently checked the ignored DOS
binaries. None of those prior results establishes a direct call from
`+0x1574` to `+0x01AF`.

| Source | Bounded result |
| --- | --- |
| START.EXE | 65,231 bytes, SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes, SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| u6F `+0x1446..+0x157E` | 313 bytes, GAME file `0x1F42C..0x1F564` inclusive, SHA-256 `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` |
| Stub `0x00AC:0x0025` | 5 bytes, expanded START image `0x0AE5..0x0AE9`, SHA-256 `d67da1e87a1a5cac157296d1e710a6673b63b369f977f13bc9426973402e8103`; `INT 3Fh`, descriptor `+0x0000` |
| Stub `0x00AC:0x002A` | 5 bytes, expanded START image `0x0AEA..0x0AEE`, SHA-256 `26e4a8aad0d6405d9d752df2d3725f4834cab36f96a1e47c81d7e08236276b3d`; `INT 3Fh`, descriptor `+0x01AF` |
| u9B `+0x0000..+0x01AE` | 431 bytes, GAME file `0x2AA87..0x2AC35`, 159 instructions, SHA-256 `260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71` |
| Resident INT 3Fh handler `0xAA2:0x02E6..0x0337` | 82 bytes, expanded START image `0xAD06..0xAD57`, START file begins `0xA9FC`, SHA-256 `6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c` |
| Handler load subpath `0xAA2:0x0338..0x03E8` | 177 bytes, SHA-256 `58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991` |

The u9B entry table is `[+0x045E, +0x0000, +0x01AF, +0x0255,
+0x0329]` at stub offsets `0x20, 0x25, 0x2A, 0x2F, 0x34`. The
hash-locked u6F instruction is `LCALL 0x00AC:0x0025`, so the descriptor
and callsite agree on `+0x0000`.

## Stack and dispatch boundary

On the first saved insertion, [PR #267's pre-transfer state](U6F_PARTY_INSERT_1446_SS.md)
has `SS=1D16`, callee `BP=37AE`, and `SP=379E`. The `+0x156E/+0x1571`
argument pushes leave segment at `SS:379C` and offset at `SS:379A`.
`+0x1574` then puts caller CS `2A8E` at `SS:3798`, return IP `1579`
at `SS:3796`, and enters the stub with `SP=3796`. Executing `INT 3Fh`
puts flags at `SS:3794`, stub CS `00AC` at `SS:3792`, and saved IP
`0027` at `SS:3790`. These are transient RAM writes even if the stack
later unwinds.

The installed `0xAA2:0x02E6` handler saves registers, reads the word
at stub `0x0027` through `LES BX,[BP+2]` and `PUSH ES:[BX]` at
`+0x02F7/+0x02FA`, and rewinds saved IP to `0025` at `+0x02FD`.
`+0x0308..+0x031D` temporarily exchanges its saved BP/flags words
around the loader call `+0x0338`; `+0x0320` pops the descriptor into BX.
The path then reaches indirect far call `+0x032A` through `DS:0x43F0`
before register restoration and `IRET` at `+0x0337`. The pointer is
zero in the static expanded START image and is runtime initialized.
The `+0x0338` subpath checks whether the overlay is loaded and can
load or patch it. Its reached branch and the indirect target need a
runtime capture. A direct far jump into `+0x0000` after `IRET` is the
intended dispatcher route, not a proved complete saved-run RAM/register
trace.

At a normally dispatched `+0x0000` entry, the original argument is
still at `[BP+6/+8]` after `PUSH BP; MOV BP,SP; SUB SP,8` (new
`BP=3794`, `SP=378C` for this saved frame). Its first record write at
`+0x0009` clears byte `+0x2D`. The body then loops over eight index
values, reading record bytes `+0x96..+0x9D` and DS tables. It can
write record `+0x73`, `+0xA1`, `+0xB2..+0xB7`, and selected `+0x32`
bytes. Its `PUSH CS; CALL +0x01AF` at `+0x00EB/+0x00EC` is conditional:
on index zero, the signed `record[+0x96] > 1` branch must be taken.
The body also calls `+0x0255` at `+0x0190/+0x0191` after its loop and
calls `+0x0329` at `+0x01A5/+0x01A6` only when signed
`record[+0x9C] > 0`. It ends in `RETF 4` at `+0x01AC`. The saved record
values at those gates were not captured at this entry, so this audit
does not claim the `+0x01AF` leaf ran on the saved insertion.

The merged `pr_u9b_record_incr` remains a complete translation of its
*own* 166-byte leaf. Calls to it in earlier party-build and insertion
C models are provisional stand-ins for the newly identified
`+0x0000` body. Their pre-transfer list, slot, and window effects
remain separately bounded; their post-transfer record and whole-RAM
parity is unproved. A full C continuation needs the runtime handler
state, the `+0x0000` body's DS tables and all three nested call paths,
and a capture of the saved branch inputs. No strict or identity count
is added here.

Reproduce locally with ignored original files and Capstone 5:

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 tools/overlay/u6f_u9b_transfer_audit.py
```

The audit prints only addresses, counts, and hashes. No original
executable bytes, save data, RAM captures, or media are committed.
