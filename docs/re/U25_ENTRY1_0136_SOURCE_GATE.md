# U25 entry 1 +0136 complete local body

The original DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U25 public entry 1 `+0136..+01B5` is file `[0x7609,0x7689)`, 128 bytes,
SHA-256 `0dd25983c54922e1ebe6f9f1b693f32c6a1c2b69199ddf110fc7d040134ec42b`.
Its 57 aligned instruction starts terminate at the sole outer `RETF 2`
at `+01B3`; next code starts at `+01B6`. The callable ledger reports
52 far callers and grade A game-evidenced but provisional classification.

The input's low byte is sign-extended with `CBW` before each of three
independent DS table reads at `+7085`, `+7045`, and `+7005`. The third
table byte selects a branch:

| Type byte | Original path |
| --- | --- |
| `00` | Zero-extend the first table byte into the BP-local result word. |
| `01`, `03`, `80` | Form a word from the second and first table bytes, `PUSH CS; CALL near +0E0C`, then save returned AX. |
| `02`, `81` | Push both byte-bearing AX words with the live CBW high byte, `PUSH CS; CALL near +06AF`, then save returned AX. |
| All others | Read the preexisting BP-local result word. The source does not initialize it. |

Both near calls use an explicit pushed CS, so their callee-entry stack
contains a far return frame. The full-state callback owns callee effects,
return cleanup, and possible transfer; the source requires return to
`+0191` or `+01AA` before continuing. It preserves live CPU words, full
1 MiB RAM, offset and physical wrapping, flags, stack order, and the
outer `RETF 2`. The loaded U25 code is assumed immutable.

The [callable ledger](overlay_function_ledger.csv) supplies the exact
entry/caller classification. The separately recovered
[U25 +0E0C helper](U25_LOAD_WORD_0E0C.md) is callee context; this
candidate does not fold either callee into the entry body. Two bounded
`deja` ownership queries timed out without a reusable indexed result.
Fresh-main search found no named source/header/test for this entry. PM
cleared ownership in i8320; pG's U25 `+1EA6` reservation is disjoint.

The focused synthetic C test checks all selected type values, an unmatched
type's untouched local result, signed `FF` index, both callee-entry stack
frames, callee return values, and the outer return frame. The source
checker locks the original 128-byte body, both near-call destinations,
and `RETF 2`. Original-byte full-state parity and a native reached frame
remain pending the PM host-native slot. This provisional source gate
does not change the strict full-C count of 82.
