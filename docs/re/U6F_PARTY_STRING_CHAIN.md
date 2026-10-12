# Unit 0x6F first-slot party string call chain

`pr_u6f_party_string_first` composes the reached first-slot string calls at
unit `0x6F:+0x1965`, `+0x196A`, and `+0x196F`, then the `+0x1974` capped
store. Its input slot must come from the merged
`pr_slot_a_party_entry_begin` handoff over restored world-save state. The
compiled one-character Pascal literal and ten live DS table bytes are
caller-supplied; no private text or table values appear in C. Count zero
skips this slice. The result stops at `+0x1983`, before filename construction,
file existence, record loading, sidecars, and party insertion. It does not
establish C-native one-Up execution or a new game-specific function count.

Before decoding, `deja "Poolrad u6f 1965 SYSTEM StrLoad 196A 196F party
string composition"` found no match; the shorter `deja "1965"` recovered
pC's prior finding that `+0x1965` itself was the first absent C instruction.
Reused the merged [first-slot entry](SLOT_A_PARTY_ENTRY.md),
[string boundary](SLOT_A_PARTY_STRING_BOUNDARY.md),
[0x0356 routine](RESIDENT_STRING_0356.md), and
[0x0425 routine](RESIDENT_STRING_0425.md), then independently decoded the
call sequence and SYSTEM `0x0B0B`/`0x0B25` from the pinned binaries.

## Source and stack proof

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit `0x6F:+0x193B..+0x1982`, GAME file `0x1F921..0x1F968` | **72 bytes, 29 contiguous instructions** | `9dd1291f5159418e576116974a60010371dddd79a4be1a5f68f2ec074eac0700` |
| Compiled Pascal literal source at GAME file `0x1F5CD..0x1F5CE` | 2 bytes, length 1 | `5598b3f30c8e047bfd81991dedffa70603c790fcf6f4414d144e6b0d8caeab5f` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| SYSTEM `0x0B0B` StrLoad, START file `0x0B781..0x0B79A` | 26 bytes | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| SYSTEM `0x0B25` StrStore, START file `0x0B79B..0x0B7BE` | 36 bytes | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |

The audited script also hash-checks both resident bodies and their reached
substring, one-byte Pascal, and ASCII UpCase helpers. Their source bounds
and helper behavior are in the two linked resident notes.

| Caller offset | Far-pointer stack effect and live value |
| --- | --- |
| `+0x193B..+0x1964` | Push output local `[BP-0x414]`, intermediate `[BP-0x314]`, selected name slot at `BP-0x171 + 0x29*index`, needle local `[BP-0x214]`, and private literal source. The merged entry selects index 1 and the first transferred slot. |
| `+0x1965` SYSTEM `0x0B0B` | Copy Pascal literal length and bytes to `[BP-0x214]`; `RETF 4` removes only the source pointer, retaining the needle-local pointer. The decoded helper has no cap, so the C boundary requires the compiled length-one shape. |
| `+0x196A` resident `0x0802:0x0356` | Consume the needle local and selected name; write `[BP-0x314]`. `RETF 8` retains its output pointer. |
| `+0x196F` resident `0x0802:0x0425` | Consume `[BP-0x314]`, ten ordered live `DS:0x0D1A..0x0D23` bytes, and write `[BP-0x414]`. `RETF 4` retains that far output pointer. |
| `+0x1974..+0x1982` SYSTEM `0x0B25` | Copy `[BP-0x414]` to basename local `[BP-0x1F6]` under cap `0x28`; `RETF 0xA` clears the remaining string arguments. The 0425 eight-byte prefix makes this cap nonbinding on valid calls. |

The C frame exposes only the live Pascal length/text at each local, plus
the next PC. Its validation checks that the entry matches the supplied
restored state's first slot; it neither substitutes a saved output nor
executes later filename or record effects. Host argument/bounds failures
leave the output frame untouched. The original DOS code has no host status.

## Pinned intermediate comparison and limit

p9's independent saved-slot-A native watch stopped at `u6F:+0x1974`, with
the retained far pointer equal to `SS:[BP-0x414]`. That Pascal buffer had
length **8**, payload SHA-256
`03c11624866b756cd15a30609c7534e0b11e6891051d505106bf44435f24b64e`,
and length-plus-payload SHA-256
`6c110ad7a5f343c8961fdb3e3762d2d8c7ebfba40902e1fd85015fcf640c3cdd`
(inbox i748). The ten call-time DS bytes have SHA-256
`cf4e5025c04cb19b9596b886668127dababab7e9b18c05532b426d82bcb89959`
and are distinct/nonzero, as recorded in p9's merged
[PR #246](https://github.com/huntergdavis/poolrad-decomp/pull/246).

The hash-locked local test restores the ignored saved world state, obtains
the first-slot handoff, reads the ignored compiled literal and table, and
runs this C chain. It yields length **8** after `0x0356`, length **8** at
`[BP-0x414]`, and length **8** after the cap-40 store. Its nine-byte
`[BP-0x414]` output hash matches the native hash above; PM also compared
the private byte buffers directly and found them equal (inbox i753).
This is **one pinned intermediate-buffer parity result**. It is not a
complete C-native saved one-Up route or a proof of subsequent party-file
effects.

`bash tests/recovered/check_overlay_u6f_party_string_chain.sh` runs the
strict-warning synthetic test. With ignored save/game/live-table paths, it
checks nine exact code/literal/runtime spans and the private output hash.
Synthetic paths include zero-count skip, first-slot source selection,
count 255's first iteration, the retained outputs/cap, and malformed
handoffs or local inputs. Raw save, literal, table, RAM, and output buffers
remain under ignored `resources/`.
