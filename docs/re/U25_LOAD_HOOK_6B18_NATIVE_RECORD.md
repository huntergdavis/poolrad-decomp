# Native class-1 load at `0x6B18`: active-record byte

The original DOS v1.3 code, continued from a saved slot-A replay, loaded the
active-record byte **7** for a class-1 operand at `0x6B18` while the distinct
window-1 word at that address was **10**. The load-word caller retained the
hook value, and unit `0x1C` DIVIDE used `7 / 3`, storing **2** to class-2 word
`0x9808`. This is a controlled native execution of this particular hook and
arithmetic path. The `0x6B18` instruction was inserted into the private live
script buffer after the [native AL=13 DIVIDE trace](U1C_DIVIDE_AL13_NATIVE_EXECUTION.md);
its reachability from an unmodified saved script remains unmeasured.

## Provenance and controlled input

The ignored original `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
the slot-A save is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The DOSBox-X binary is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The prior native return RAM at `3582:2F73` is SHA-256
`b4919a8c5c026e481fff3b6ad2fe93aa8b25741414afa395b9e7d8400bccf1d3`.

At that paused return, the next script PC was `0x9B71`, in buffer
`3AD6:0000` with virtual base `0x9900`. The private nine-byte instruction at
buffer offset `0x0271` was changed to DIVIDE: slot 1 is class-1 address
`0x6B18`, slot 2 is literal `3`, and the destination is class-2 word `0x9808`.
The controlled instruction's SHA-256 is
`1ea9a52942d29f0868bf67cbc9df5b1514747e16a3d71393f5701970e3d1531e`.
The active-record pointer DS:`[0x5D92]` was `449A:0008`; its byte at `+0x14`
was set to 7. Window-1 word `0x6B18` at `3A16:0030` was set to 10. The
destination at `3A96:0210` began at 3. Relative to the prior return RAM, the
controlled pre-run image differed at exactly six physical offsets:
`0x3A190`, `0x3AFD1`, `0x3AFD3`, `0x3AFD4`, `0x3AFD6`, and `0x449BC`.
Four are private script bytes; the others are the two discriminating values.
The pre-run 640 KiB RAM hash is
`0a5cfff8388302bd4caf0241c6782f3a75401ab2067b72fadda87185a3ee6f97`.

The live unit `0x25` code segment was `30CF`. Its class-1 load hook
`+0x07BC..+0x0A73` hashes
`6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451`,
matching the original 696-byte overlay span. Live load-word
`+0x0E0C..+0x0F52` hashes
`e8f999e62ee3f04b194908f90ce7c0e15b0478a2af16a3843a1a076c86c67ce5`.
The 145-byte live arithmetic handler at `3582:01A3..0233` hashes
`23819e5c20e81940bd9d098583bf52e7e810555a49553d6550bbd89fdb0473f4`.
These match the byte analysis in the [prior `0x6B18` composition](U1C_ARITH_CLASS1_RECORD_6B18.md)
and [strict parity audit](U1C_ARITH_STRICT_PARITY_GAPS.md). Before this trace,
`deja` could not acquire its local cache lock; MCP recall for this address and
arithmetic route returned no matching prior session.

## Native stops and frames

| Stop | Full 640 KiB RAM SHA-256 | Measured state |
| --- | --- | --- |
| `30CF:07DE`, before record read | `f37f9208e0fce765d31a7a50ef143ebb4890b4e8ffc8ffe9447bfd32a21820bb` | `ES:DI=449A:0008`, `AX=0x18`; next instruction reads `ES:[DI+0x14]`, whose byte is 7. Window-1 word remains 10. |
| `30CF:0E50`, load-word flag check | `5a0619543276298b59cfd0f30a91b82953e05df6eed5ae43ddc71ee4519b0e06` | `AX=7`; caller local `[BP-2]=7` and by-reference hook flag `[BP-4]=1`. `JNE +0x0E69` skips the window-1 read; its word remains 10. |
| `3582:0210`, before unsigned `DIV` | `e984ed4fc7afe0bf12a463186ff2f2b756aa59fb24164475914fe27cf733306d` | `DX:AX=0:7`; arithmetic locals `[BP-6]=7`, `[BP-4]=3`, `[BP-2]=0x9808`. Destination remains 3; script PC is `0x9B7A`. |
| `3582:0230`, after store helper | `6ae6f78ad6c8ecce9e431b2aafd061ca92fd910ae39c8145fc5a9cefaa9b8c94` | Class-2 destination `0x9808` changed `3 → 2`. Record byte remains 7 and window-1 word remains 10. |
| `3582:2F73`, far-call return target | `6ae6f78ad6c8ecce9e431b2aafd061ca92fd910ae39c8145fc5a9cefaa9b8c94` | Return reached with `SS:SP=1D16:3EE8`, `DX=2`, destination 2, script PC `0x9B7A`. |

The debugger register frames were `DS:ES:SS=14A0:449A:1D16`,
`SP:BP=3EA8:3EAC` at `30CF:07DE`; `DS:ES:SS=14A0:449A:1D16`,
`SP:BP=3EB8:3EBE` at `30CF:0E50`; and
`DS:ES:SS=14A0:449A:1D16`, `SP:BP=3ED8:3EE2` at `3582:0210`.
After the store, `DS:ES:SS=14A0:3A96:1D16` and `SP:BP=3ED8:3EE2`;
at the far return `SP:BP=3EE8:3EE8`.

The hook's `+0x07C5` sets the flag, `+0x07D5` accepts relative address
`0x18`, `+0x07DA` loads DS:`[0x5D92]`, `+0x07DE` reads `record[0x14]`, and
`+0x07E2` clears AH. The `+0x07E7` jump to `+0x0A6B` preserves the flag.
Load-word's `+0x0E4A` call returns AX=7, and `+0x0E50..+0x0E54` bypasses
its window read. The arithmetic `DIV` at `3582:0210` therefore divides 7 by
3; the store helper writes quotient 2. The same RAM hash at store completion
and far-call return reflects register/control-flow progress without a RAM
write between those stops.

## Scope

The existing C model already composes this branch **when the caller supplies
the active record**, and its synthetic `7` versus `10` test predicts the
observed stored value 2. The existing `poolrad_recovered_tests` and
`poolrad_overlay_u1c_arith_01a3_tests` both passed in a temporary build.
This trace supplies native confirmation for that one
input and path; it makes no new C change or full-handler parity claim. It does
not establish a natural saved-game route to this instruction, any other
class-1 effect case, class-1 store hooks, class-4 stores, or divide-fault
runtime behavior. Original media, debugger output, and full RAM captures stay
under ignored `resources/`.
