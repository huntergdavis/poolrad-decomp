# Slot-A U1C +31A8 post-call jump

PM inbox i7696 approved a bounded local C gate for the unconditional short
jump at U1C +31A8. A targeted `deja` query had no exact match; short `deja
"31A8"` found the earlier p9 return-frame session `01a12b3e-db2` and PM
defer i7297. The checked-in
[`SLOT_A_DELAY_RESIDUAL_CALLER_AND_REPLAY.md`](SLOT_A_DELAY_RESIDUAL_CALLER_AND_REPLAY.md)
is reused only for the returned stack frame. The +31A8 successor itself was
explicitly untraced there.

DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Saved +33BD RAM SHA-256 is
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
Original file `[0x3EEE,0x3EF0)` and loaded RAM physical `[0x389C8,0x389CA)`
both contain `EB D5`, SHA-256
`5c4631109e1393c47d57ef4ea5192cde998e2db47aa8a6cbdea61d791910e060`.
The signed displacement `D5` is -43; return IP +31AA minus 43 is +317F.
The stop is **before** the +317F loop condition. The saved loaded bytes pin
code identity, not a native +31A8-to-+317F execution capture.

The p9 return evidence passes through `3582:3151` and `3582:316F`, then
returns to `3582:31A8` with `SS:SP=1D16:3EEE`, `BP=3EEE`, and words
`SS:3EEA=31A8`, `SS:3EEC=3582`. The jump does not read those words, flags,
or RAM. `pr_rec_u1c_31a8_jmp_loop` accepts supplied CPU and 640 KiB or 1 MiB
RAM, requires CS:IP `3582:31A8` and the two loaded opcode bytes, and changes
only IP to +317F. Invalid inputs leave CPU and RAM unchanged.

Synthetic tests compare all 14 CPU words and all 1 MiB RAM after both valid
RAM sizes and both ZF values, including the documented returned frame. They
also cover wrong CS/IP, each opcode byte, size and null inputs. The original
and loaded source audit uses ignored `GAME.OVR` and saved RAM. Strict C11,
ASan/UBSan, CMake registration, and diff hygiene are local verification only.

## Parity review

- **Target:** U1C +31A8..+31A9, stopping at +317F.
- **Candidate:** `pr_rec_u1c_31a8_jmp_loop`.
- **Signals:** matching original/loaded bytes, signed displacement, p9's
  returned frame, synthetic full-state preservation tests.
- **Verdict:** conditional source-instruction match; natural successor
  execution and full-function parity remain unverified.
- **Limit:** no dispatcher, +317F loop, native, strict/full-C, or game-function
  credit is claimed.
