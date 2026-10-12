# Resident 0x0802:0x0425 table-driven string transform

`pr_rec_res_0802_0425` models the complete visible Pascal-string output of
the 234-byte resident body called by `GAME.OVR` unit `0x6F:+0x196F`. Its
ten table bytes are **caller-supplied runtime DS input**. The source is the
output of the merged [0x0356 transform](RESIDENT_STRING_0356.md); this C
routine calls that implementation for each table byte. Neither this direct
routine nor its test composes the missing SYSTEM StrLoad at `+0x1965`, the
later record/file effects, or the DOS one-Up route. Resident callable
provenance remains `game_or_other_library_unresolved`; this note makes no
game-specific strict-C or identity count claim.

Before decoding, the exact `deja "Poolrad resident 0802 0425 string transform
ten-byte DS 0D1A slot A"` query found no match. The shorter `deja "0425"`
recovered the earlier pC instruction/ABI audit (inbox i680, published in
[the party string boundary](SLOT_A_PARTY_STRING_BOUNDARY.md)) and p9's native
table watch. Their findings were independently checked against the pinned
bytes below. The call-time table provenance is also in p9's
[PR #246](https://github.com/huntergdavis/poolrad-decomp/pull/246).

## Hash-locked bounds and caller

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Resident `0x0802:0x0425`, loaded `0x08445..0x0852E`, START file `0x0847C..0x08565` | **234 bytes, 91 contiguous 16-bit instructions** | `28c69009d8bd4218b1eb50754c3f32a6f36ef40d3927d630fc8481d10b99e5a3` |
| Callee `0x0802:0x0356`, START file `0x083AD..0x0847B` | 207 bytes | `ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889` |
| Caller unit `0x6F:+0x1965..+0x1973`, GAME file `0x1F94B..0x1F959` | 15 bytes | `8caf30b0059e878ceaacb6f7fc0ec602ddd1838f9410c950805ad63d3c4e5126` |

The 0425 prologue starts exactly at `0x08445`; its `RETF 4` ends at loaded
`0x0852E`, immediately before the next mapped resident routine. At the far
call, `[BP+6]` is the preceding Pascal intermediate and `[BP+0xA]` is the
destination `[caller BP-0x414]`. The routine copies its input before changing
it. It does not directly read or write a window, party record, file, or heap.

## Reached instruction paths

| Resident offset | Reached behavior |
| --- | --- |
| `+0x042C..+0x043B` | Copy source into a 255-character local with SYSTEM `0x0B25`. |
| `+0x0440..+0x0487` | Set byte index to 1; for each of ten indices, read `DS:[0x0D19+i]`, make a one-character Pascal needle through `0x0C3B`, call local `+0x0356`, then copy that result back into the mutable local. The comparison at `+0x0482` ends after index 10. |
| `+0x0489..+0x04AC` | SYSTEM `0x0B57` copies one-based prefix of at most eight bytes, then `0x0B25` stores it back to the local. |
| `+0x04B1..+0x04F3` | If copied length is nonzero, visit every copied byte once, calling SYSTEM `0x182A` for ASCII `a..z` uppercasing. |
| `+0x04F5..+0x050C` | SYSTEM `0x0B25` stores the Pascal result to the far destination with cap `0x28`, then `RETF 4`. The earlier prefix makes this cap nonbinding on valid inputs. |

The reached runtime helper spans were decoded from the same START image:

| SYSTEM offset | START file span | SHA-256 | Effect |
| --- | --- | --- | --- |
| `0x0B25` | `0x0B79B..0x0B7BE` | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` | Copy length and text under the pushed cap. |
| `0x0B57` | `0x0B7CD..0x0B80D` | `18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa` | One-based prefix copy. |
| `0x0C3B` | `0x0B8B1..0x0B8C2` | `b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484` | Write Pascal length 1 followed by the passed byte. |
| `0x182A` | `0x0C4A0..0x0C4B2` | `738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c` | Convert ASCII lowercase to uppercase; leave all other byte values unchanged. |

The C API validates complete input spans and at least ten table bytes,
uses bounded local copies, and leaves output untouched on host argument or
capacity failure. The DOS routine has no such host error status. Copying
the supplied table before processing permits input/output aliasing; the
ten original byte values are read in the same order. The `0x0356` fixed
scan index and deletion behavior is preserved by invoking its merged C
translation once per table byte.

## Verification and limit

The synthetic test covers all misses, ordered hits, fixed-index deletion
effects, zero-length input, eight-byte prefix, ASCII-only uppercase,
binary bytes, aliasing, and host bounds. Run
`bash tests/recovered/check_resident_string_0425.sh` for the focused strict
warning build. Passing an ignored saved game, overlay, and 10-byte table
file also checks seven exact source/caller/helper spans and the pinned input
hashes. On that private local case, the first saved slot has count **6**,
source length **8**, the `0x0356` result length **8**, and the C `0x0425`
result length **8**. The live table has **10 distinct nonzero bytes** with
SHA-256 `cf4e5025c04cb19b9596b886668127dababab7e9b18c05532b426d82bcb89959`.
This checks input provenance and numeric C output only; no native scratch
buffer comparison or C-native one-Up parity is claimed. No original table,
game text, save, or derived string bytes are committed.
