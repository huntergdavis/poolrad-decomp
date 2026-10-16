# Cold-start credits terminal wait transfer: bounded source gate

This source-only gate covers the terminal U17 `+0615..+0621` setup, its `PUSH CS; CALL +0000` transfer, and the separately bounded local `+0000..+004A` routine. It follows the printer sequence described in the [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md) and [first printer gate](NATIVE_COLD_START_CREDITS_FIRST_PRINTER_SOURCE_GATE.md), but does not establish that a saved native run reached this transfer, how long it waited, or whether its nested resident call wrote a display page. It is independent of the second-printer gate. A bounded `deja` search for the terminal transfer and nested wait/flush route found no indexed session match. The spans below were checked against ignored DOS v1.3 originals and retained U17-entry RAM; no emulator was started under the host's heavy-native hold.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. The 12-byte original and loaded `+0615..+0621` span is GAME.OVR `[0x006E9,0x006F5)`, SHA-256 `6b7f4a55cfca0f207072e1c2cef880c68853e22222398f7c718522d1c8dbbf66`. It sets AX to 10 at `+0615`, pushes that word at `+0618`, pushes CS at `+0619`, and near-calls local `+0000` at `+061A`; the static return is `+061D`. The four-byte `+0619..+061D` transfer, original GAME.OVR `[0x006ED,0x006F1)` and loaded RAM, has SHA-256 `f6f62b95763237ed8a196c0a21a87dfc609ab788099ed45a3e9d7eb217f10dac`. U17 then restores SP from BP, pops BP, and far-returns at `+0620` if the local routine returns. `PUSH CS` supplies the far return segment for that routine's `RETF 2` despite the near `CALL`: at its BP frame, `[BP+2]` is `+061D`, `[BP+4]` is U17 CS, and `[BP+6]` is argument 10.

The local `+0000..+004A` routine is 74 bytes. Original GAME.OVR `[0x000D4,0x0011E)` SHA-256 is `735daab8b6753b1abd4c3ae4759d94083f3076a7ac694c07df2465dead83236b`; loaded RAM SHA-256 is `c250730b8aa30dbdb084c95d9c682f2b8cb1ffbd7959edae67670912f11dd462`. It ends with `RETF 2` at `+0047..+004A`, original and loaded SHA-256 `aca87021d83b6ed907344060f599433727424acd500a7488bde4673e85f23337`. The following inline data does not belong to this routine.

| U17 offset | Decoded operation and bounded effect |
| --- | --- |
| `+0000..+0006` | Establish BP frame and reserve eight local bytes. |
| `+0006..+0011` | Far-call `RES_0709:0C3F`; store returned AX and DX as a two-word initial value. |
| `+0011..+0025` | Multiply `[BP+6]` by 100 in AX, clear DX after multiplication, add the initial two words with carry, and store the resulting two-word threshold. For this caller the argument is 10; the high word of any product is discarded before the addition. |
| `+0025..+0036` | Re-call `RES_0709:0C3F`; compare returned DX:AX with the threshold. The high-word branches use signed `JG/JL`; on equality the low word uses unsigned `JAE`. A reached threshold branches to `+003F`. |
| `+0036..+003F` | If below threshold, far-call `RES_0A40:02FD`; zero AL loops to `+0025`, nonzero AL falls through to `+003F`. |
| `+003F..+004A` | Far-call `RES_0899:0104`, then restore the BP frame and `RETF 2` to U17 `+061D`. This is the nested call in this local routine, distinct from any line-input route. |

The original and loaded far-call hashes, respectively, are `afe9a7a27fd730677e0e1ad1a28bfa2acd52e1eb09ac10378fa6fdfb33ea485a` / `f5529e40e32c082febc2310c77a1df5e53e0d1840ffe073f0dcd8b6ec5694c03` for the first `+0006` clock call (GAME.OVR `[0x000DA,0x000DF)`), the same pair for the loop `+0025` clock call (`[0x000F9,0x000FE)`), `46de73ea1af080a8a985164d0627c3fada33cc55d117cd85a3a7203f9485dab1` / `17f947122d458531a36588fa846a296f5d3c54545b4dba070b64f50377e9a074` for the `+0036` poll (`[0x0010A,0x0010F)`), and `ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356` / `d16a975ffb6c917ea41c6741692d8666e6952808ea863d6186338bb1081df8e1` for the `+003F` resident call (`[0x00113,0x00118)`).

The separate [title/credits control map](NATIVE_COLD_START_TITLE_CREDITS_CONTROL_GATE.md) calls this a wait/flush path. This gate verifies its source instructions and ABI only: the physical time unit, poll result, runtime branch, `RES_0899:0104` effect, and display write remain unobserved. It adds no C translation, full-frame parity, or game-function count.
