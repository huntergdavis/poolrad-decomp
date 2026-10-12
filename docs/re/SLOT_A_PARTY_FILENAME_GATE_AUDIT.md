# Slot-A first party filename caller and native boundary audit

This is an independent bounded audit of DOS v1.3 `GAME.OVR` unit `0x6F`
after the first-slot string result. The merged
[string-call composition](U6F_PARTY_STRING_CHAIN.md) ends at `+0x1983`,
and the merged [filename model](U6F_PARTY_FILENAME_BOUNDARY.md) models
construction and a callback file probe through the conditional branch at
`+0x19AF`. This note pins the caller's full post-`0x0425` span and the
separate native buffer observations. It adds no C implementation or
complete loader, count, identity, or C-native saved one-Up parity claim.

Before decoding, `deja "poolrad u6f 1974 19a8 post 0425 output string store
filename file exists sidecar"` found no direct prior session. The merged
[string boundary](SLOT_A_PARTY_STRING_BOUNDARY.md),
[party-input model](SLOT_A_PARTY_INPUT.md), and
[resident file selector](RESIDENT_DAX_SELECTOR_02EB.md) supplied the prior
call-chain and selector ABI. Their facts were checked against the pinned
caller bytes below; the selected-slot output from the merged C frame is an
input to this audit.

| Ignored input or source span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| `+0x1974..+0x1982`, GAME file `0x1F95A..0x1F968` | 15 bytes, 6 instructions | `40340a7f4f937729d3786c40ce28c8cc26d796f9abd977727eaafe0a254b1253` |
| `+0x1983..+0x19A7`, GAME file `0x1F969..0x1F98D` | 37 bytes, 15 instructions | `980cf27a7e57ecae8ab4faceddcaa39190ada07c3464b26d08ab62e7e9178639` |
| `+0x19A8..+0x19B0`, GAME file `0x1F98E..0x1F996` | 9 bytes, 3 instructions | `34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb` |
| Post-handoff `+0x1983..+0x19B0` | 46 bytes, 18 contiguous instructions | `bcbb88786a899c12bc85bf2bee9ec8843a863d284535da2c6b452710086c65a7` |
| Full `+0x1974..+0x19B0` | 61 bytes, 24 contiguous instructions | `21afbfb922ed46291a8fe14d424e6c805619bf8612f0839b5908097e1238a949` |
| Private `CS:+0x15E9`, GAME file `0x1F5CF..0x1F5D3` | 5 bytes, Pascal length 4 | `b62e1e6d72460352cdcf1a56fc80b48afadc448d103ec7e9a739d6f2a99b7402` |

## Source order and exact boundary

| Caller offset | Verified effect |
| --- | --- |
| `+0x1974..+0x1982` | Push destination `SS:[BP-0x1F6]` and cap `0x28`; SYSTEM `0x0AF8:0x0B25` copies the far source pointer retained by `0x0802:0x0425` at `SS:[BP-0x414]`. Its `RETF 0xA` clears the string arguments. The merged C frame exposes this Pascal basename and next PC `+0x1983`. |
| `+0x1983..+0x1992` | Push new filename destination `SS:[BP-0x312]` and live source `DS:0x537A`; SYSTEM `0x0AF8:0x0B0B` copies that Pascal string and retains the destination pointer. This is the first sequence after the prior C handoff and is covered by the merged filename model. |
| `+0x1993..+0x199D` | Push basename `SS:[BP-0x1F6]`; SYSTEM `0x0AF8:0x0B98` appends it to the retained filename destination, capped at 255, and retains that destination. |
| `+0x199E..+0x19A7` | Push private `CS:+0x15E9` Pascal suffix; another `0x0B98` appends it, retaining the assembled filename pointer. The suffix bytes stay in ignored host input. |
| `+0x19A8` | Resident `0x0802:0x02EB` consumes the retained far filename pointer. Its bounded model copies at most `0x28` characters, calls Assign, Reset, Close, IOResult in order, writes the full IOResult word to `DS:0x49CE`, and returns AL=1 exactly when the copied name is nonempty and IOResult is zero. It `RETF 4` consumes the pointer. These file and global-state callbacks are the first external host effects after filename assembly. |
| `+0x19AD..+0x19B0` | `OR AL,AL; JE +0x19EA`: absent or failed file test skips the allocation/record insertion for this slot; nonzero AL falls through to `+0x19B1` GetMem of `0x11D` bytes. The record and later sidecar effects are outside this audit. |

The source order proves only a **leading Pascal filename component** at
`DS:0x537A`, followed by the normalized basename and private suffix. It
does not establish a directory, device, or path meaning for that component.
The selector's 40-character copy cap applies to the assembled name; the
preceding SYSTEM append operations each have their own 255-character bound.
The merged C model rejects a constructed string that would exceed the
254-byte physical local region starting at `BP-0x312`; the pinned 23-byte
case is within that region. This host bound is a reported limit, not a DOS
branch.

Two local addresses reuse earlier scratch space after its value is consumed:
`BP-0x1F6` lies inside the nominal 256-byte region starting at `BP-0x214`,
but the active compiled literal occupies only two bytes at the beginning;
`BP-0x312` lies two bytes into the old `BP-0x314` intermediate string, which
is dead after `0x0425`. A portable frame may hold the logical Pascal values
in separate arrays. A physical stack replay must keep these lifetimes and
addresses straight.

## Pinned buffers and selector branch

The native [return-side watch in PR #249](https://github.com/huntergdavis/poolrad-decomp/pull/249)
stopped before `+0x1974` at `SS:SP=1D16:37B4`, `BP=3DCA`. Its retained
top-of-stack far pointer and output local were both `1D16:39B6` (`BP-0x414`).
That Pascal string had length 8 and nine-byte length-plus-payload SHA-256
`6c110ad7a5f343c8961fdb3e3762d2d8c7ebfba40902e1fd85015fcf640c3cdd`.
The merged C string-call composition matched this one intermediate output
privately. The separate
[native filename evidence in PR #253](https://github.com/huntergdavis/poolrad-decomp/pull/253)
stopped at `+0x1983` with `SS:SP=1D16:37B8`
and the basename at `SS:3BD4` (`BP-0x1F6`) had Pascal length 8 and the
same nine-byte hash. The copy therefore preserves the active Pascal bytes
on this pinned run; it does not prove broader loader parity.

PM independently read the existing ignored `+0x1974` RAM capture (640 KiB,
SHA-256 `a05b315da2205b048ca8e4ef9fa41a2c672325b81cda8851eb4556044f5091d3`):
at `DS=14A0`, the `DS:0x537A` Pascal component has length 11, payload
SHA-256 `7eb58e7fad324c654b05255489704e14d8eb44da3d709646b48e7c7efc427eab`,
and twelve-byte length-plus-payload SHA-256
`903c2ad4e1bb2d3871d2880ff2bb4961eafb7a8d95bcb9d609da2464e3a4cfde`.
The `+0x1983` stop independently found the same length and full hash at
`DS:0x537A`; its 640-KiB private RAM SHA-256 is
`78e17a06de55c781e6be02293d069bffe8ec3a2308fb8ce881fbb1a4c5d3348a`.
This establishes the input immediately before its `+0x198E` load on the
pinned run. No raw text or table bytes are published.

At the native `+0x19A8` stop, immediately before the file selector,
`SS:SP=1D16:37B4`, `BP=3DCA`, and the assembled filename at
`SS:3AB8` (`BP-0x312`) had Pascal length **23**. Its 24-byte
length-plus-payload SHA-256 was
`74c0568877934bb1fdedebf542d12210bd5fa43a70d557a32ef1630daa6ea843`,
matching PM's private C filename candidate from the merged model. The
private 640-KiB RAM SHA-256 was
`44fd648205d7555863dcd72d487e4d20d1fa1386d29edc8fe8cc908b24e75620`
(the same native PR #253). This is a second intermediate buffer match.
The separate [native selector-return watch](SLOT_A_NATIVE_19AD_SELECTOR_RETURN.md)
measured `AX=0x0001` at `+0x19AD`, `DS:0x49CE=0x0000` before and after the
call, and the `+0x19B1` fall-through on this **first iteration**. The
unchanged word does not disprove the source-level write, and this stop does
not isolate the individual DOS file callbacks or later loop outcomes.

The merged filename C API accepts the basename, a host-supplied
`DS:0x537A` Pascal value, the private `CS:+0x15E9` suffix, and optional
resident selector callbacks. Without callbacks it stops at `+0x19A8`;
with completed callbacks it reports selector AL and the chosen next PC.
An absent file goes to `+0x19EA`, a present file to `+0x19B1`; neither
continuation is executed by that model or this audit. The existing
[party-input model](SLOT_A_PARTY_INPUT.md) supplies a bounded eventual
chain but does not execute these intervening file, record, sidecar, and
post-insertion effects. No count or C-native route parity follows from this
static audit or the two intermediate buffer matches.
