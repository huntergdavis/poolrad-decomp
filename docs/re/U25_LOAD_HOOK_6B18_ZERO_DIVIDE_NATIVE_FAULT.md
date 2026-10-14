# Native `0x6B18` zero-divisor fault and DOS exit path

A controlled DOS v1.3 replay used the unit `0x25` class-1 load hook at
`0x6B18` as the **divisor** of unit `0x1C` DIVIDE. The active-record byte was
zero while the distinct window-1 word was 10. Native load-word returned zero,
and `DIV` at `3582:0210` faulted into INT 0 at `131C:00DF`. The runtime wrote
error code 200 to DS:`[0x441C]` and reached `INT 21h` with `AX=0x4CC8`
(terminate with exit code `0xC8`). The class-2 destination sentinel
`0xA55A` stayed unchanged through the last captured RAM boundary. No
arithmetic store or handler return occurred before the exit call. This is
evidence for this controlled input, not a natural script-reachability or
whole-handler parity claim.

## Provenance and input

The ignored original `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
the slot-A save is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The DOSBox-X binary is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
This replay continued from the [native `0x6B18` nonzero trace](U25_LOAD_HOOK_6B18_NATIVE_RECORD.md)
at `3582:2F73`, whose 640 KiB RAM SHA-256 is
`6ae6f78ad6c8ecce9e431b2aafd061ca92fd910ae39c8145fc5a9cefaa9b8c94`.

At virtual script PC `0x9B7A`, the private nine-byte instruction in the
`3AD6:0000` script buffer was changed to DIVIDE: literal numerator 21,
class-1 divisor address `0x6B18`, class-2 destination `0x9808`. The inserted
instruction SHA-256 is
`3648e0fada85a53b1052d70c18359b752bc1dede3b35e86d3ed4e0d62e72d440`.
The active-record pointer DS:`[0x5D92]` was `449A:0008`; byte `+0x14` was
set to zero. Window-1 word `0x6B18` at `3A16:0030` remained 10. Destination
word `0x9808` at `3A96:0210` was set to sentinel `0xA55A`. Relative to the
prior return image, exactly 12 physical RAM bytes changed: nine script bytes
at `0x3AFDA..0x3AFE2`, record byte `0x449BC`, and destination bytes
`0x3AB70..0x3AB71`. Controlled pre-run RAM SHA-256 is
`85ded641d8a7507bf1a098585ffe5179a5117f52696ef78e937589ff4b01c045`.
The installed INT 0 vector was `131C:00DF`.

Live code hashes match the original spans identified in the
[class-1 composition](U1C_ARITH_CLASS1_RECORD_6B18.md) and
[strict parity audit](U1C_ARITH_STRICT_PARITY_GAPS.md): 696-byte load hook
`30CF:07BC..0A73` SHA-256
`6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451`;
145-byte arithmetic handler `3582:01A3..0233` SHA-256
`23819e5c20e81940bd9d098583bf52e7e810555a49553d6550bbd89fdb0473f4`;
64-byte INT 0 entry `131C:00DF..011E` SHA-256
`2e0d02cf1da6ef322f33ac78239cb6e20711c9be3ce83e29563a2a1bad2c2a5e`.
`deja` could not acquire its read-only cache lock; MCP recall for this
zero-divisor route found no matching prior session.

## Native stops

| Stop | Full 640 KiB RAM SHA-256 | Observation |
| --- | --- | --- |
| `30CF:0E50`, load-word flag check | `5250dbfb46997d19a0782b502764d46ab6476dbf82c5ed8a194fc16baaec020f` | `AX=0`, by-reference flag `[BP-4]=1`, hook result `[BP-2]=0`; window-1 word remains 10, destination `0xA55A`. `JNE +0x0E69` skips the window read. |
| `3582:0210`, before `DIV word [BP-4]` | `3d42f6987a436da360b0e39fd8cd8bd187fa3baf72f79f8faa938485bae53a67` | `DX:AX=0:21`; locals `[BP-6]=21`, `[BP-4]=0`, `[BP-2]=0x9808`; destination `0xA55A`, script PC `0x9B83`. |
| `131C:00DF`, INT 0 entry | `960674319ddeeec0efc7340954d8c5e0f7a432ff70d9e7a1068c97626253d459` | Interrupt stack `SS:SP=1D16:3ED2` contains return `3582:0210` and flags `0x7246`; compared with pre-`DIV` RAM, only the six pushed frame bytes changed. Destination still `0xA55A`. |
| `131C:00F6`, after error store | `da4e679125cc0ca148ec39f912be10cda8389d7c51e643a5fd7fdd875cfb1b63` | `+0x00DF` loaded AX=`0x00C8`; `+0x00F3` stored it at DS:`[0x441C]`. Error word is 200; destination still `0xA55A`. Compared with INT 0 entry RAM, only the error word's low byte changed. |
| `131C:01A1`, before exit setup | `db8879a8efe3d787ed350380e140dadca07d5eba1bb65caa2adfb1bff43e72a1` | `AX=0x00C8`, DS:`[0x441C]=200`, destination `0xA55A`, script PC `0x9B83`; the runtime next executes `MOV AH,4Ch`. |
| `131C:01A3`, DOS exit call | No RAM dump | `INT 21h` is the next instruction with `AX=0x4CC8`, giving DOS function `4Ch` and exit code `0xC8`. |

The pre-`DIV` frame was `DS:ES:SS=14A0:449A:1D16` and
`SP:BP=3ED8:3EE2`; at INT 0 entry it was
`DS:ES:SS=14A0:449A:1D16`, `SP:BP=3ED2:3EE2`. At the exit setup,
`DS:ES:SS=14A0:14A0:1D16` and `SP:BP=3ED6:3EE2`. The INT 0 frame's saved
IP is the **faulting** `0x0210`, not `0x0213`; the divide never retired.
Breakpoints at the arithmetic post-store `3582:0230` and far return
`3582:2F73` remained armed and were not hit on the path to `131C:01A3`.

## Scope

The existing C arithmetic model returns `DIVIDE_ERROR` and records 200 for a
zero divisor without entering the DOS runtime. This native observation
confirms no destination store before the fault and measures the additional
INT 0 error and DOS exit path. Execution was stopped at the `INT 21h`
instruction, so the host DOS shell state after that call was not observed.
The inserted script's natural reachability remains unresolved. No C change,
full-C credit, or broader arithmetic parity follows. Original media, script
bytes, debugger output, and full RAM captures stay under ignored `resources/`.
