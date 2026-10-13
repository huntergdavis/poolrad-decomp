# Saved one-Up AL=13: native DIVIDE execution

One private DOS replay of saved slot A reached the unit `0x1C:+0x01A3`
DIVIDE handler. The original code read word `0x9808 = 10`, divided it by
literal `3`, stored quotient `3` back to `0x9808`, and returned to its caller.
The bounded C arithmetic model gives the same operands, result, store, and
next script PC for this instruction. This is a **match for this class-2,
nonzero-divisor path**, not whole-handler or full-game parity.

The earlier [AL=13 route audit](U1C_DIVIDE_AL13_ROUTE.md) ended at
`3582:028E`, before the branch. Its exact 640 KiB RAM image was recovered
locally by SHA-256 and used to recover the preceding RNG seed. The
[strict-parity audit](U1C_ARITH_STRICT_PARITY_GAPS.md) remains applicable to
other operand classes, hooks, and divide faults. Those two notes supplied the
call mapping and expected addresses; the execution and hashes below are from
a new replay, `resources/oracle-runs/slot-a-al13-divide-codex-004/` (ignored).

## Original inputs and controlled seed

| Private input | SHA-256 |
| --- | --- |
| DOS v1.3 `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| DOSBox-X 2026.01.02 heavy debugger | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |
| Earlier run-007 RAM at `3582:028E` | `e756eb4ff9a94fc63d434e57aa5d9c790ff61786b509edf87f38f1971aa65c80` |

The fresh run used the documented slot-A load sequence and received exactly
one Up key.
At the pre-RNG breakpoint `3582:026C`, its unmodified seed at
`DS:4428..442B` was `0x10982F39`; the generator would have selected AL=8.
The earlier run-007 post-RNG seed is `0x645985A2`. Inverting the audited
generator `(seed * 0x08088405 + 1) mod 2^32` gives its pre-RNG seed
`0xD110A9ED`. The heavy debugger command
`SM 14A0:4428 D:D110A9ED` restored that seed before the original RNG call.
Full 640 KiB images before and after `SM` differ **only** at physical
`0x18E28..0x18E2B` (`DS:4428..442B`). No result register, script byte,
party coordinate, handler code, or destination word was set by the debugger.
This replay is therefore a controlled native execution of the run-007 AL=13
route, not an unmodified-clock repeat.

| Stop | Full RAM SHA-256 | Measured state |
| --- | --- | --- |
| `3582:026C`, before seed restore | `fc9dd3524cb3ca4a61b752347322475703c817d04e9dacdc140e5c92f84f70d4` | `AX=14`, `DS=14A0`, script PC `9B40`, party `(14,4,6)`, `0x980F=16`, `0x9808=10`. |
| `3582:026C`, after seed restore | `f3f62e9c0540c8622e481b1a1638588982feb68707e698e6a312d17a3fed8f51` | Seed `0xD110A9ED`; only the four seed bytes changed. |
| `3582:0271`, RNG return | `dd6b21f047a4ff2b424db78e0d0b8670c1dfd75ee8e51a6045fa878c494f72b7` | `AX=13`; seed advanced to `0x645985A2`; `0x980F` still 16. |
| `3582:028E`, RANDOM store return | `1f64933f9a8e9c11b167d4e94b8edacea421a2d8b7a202a3a684fe410eb2525b` | `0x980F=13`, `0x9808=10`, script PC `9B40`. |

## Reached branch, operands, store, and return

The dispatcher subsequently entered `3582:01A3` with `DI=0x9B68`, script
PC `0x9B68`, and opcode latch `0x06` (DIVIDE). This is direct observation of
the AL=13 branch's selected handler. The conditional at script PC `0x9B66`
was not separately breakpointed. The live nine-byte instruction at `0x9B68`
hashes `419b6b01316681edad2b0881877c6914f58f3edd3b89fb35238928183a52d0b5`;
its decoded slots are variable word `0x9808`, byte literal `3`, and
destination word `0x9808`. The relocated 145-byte handler hashes
`23819e5c20e81940bd9d098583bf52e7e810555a49553d6550bbd89fdb0473f4`,
matching the earlier run-007 relocation.

| Native stop | Full RAM SHA-256 | Measured effect |
| --- | --- | --- |
| `3582:01A3`, handler entry | `6836f53ce94cf740e0494b5448ae10e9f38fd20d837afc696d885574d1c759ea` | `DI=9B68`, opcode `06`, `0x980F=13`, source word `0x9808=10`. |
| `3582:0210`, before unsigned `DIV` | `b996d110b425a4439e274a38715e50ec129819cd779beff55d2aa5ea5326d20f` | `DX:AX=0:10`; stack locals `[BP-6]=10`, `[BP-4]=3`, `[BP-2]=9808`; script PC advanced to `9B71`; destination still 10. |
| `3582:0213`, after `DIV` | No RAM dump | Native registers `AX=3`, `DX=1`: quotient 3, remainder 1. |
| `3582:022B`, before `084F:0052` store | `11f3b5f06fc537394768c4d767651ebb866bef98208fa149ac502e761dfe85e4` | Result local `[BP-9]=3`; destination `0x9808` still 10. |
| `3582:0230`, after store helper | `b4919a8c5c026e481fff3b6ad2fe93aa8b25741414afa395b9e7d8400bccf1d3` | Window-2 word `0x9808` changed to **3**; script PC remains `9B71`. |
| `3582:2F73`, far-call return target | `b4919a8c5c026e481fff3b6ad2fe93aa8b25741414afa395b9e7d8400bccf1d3` | Return target reached with `SS:SP=1D16:3EE8`, script PC `9B71`, `0x9808=3`. |

At the post-store stop, the native stack held far return `3582:2F73` at
`SS:[BP+2]` / `SS:[BP+4]`; the debugger then stopped at that exact caller
address. The identical post-store and return RAM hashes reflect register and
stack-pointer changes without a further memory write in this interval.

## Bounded C comparison

A local probe called `pr_u1c_op_arith` with the live nine-byte instruction
placed at offset `0x268` of its `0x9900`-based script buffer, PC `0x9B68`,
latch `6`, and window-2 word index `0x108` initialized to `10`. It returned
`PR_U1C_ARITH_OK`, `v1=10`, `v2=3`, destination `0x9808`, result `3`,
`stored=1`, next PC `0x9B71`, and destination word `3`. The existing targeted
`poolrad_overlay_u1c_arith_01a3_tests` executable also passed. The measured
native outcome and the C model match for these observables; no C change was
needed. This comparison does not assert parity for class-1 or class-4 helper
paths, a zero divisor's runtime halt, other script routes, or the surrounding
gameplay loop. It adds no full-C or playable-function credit.

All original media, the saved game, screenshots, debugger stream, full RAM
images, and the local probe remain under ignored `resources/` or `/tmp` and
are not committed.

## Private reproduction

Use `tools/oracle/prepare_run.sh` to copy the original inputs under ignored
`resources/oracle-runs/`. Launch the hash-matched heavy DOSBox-X build with
`AUTOTYPE -w 160 -p 8 enter l , a`, pause at the loaded view, and arm
`BP 3582:026C`, `BP 3582:028E`, and `BP 3582:01A3`. Resume and send one Up.
At `+026C`, run `EV AX BX CX DX CS IP DS SS SP BP` and
`MEMDUMPBIN 0000 0000 A0000`, preserve the private dump, then run
`SM 14A0:4428 D:D110A9ED` and dump again. Verify the four-byte RAM diff
before resuming. Arm `BP 3582:0271` to measure AL=13 at the RNG return,
then continue to `+028E` and preserve that RAM. The stop at `+01A3` is the
reached DIVIDE handler. At that point, use `BPDEL *` and arm
`BP 3582:0210`, `BP 3582:0213`, `BP 3582:022B`, and `BP 3582:0230`.
Record registers and move each private `MEMDUMP.BIN` before the next dump.
Finally arm `BP 3582:2F73` to observe the far return. Keep all original
media, debugger output, and RAM images in ignored private storage.
