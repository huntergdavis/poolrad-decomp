# Second-key resident caller `+00AA` conditional gate

This bounded gate starts at the directly reached `10D2:00AA` stop,
after `CMP DX,BX`, and ends before the selected successor at
`10D2:00C4`. It covers `JG +00B2; JL +00C4; CMP AX,CX; JBE +00C4`.
It does not model the arithmetic and local stores between the nested
callee return at `+008D` and this comparison, nor the instructions
at either successor. It gives no full-function or complete second-key
credit.

## Source and native stops

The required `deja "Poolrad RES_08AE +00AA JG JL CMP AX CX JBE +00C4 second key branch"`
query was issued before this model. Its index refresh completed, but
the search did not return after several minutes and was interrupted;
no claim of absent prior matches is made. The merged
[`+0036` same-run note](SLOT_A_SECOND_KEY_RESIDENT_0036_MUL.md)
supplied the input and replay provenance pattern; the stops below
were measured directly on the continuing native run.

The controlled slot-A run used DOS v1.3 START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and 13,137-byte slot-A SAVGAMA.DAT SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Original START file `0x8BA1..0x8BA8` and loaded `10D2:00AA..00B1`
contain `7F 06 7C 16 3B C1 76 12`, SHA-256
`fead527d34d76315bfe1711325890af5d2d33120caecf875c51e14a84332addb`.

The 14 CPU words are ordered `CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`:

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| `10D2:00AA` | `10D2 00AA 14A0 1D16 1D16 3DAC 3DB4 000A 0000 000A 0000 0001 0000 7246` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| `10D2:00C4` | `10D2 00C4 14A0 1D16 1D16 3DAC 3DB4 000A 0000 000A 0000 0001 0000 7246` | same |

The high WORDs compare equal (`DX=BX=0`), so `JG` and `JL` fall
through. The low WORDs also compare equal (`AX=CX=10`), so `JBE` is
taken to `+00C4`. The intermediate `CMP AX,CX` leaves FLAGS=`7246`.
Direct entry and exit RAM dumps are byte identical.

## Bounded C parity

`pr_rec_res_08ae_00aa_branch` consumes the materialized FLAGS from
the preceding high-WORD `CMP`, preserves all registers and RAM, and
selects `+00B2` or `+00C4`. When the high WORDs compare equal it
materializes all six arithmetic FLAGS from `CMP AX,CX`; otherwise
the conditional jumps preserve input FLAGS. The ignored strict-C11
native-seeded harness compares **0/655,360 RAM byte and 0/14 CPU-word
mismatches**. Focused registered CTest passes the reached equality,
signed high-WORD greater/less, unsigned low-WORD greater/less, and
low-WORD signed-overflow cases. Synthetic cases are instruction checks,
not additional native route claims. Private captures stay in ignored
`resources/`.

## Parity review

- **target_function:** reached `RES_08AE:+00AA..+00B1`, selected
  successor `+00C4`.
- **source_candidates:** `pr_rec_res_08ae_00aa_branch` models the
  conditional chain and the low-WORD comparison.
- **signals:** exact original and loaded bytes; direct same-run entry
  and exit CPU/RAM; full-state C replay; synthetic branch tests.
- **score and verdict:** 4/5 corroborating signal groups, **match** for
  this reached equality route. Other native branch outcomes remain
  unobserved.
- **review_notes:** the incoming high-WORD comparison is outside the
  model and supplied as materialized FLAGS. The observed path does not
  execute the `+00B2` successor. This is a bounded instruction slice
  with zero strict-function and playable-game credit.
