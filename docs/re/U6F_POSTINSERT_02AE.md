# Unit 0x6F post-insertion directory operation

The no-argument unit `0x6F:+0x02AE..+0x032E` body has a bounded C model in
`pr_u6f_postinsert_02ae`. Its only known direct call is `+0x19E7`, after
the selector's nonzero-AL branch, allocation, record loader and party
insertion. A [provisional native first-slot measurement](https://github.com/huntergdavis/poolrad-decomp/pull/255)
reports `AX=0x0001` at `+0x19AD`, selecting `+0x19B1` for that iteration.
It does not measure the later allocation, record load, insertion or
`+0x19E7` execution. This model earns no full-route or strict-function count.

Before decoding, `deja "Poolrad u6f 02AE mkdir IOResult 0 5 retry directory"`
found no matching indexed session; `deja "u6f 02AE"` returned prior party
build research. The merged [continuation map](U6F_PARTY_CONTINUATIONS.md)
supplied the upstream branch and initial bounds. The merged
[party-input](SLOT_A_PARTY_INPUT.md),
[string-boundary](SLOT_A_PARTY_STRING_BOUNDARY.md), and
[selector](RESIDENT_DAX_SELECTOR_02EB.md) notes supplied context; all spans
below were independently checked against ignored DOS inputs. No original
literal or executable bytes appear here or in the tests.

## Source and caller evidence

`GAME.OVR`: 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit file base `0x1DFE6`; `+0x02AE..+0x032E` maps to file
`0x1E294..0x1E314`, 129 bytes / 51 x86-16 instructions, SHA-256
`ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539`.
The private Pascal prefix at CS:`0x029A` is 18 bytes including its length
at file `0x1E280..0x1E291`, SHA-256
`cb2247d8e036127fdc14edbe785093db176bb7d8161838f29269dd151755cf80`.
The private suffix at CS:`0x02AC` is 2 bytes at file
`0x1E292..0x1E293`, SHA-256
`63ded01247acc7888c74eae4707f8bd28a532c761ebcbe16d3e0bb7996945a28`.
Their bytes remain private; the public API receives both from the host.

| Offset | Decoded action | Model boundary |
| --- | --- | --- |
| `+0x02B5..+0x02CB` | `Copy` of live DS:`0x537A` starting at one-based index 1 with count `length-1`; runtime clamps negative count to zero. | Re-read live Pascal string on each attempt, retain first `max(length-1,0)` characters. |
| `+0x02D0` | Far-call SYSTEM `0xAF8:0x17D6`. | Host `make_directory`, receiving the converted ASCIIZ path. |
| `+0x02D5..+0x02E7` | Far-call SYSTEM IOResult `0xAF8:0x04A2`; words 0 and 5 branch to return. | Host `io_result`; both words exit without message callbacks. |
| `+0x02E9..+0x0315` | Push color `0x0E`; load private prefix, turn raw DS:`0x537B` into one character, append it and private suffix. | Host-supplied literals; message is built only on other IOResult words. |
| `+0x031A` | Far-call resident `0x0709:0x0BF5`. | One opaque `display_and_input` callback for row clear, display and one key read; its key result is ignored. |
| `+0x031F..+0x032E` | Recheck stored IOResult; 0/5 return, otherwise jump to `+0x02B5`. | Host attempt limit stops before the next directory operation. |

`START.EXE`: 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The hash-locked loader expansion places SYSTEM `0xAF8:0x17D6` at
loaded `0xC756..0xC768` (19 bytes, SHA-256
`90731793b0ac2c364059b1e22581baf55595d3328f57b9257846664eb84b4f01`).
It calls the Pascal-to-ASCIIZ helper at `0xC780..0xC79A` (27 bytes,
SHA-256 `94899fa2b266515992d6ea94fef07edcd168dc03e772b3aea6f34c9bb655d5d9`),
which caps at **0x7F characters** and terminates the path. At `0xC760`
the wrapper sets DOS `AH=0x39`, then calls `0xC79B..0xC7A9` (15 bytes,
SHA-256 `978fabd0bca91df715152943c57b87c26e835dbd9533fbb60843e32bfc86908b`),
whose `INT 21h` stores a carry-set error word. This establishes mkdir,
not a filename-open operation.

The resident display/input callee at loaded `0x7C85..0x7CCE` is 74 bytes,
SHA-256 `4a3f940caae95794f257a25f5241bcf3a04c2749b212f5b8d9def5befd63a4cc`.
At `0x7C95..0x7C99` it passes **0x28** to SYSTEM StrStore before row clear
`0x7CAB`, display `0x7CBE`, and key read `0x7CC1`. Thus the 40-character
visible cap belongs to the resident callee, not to this unit's StrCat chain.

## Model boundary and reproduction

Callbacks run only when their source call is reached. Missing or failing
callbacks return the next source offset and retain earlier effects. The
host-specified attempt guard is checked after an error display and before
the next mkdir. The input is the live DS Pascal span, so a callback change
is visible on retry. The model does not simulate DOS filesystem state,
private literal contents, allocation or the upstream record load.

The synthetic path-cap test uses a long host string to isolate the SYSTEM
0x7F conversion limit. Such a string exceeds this caller's small stack
scratch region; original stack overwrite behavior is outside the bounded
model. The ordinary observed party prefix is within the region. The
synthetic message-cap test uses a long host prefix to isolate the resident
0x28 display limit; the original private prefix is shorter. Neither cap
test claims such oversized input occurred on the saved route.

From the repository root:

```sh
bash tests/recovered/check_overlay_u6f_postinsert_02ae.sh
bash tests/recovered/check_overlay_u6f_postinsert_02ae.sh \
  resources/DOS/POOLRAD/GAME.OVR resources/DOS/POOLRAD/START.EXE
```

The first command compiles with `-Werror` and runs synthetic branches.
The second also checks seven private source spans in memory and prints only
aggregate hash-gate status.
