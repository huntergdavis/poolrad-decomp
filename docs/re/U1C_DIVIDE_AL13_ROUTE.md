# Saved one-Up AL=13: ECL DIVIDE route and call audit

> **Update:** A later controlled native replay reached the DIVIDE handler,
> measured `10 / 3 = 3`, stored the result to `0x9808`, and returned.
> See [the execution trace](U1C_DIVIDE_AL13_NATIVE_EXECUTION.md). The run-007
> observations below remain the prebranch evidence for this original audit.

The ignored native run `slot-a-branch-live-007` stopped just after the first ECL RANDOM store at
`3582:028E`, before the conditional branch or DIVIDE executes. Its RAM has latch `0x08`, script PC
`0x9B40`, window-2 word `0x980F = 13`, and word `0x9808 = 10`. The script's later `0x9B68`
instruction is opcode `0x06` with slot 1 = variable word `0x9808`, slot 2 = byte literal `3`,
and slot 3 = destination `0x9808`. The [branch sweep](SLOT_A_PARTY_INPUT.md) establishes that
AL=13 selects this instruction. **Execution of DIVIDE itself was not captured in this run**;
the value `3` and following store are predictions from the checked machine code and helper
paths. This does not establish full DOS/C replay parity.

## Hash-locked inputs

| Ignored input or measured span | SHA-256 |
| --- | --- |
| DOS v1.3 `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| `GAME.OVR` file `0x0EE9..0x0F79`, unit `0x1C:+0x01A3..+0x0233`, 145 bytes | `32f04c0374596652b532b94da3cd8eddfaa90108b8b3b7ed9f754c7591aa7707` |
| Run 007 RAM at `3582:028E`, 640 KiB | `e756eb4ff9a94fc63d434e57aa5d9c790ff61786b509edf87f38f1971aa65c80` |
| Relocated 145-byte handler at `3582:01A3` in that RAM | `23819e5c20e81940bd9d098583bf52e7e810555a49553d6550bbd89fdb0473f4` |
| Nine script bytes at virtual PC `0x9B68` in that RAM | `419b6b01316681edad2b0881877c6914f58f3edd3b89fb35238928183a52d0b5` |

The original handler has only four far-call sites. The RAM relocation resolves all four through
`084F` stubs; each five-byte site was hashed separately:

A byte-for-byte comparison of the 145-byte disk span with the RAM span found exactly eight
different bytes, at unit offsets `+0x01AF/+0x01B0`, `+0x01C1/+0x01C2`,
`+0x01E1/+0x01E2`, and `+0x022E/+0x022F`. These are the segment words of the four far
calls below; all other instruction bytes agree.

| Unit offset | Live target | Site SHA-256 | Reached effect for this instruction |
| --- | --- | --- | --- |
| `+0x01AC` | `084F:002A` | `e95e7e53d5f7bfdd6c07d9f9ed26e3103641c9f6fe8ebd56f6f287a959799928` | Loader(3): records tags/bytes and advances PC to `0x9B71`. |
| `+0x01BE` | `084F:0025` | `ed9381db2d8482ebc62288d874c93494e8f1cd34ada61c88a07148581bdd3dac` | Called twice: slot 1 reads word `0x9808`; slot 2 returns literal `3`. |
| `+0x01DE` | `084F:004D` | `f25b91ca31934f43c214f79d63e2909915f917fb467f9faeb2a25e4c12681f04` | Joins slot-3 bytes into destination word `0x9808`. |
| `+0x022B` | `084F:0052` | `4f3457fbcbfa0893d58d8fadc77d12a070bc02a46b0dab4dd9b842faab5f2720` | Stores result as a word to `0x9808` if division succeeds. |

The script buffer pointer at `DS:[0x49DE]` is `3AD6:0000`; the window-2 pointer at
`DS:[0x49DA]` is `3A96:0000`. Thus `0x9808` is class 2 (`0x9700..0x98FF`), at word
index `0x108` / byte offset `0x210` in window 2; the native snapshot reads **10** there.
The slot-1 `0x2B:0x25` path calls the class-2 load-word core and directly reads that word.
Slot 2 has tag 0, so it zero-extends its low byte; any older high byte in that loader slot
is ignored. Slot 3 supplies the address. The class-2 `0x2B:0x52` path writes the word
directly, with no class-1 load or store hook. The loader resets its string count, records
the three slots, and advances the PC; these tags invoke no string unpacking or allocation.
The [helper contract](ECL_CONTRACTS.md) and [existing handler audit](U1C_ARITH_01A3.md)
are the prior evidence used for the call mapping; the live addresses and hashes above were
checked against the ignored RAM snapshot.

At `+0x020B..+0x0213`, the handler sets `AX = v1`, clears `DX`, then executes unsigned
16-bit `div word [bp-4]` with `v2`. Here `DX:AX = 0:10`, divisor `3`, so AX becomes `3`;
the store call would change `0x9808` from 10 to 3. The next instruction at `0x9B71` is
MULTIPLY of that same word by literal 2 into itself (its nine-byte script span hashes
`3cacf7797f9d929f2a2484ce6109ab6660341c61576744d6d8d42cf8a51d3b04`), so
the bounded C model predicts 3 to 6 if dispatch continues. Those stores were not
measured natively in run 007.

For a nonzero 16-bit divisor, the quotient cannot overflow because DX is zero and AX is
at most `0xFFFF`. A zero divisor instead raises processor INT 0 before the store call.
Run 007's installed vector is `131C:00DF`; its first instruction loads AX with `0x00C8`
(runtime error 200), entering the runtime halt path. The C model returns
`PR_U1C_ARITH_DIVIDE_ERROR` with no store and records error 200; it does not emulate
that halt. The runtime vector is native evidence, while the zero-divisor path itself
is a byte-level inference, not a run-007 observation.

The synthetic `test_overlay_u1c_arith_01a3` covers a two-instruction same-word chain
with `0xFFFF / 3` followed by multiplication, and a zero divisor read from a second
class-2 word. It verifies PC advance, source-before-store behavior, direct store status,
and no store on the fault. No original script bytes or RAM capture are committed.
