# Unit 0x6F first-slot continuation dependencies

This is a static map of both exits from the merged
[first-slot filename model](U6F_PARTY_FILENAME_BOUNDARY.md): `+0x19B1`
when selector AL is nonzero, and `+0x19EA` when AL is zero. It does not
choose an outcome for the saved slot-A run or execute the record loader.
pC's separate caller and pointer audit ends at `+0x19AF`; this note starts
at the branch targets. No C slice or function-count credit is claimed.

Before decoding, `deja "Poolrad u6f 19B1 19EA continuation dependencies
selector branch"` and `deja "u6f 19B1"` found no indexed match. Reused
[the party input model](SLOT_A_PARTY_INPUT.md),
[the string-boundary audit](SLOT_A_PARTY_STRING_BOUNDARY.md), and
[the resident selector audit](RESIDENT_DAX_SELECTOR_02EB.md) for the
upstream frame and known callee roles. All spans and call sites below were
independently decoded from the ignored, hash-verified DOS overlay. The
party input note's description of `+0x02AE` as 468 bytes is superseded by
the exact 129-byte prologue-to-return span below.

## Source spans

`GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The unit `0x6F` file base is `0x1DFE6`. Capstone 5 x86-16 decoding covers
every byte of the code spans. Endpoints are inclusive; the insertion
routine's following inline data is hashed separately and is not counted
as instructions.

| Span | GAME file span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| caller `+0x19B1..+0x19E9` | `0x1F997..0x1F9CF` | 57 / 22 | `76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5` |
| common loop gate `+0x19EA..+0x19F6` | `0x1F9D0..0x1F9DC` | 13 / 4 | `f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4` |
| post-loop pointer gate `+0x19F7..+0x1A12` | `0x1F9DD..0x1F9F8` | 28 / 8 | `bfac9c4b98fb5b3f87a5c17a87f7f14be9895def4a1333f1ced5a24d3234ae40` |
| post-insertion `+0x02AE..+0x032E` | `0x1E294..0x1E314` | 129 / 51 | `ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539` |
| record loader `+0x0AEC..+0x0EC3` | `0x1EAD2..0x1EEA9` | 984 / 370 | `715424407210de3824c3a0218cc29033f04bb357ec718aa24a7995d9452d1e36` |
| party insertion code `+0x1446..+0x157E` | `0x1F42C..0x1F564` | 313 / 99 | `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` |
| following inline data `+0x157F..+0x1601` | `0x1F565..0x1F5E7` | 131 / not code | `1dfa5f64639ccd4fec61babcf5419475c3c8317f0d3ba3b69f5ab0731dd192ee` |

The insertion code ends with `RETF 4` at `+0x157C`; all ten direct
branches in its 313-byte prefix stay inside that prefix. The combined
444-byte prologue-to-next-entry file interval has SHA-256
`375ed94f3c4e011a80bdfa7a761b7708f1c08a85de7cc683849de2a0e687fc03`.
Capstone 5.0.7's linear decode of that combined interval reports 161
starts because it interprets the inline data as instructions. GNU objdump
2.46 displays 147 starts by default because it suppresses zero runs in
the data; `--disassemble-zeroes` displays 161 starts at exactly the same
addresses as Capstone. Neither whole-interval count is the routine's
instruction count: its executable prefix has 99.

## Immediate caller flow

| Site | Instruction-level effect | Dependency or destination |
| --- | --- | --- |
| `+0x19B1..+0x19BE` | Push word `0x011D`; call SYSTEM `0xAF8:0x023F` at `+0x19B5`; save returned DX:AX far pointer at caller `[BP-0x1CB]`/`[BP-0x1CD]`. | Heap allocation. The runtime wrapper can stop on allocation error; no heap behavior is invented here. |
| `+0x19C2..+0x19D7` | Push the normalized basename local `[BP-0x1F6]`, allocated far pointer, and two words whose low bytes are `1`; near-call unit `+0x0AEC` at `+0x19D7`. | Record and sidecar load, `RETF 0x000C`. |
| `+0x19DA..+0x19E3` | Push the same allocated far pointer; near-call unit `+0x1446` at `+0x19E3`. | Party insertion, `RETF 4`. |
| `+0x19E6..+0x19E7` | Near-call unit `+0x02AE` with no arguments. | Conditional directory creation and message/retry path, `RETF`. |
| `+0x19EA..+0x19F4` | Compare current byte `[BP-0x1C9]` with copied count `[BP-0x212]`; equality enters `+0x19F7`, otherwise jump `+0x1937`, where the current byte increments before the next slot. | Common to selector AL=0 and a returning present-file path. The count came from `[BP-0x1F9]` at `+0x191E..+0x1922`; the first iteration starts at `1` and zero count skips directly to `+0x19F7`. |
| `+0x19F7..+0x1A10` | Read party-head far pointer `DS:0x5D96/0x5D98`; if both words are zero, jump `+0x1B7E`; otherwise enter `+0x1A13`. | A separate post-loop phase, outside this map. |

The nonzero-AL route can reach the common gate only after GetMem and all
three near callees return. The zero-AL route starts at that gate directly,
so it has no allocation, record load, insertion, or post-insertion call for
that slot. A successful selector AL does not prove any later host file or
heap effect completed.

## Direct callee edges and unresolved effects

The record loader `+0x0AEC` has 47 far-call sites, all decoded inside its
984-byte body: `0x0802:0x025D` at `+0x0C26/+0x0D0B/+0x0E09` (three file-open
calls), `0x0802:0x02EB` at `+0x0CAA/+0x0DAA` (two probes),
`0x0802:0x0425` at `+0x0B29` (one string transform), and
`0x0899:0x0104` at `+0x0E8D`. The remaining 40 calls are SYSTEM:
`0x023F` four, `0x0B0B` eight, `0x0B25` six, `0x0B98` ten,
`0x0C3B` two, `0x1491` three, `0x14FB` three, and `0x15C5` four.
These call sites show why a composed record-load translation needs real
file, record, heap and string effects; existing party-input C skips them.

The insertion callee `+0x1446` has three far calls: SYSTEM
`0xAF8:0x16A8` at `+0x14CA`, SYSTEM `0xAF8:0x0DC0` at `+0x14F2`,
and overlay thunk `0x00AC:0x0025` at `+0x1574`, already identified as
unit `0x9B:+0x01AF` in the [party input note](SLOT_A_PARTY_INPUT.md).
The same note maps its list append, slot byte `+0xBF`, and window-1 count
increment. This static caller map does not rerun those effects.

The no-argument `+0x02AE` body is **129 bytes**, ending at its `+0x032E`
far return. It copies a substring of live `DS:0x537A` at `+0x02CB`,
calls SYSTEM `0xAF8:0x17D6` at `+0x02D0`, then calls IOResult
`0xAF8:0x04A2` at `+0x02D5`. Result words `0` and `5` leave at
`+0x032B`; any other word builds a private message through SYSTEM
`0x0B0B/0x0C3B/0x0B98`, calls resident `0x0709:0x0BF5` at `+0x031A`,
and loops to `+0x02B5`. The message bytes and any actual DOS outcome
are not published or inferred.

`START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The hash-locked loader maps SYSTEM `0xAF8:0x17D6` to loaded image
`0x0C756`, first packed file offset `0x0C44C`. Its 19-byte
`0x0C756..0x0C768` span has SHA-256
`90731793b0ac2c364059b1e22581baf55595d3328f57b9257846664eb84b4f01`.
It near-calls `0x0C780`, sets `AH=0x39` at `0x0C760`, then near-calls
`0x0C79B`; that helper executes `INT 21h` at `0x0C7A1` and stores a
carry-set DOS error word at `DS:0x4426`. Thus the reached operation is
DOS **make directory**; a native write or error-code trace is still
needed to determine this run's effect.

## Next boundary

p9's native stops establish that the live basename and DS prefix at
`+0x1983`, and the assembled filename at `+0x19A8`, match the private C
candidate for one saved slot-A case. They do not measure the selector's
returned AL, DOS IOResult, allocated record, or directory creation. The
first branch-specific evidence needed is selector return at `+0x19AD`.
After that, `+0x02AE` is a bounded C candidate with explicit directory,
IOResult and display callbacks and a host retry bound; it is not a
complete route candidate without the `+0x0AEC` record loader and heap.
