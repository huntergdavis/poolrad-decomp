# Resident 0x0802:0x0356 Pascal-string transform

`pr_rec_res_0802_0356` translates the visible output behavior of the
207-byte resident body called at `GAME.OVR` unit `0x6F:+0x196A`. It takes a
Pascal source string and Pascal needle, deletes matches while advancing a
fixed byte index, and stores the result to the caller's destination. The
slot-A caller's private one-character literal is **host supplied**. This
body has no file, heap, DS/window, or party-record effect. Its broader
game/library identity and strict full-C credit remain subject to PM parity
review; it does not complete the world-save loader or establish C-native
one-Up parity.

Before decoding, `deja "Poolrad resident 0802 0356 string transform slot A"`
found no exact session; `deja "0356"` returned only unrelated uses of the
same offset. Reused the merged
[party-entry ABI](SLOT_A_PARTY_ENTRY.md), the previously decoded runtime
Copy and compare semantics in [U3CE entry 4](U3CE_ENTRY4_01C8.md) and
[the horizontal menu](U1C_HORIZONTAL_MENU_10D1.md), then checked every
reached helper in the hash-locked MZ image. pC independently decoded the
callee and nested Delete path (inbox i697).

## Source and caller bounds

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Resident `0x0802:0x0356`, loaded `0x08376..0x08444`, source file `0x083AD..0x0847B` | **207 bytes, 82 contiguous 16-bit instructions** | `ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889` |
| Caller unit `0x6F:+0x1965..+0x1973`, `GAME.OVR` file `0x1F94B..0x1F959` | 15 bytes, three far calls | `8caf30b0059e878ceaacb6f7fc0ec602ddd1838f9410c950805ad63d3c4e5126` |

The slot-A caller loads a private one-character Pascal literal into local
`[BP-0x214]` at `+0x1965`. At `+0x196A`, `[BP+6]` of this resident body is
that needle, `[BP+0xA]` is the selected 41-byte saved Pascal name slot, and
`[BP+0xE]` is output `[BP-0x314]`. `RETF 8` retains the output far pointer
for the next call, resident `0x0802:0x0425` at `+0x196F`. The output has a
255-byte Pascal-string cap. No literal or name bytes are published.

## Complete visible paths

The body calls SYSTEM `0xAF8:0x0B25` three times, and `0x0B57`, `0x0C10`,
and `0x0CBC` once each. These runtime bodies and `0x0CBC`'s nested calls
were independently decoded from the hash-verified raw MZ image, using the
runtime segment mapping already established in the resident inventory:

| SYSTEM offset | Source file span | SHA-256 | Reached behavior |
| --- | --- | --- | --- |
| `0x0B25` | `0x0B79B..0x0B7BE` | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` | Copy Pascal length and text, capped at the pushed maximum. |
| `0x0B57` | `0x0B7CD..0x0B80D` | `18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa` | Copy substring at one-based index and requested count; out-of-range index gives empty. |
| `0x0B98` | `0x0B80E..0x0B839` | `c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64` | Append Pascal strings, capped at 255; reached inside Delete. |
| `0x0C10` | `0x0B886..0x0B8B0` | `11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58` | Compare length and all bytes; `JNE` means unequal. |
| `0x0CBC` | `0x0B932..0x0B987` | `14be20a0c6b74485ec3dcc683266dc07bc2c6e130d9ddcb2f83434b926996a33` | If count is positive, compose `Copy(s,1,index-1)` with `Copy(s,index+count,255)` via `0x0B98`, then store via `0x0B25`. |

The original first copies source and needle to separate locals. It sets the
loop limit to the **low byte** of `original_source_length - needle_length + 1`.
For `i=1..limit`, it copies the current source substring of needle length,
compares it with the needle, and deletes that many bytes on equality. The
limit is never recomputed after deletion; `i` advances even when deletion
shifts another match into position `i`. Thus adjacent matches can remain.
Underflow and the `255 - 0 + 1` case both wrap to a byte limit; an empty
needle can compare equal but `Delete(count=0)` has no visible effect. The
final `0x0B25` stores only the length byte and live text, preserving bytes
past the result in the caller's output buffer.

The C API validates complete source/needle Pascal spans and output capacity.
Failure leaves output unchanged, a host bound absent from the DOS routine;
valid inputs follow all branches above. Input and output may alias because
the DOS body first takes local copies. The C implementation expresses
Delete's prefix/suffix composition as an in-place shift; in this body the
result never grows, so this preserves its output bytes.

The next caller dependency is resident `0x0802:0x0425`, which reads live
`DS:+0x0D19` table bytes (pC inbox i680); this PR leaves that callee and
the later file/record/sidecar effects outside scope. PR #242's native
window-1 counter endpoints are separate evidence and do not prove any
intermediate write or C-native party parity.

Synthetic `poolrad_resident_string_0356_tests` covers no hit, one hit,
separated/adjacent matches, a multi-byte needle, fixed-loop behavior,
empty needle, byte-limit wrap, full 255-byte source, aliasing, and host
bounds. The optional `bash tests/recovered/check_resident_string_0356.sh
resources/DOS/POOLRAD/SAVGAMA.DAT resources/DOS/POOLRAD/GAME.OVR` also
hash-pins the ignored save, game and START executable plus seven exact
source/runtime/caller spans. It supplies the private first saved slot and
one-character literal to the C routine: source length **8**, needle length
**1**, result length **8**. This numeric match is a local input check, not a
native scratch-buffer comparison. Tests contain no original game text or
bytes.
