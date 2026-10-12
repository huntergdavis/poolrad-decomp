# Unit 0x6F first-slot filename and file-probe boundary

`pr_u6f_party_filename_first` continues the merged
[first-slot string chain](U6F_PARTY_STRING_CHAIN.md) at unit `0x6F:+0x1983`.
The prior frame supplies its source-driven normalized basename at
`[BP-0x1F6]`. The caller supplies a live Pascal string from `DS:0x537A`
and the compiled Pascal suffix at `CS:0x15E9`. The source instructions
establish the DS string's role as the **first component of the filename
local**: SYSTEM StrLoad writes it at `[BP-0x312]`, and two StrCat calls
append the basename and suffix to that same retained destination. Its
larger meaning is not inferred from the private bytes.

The model reaches resident `0x0802:0x02EB` at `+0x19A8`, the first file
effect. With no file callbacks it returns a constructed filename and the
explicit `+0x19A8` boundary. With callbacks it invokes the merged
[bounded resident file-probe model](RESIDENT_DAX_SELECTOR_02EB.md) and
selects the next PC from AL. It stops before `+0x19B1` allocation or
`+0x19EA` missing-file continuation. It does not claim the saved first
file exists, execute record/sidecar effects, establish C-native one-Up
parity, or change the verified game-function count.

Before decoding, `deja "Poolrad u6f 1983 filename construction 19A8 first
file effect slot A"` found no exact session. The shorter `deja "1983"`
returned the prior Claude caller decode; the source bytes were independently
checked. Reused pC's caller/alias audit (inbox i775), the merged resident
`0x02EB` instruction audit, and p9's live DS input measurement (inbox i774,
independently checked by PM in i777).

## Hash-locked call slice

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Caller unit `0x6F:+0x1983..+0x19B0`, GAME file `0x1F969..0x1F996` | **46 bytes, 18 contiguous instructions** | `bcbb88786a899c12bc85bf2bee9ec8843a863d284535da2c6b452710086c65a7` |
| Compiled suffix `CS:0x15E9`, GAME file `0x1F5CF..0x1F5D3` | 5 Pascal bytes, length 4 | `b62e1e6d72460352cdcf1a56fc80b48afadc448d103ec7e9a739d6f2a99b7402` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| SYSTEM `0x0B0B` StrLoad, START file `0x0B781..0x0B79A` | 26 bytes | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| SYSTEM `0x0B98` StrCat, START file `0x0B80E..0x0B839` | 44 bytes | `c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64` |
| Resident `0x0802:0x02EB`, START file `0x08342..0x083AC` | 107 bytes | `141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474` |

| Caller offset | Reached source effect |
| --- | --- |
| `+0x1983..+0x1992` | Push filename destination `SS:[BP-0x312]` and live source `DS:0x537A`; SYSTEM StrLoad copies the Pascal length/text and retains the destination pointer with `RETF 4`. |
| `+0x1993..+0x199D` | Push prior normalized basename `SS:[BP-0x1F6]`; SYSTEM StrCat appends it to the retained filename destination and again retains that destination pointer. |
| `+0x199E..+0x19A7` | Push compiled `CS:0x15E9` suffix; a second StrCat appends it and retains the filename pointer. StrCat saturates at Pascal length 255; this bounded model accepts sums that fit the actual local. |
| `+0x19A8` | Resident `0x0802:0x02EB` receives the retained filename at `[BP+6]`, copies at most 40 characters, then invokes Assign, Reset at record size `0x80`, Close, IOResult, and a `DS:0x49CE` word write. These DOS operations are explicit host callbacks. |
| `+0x19AD..+0x19B0` | `OR AL,AL; JE +0x19EA`: completed probe AL=0 selects missing-file continuation; otherwise next PC is `+0x19B1` GetMem. AH is not tested. Neither branch is executed here. |

The filename local starts at `[BP-0x312]`, two bytes into the old
`[BP-0x314]` intermediate; that intermediate is dead after `0x0425`.
The basename local overlaps a formerly live literal scratch region, whose
length-one value is likewise dead after the prior calls. The C API keeps
the live Pascal values separate and requires the prior frame to contain
the completed cap-40 basename. It rejects a constructed Pascal span over
254 bytes, the physical region before the neighboring local, rather than
modeling cross-local corruption. Host validation failures leave output
untouched. A file-callback failure retains already completed callbacks
and exposes the constructed filename at the `+0x19A8` boundary.

## Measured input and remaining native check

p9's hash-verified private +1974 RAM has `DS=0x14A0` and a Pascal input
at `DS:0x537A` of length **11**, 12-byte SHA-256
`903c2ad4e1bb2d3871d2880ff2bb4961eafb7a8d95bcb9d609da2464e3a4cfde`.
The local audit copies only those 12 bytes into ignored `resources/`, verifies
that hash and the compiled suffix span, restores the ignored save, and runs
both merged string helpers before this C slice. Its first constructed
filename has component lengths **11 + 8 + 4**, Pascal length **23**, and
24-byte SHA-256
`74c0568877934bb1fdedebf542d12210bd5fa43a70d557a32ef1630daa6ea843`.
This is a **C candidate output hash**, not a native filename comparison.
The saved file-probe IOResult and following record effects remain unmeasured
in this composition.

`bash tests/recovered/check_overlay_u6f_party_filename.sh` runs focused
strict-warning synthetic paths. With ignored save/game/table/live-prefix
inputs it pins five new source spans, invokes the prior nine-span gate,
and checks the numeric/hash-only private candidate. Synthetic tests cover
the retained-destination filename, missing host I/O boundary, successful
and failed existence branches, callback order and failure, zero-length
name, malformed prior frame, and an overlong local. No original filename,
literal, game media, RAM, or derived string bytes enter Git.
