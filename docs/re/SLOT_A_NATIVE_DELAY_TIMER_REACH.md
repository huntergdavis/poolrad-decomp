# Saved slot-A native DELAY reach and timer boundary

A fresh DOS v1.3 replay reached ECL PC `0x9C27` (`U1C:+0817`), then PC
`0x9C28` (`U1C:+271F`), resident `0709:+0DBE`, and resident `A40:+029C`.
The reached A40 call returned after **17 guest BIOS clock ticks**. This is
native call and timing evidence for this one replay; it is not a C/native
end-to-end parity or playable-function count claim.

Before debugging, `deja "Poolrad slot A natural encounter APPROACH ECL 9C27
DELAY 9C28 U1C 271F RES 0709 A40 native breakpoint RNG menu route"` timed
out after 20 seconds without a recall hit. The prior static decoding and
bounded C model in [U1C_APPROACH_0817.md](U1C_APPROACH_0817.md),
[U1C_DELAY_271F.md](U1C_DELAY_271F.md),
[RES_0709_DELAY_WRAPPER_0DBE.md](RES_0709_DELAY_WRAPPER_0DBE.md), and
[RES_A40_DELAY_029C.md](RES_A40_DELAY_029C.md) supplied the candidate chain;
the breakpoints, loaded spans, frames, and state below were checked in this
separate run. The prior A40 note records a different exactly-one-Up nonhit.
Both observations are bounded to their own runs.

## Input and loaded-code identity

The ignored input copy is START.EXE, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
and SAVGAMA.DAT, SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X 2026.01.02 executable hashes
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The five code spans below were hashed from the **A40-entry RAM** snapshot,
independent of the input-file hashes:

| Loaded span | Bytes | Loaded SHA-256 | Original source proof |
| --- | ---: | --- | --- |
| `3582:+0817` APPROACH | 58 | `b18c884571cedd3093b2db99ab977f8f6d859d0bba838d0c6de65cbab72c1b98` | GAME `[0x155D,0x1597)`, `43796e27c5f066c72c390f219381f3da43fae497dcc79f557a028a3a6610f002` |
| `3582:+271F` DELAY | 14 | `ecb6e7a981de47539c4809d0338832f0c107bf96a9e83d65e2847bf5a2f14ec2` | GAME `[0x3465,0x3473)`, `aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1` |
| `0F2D:+0DBE` wrapper | 21 | `7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be` | expanded START `[0x7E4E,0x7E63)`, `a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d` |
| `1264:+029C` A40 body | 33 | `7ee3c156a50e06d1f23e4da738158d3cb48179223d3135c55b7976eae426e75e` | expanded START `[0xA69C,0xA6BD)`, same hash |
| `1264:+02BD` A40 spin helper | 12 | `d2df9bf05a361b9f27a4fe2591ea9938c31fb9d319ea1640d73bfbc52d93a2ea` | expanded START `[0xA6BD,0xA6C9)`, same hash |

## Reached call path and frames

The private debugger was started on a controlling PTY. The slot-A frame
settled at party coordinates/facing `(15,4,N)` and clock `10:50`. One Search
input returned to the prompt without a watched stop; the next input, one Up,
reached the following armed breakpoints naturally. No seed, save, guest RAM,
code, result register, or breakpoint return value was edited.

| Stop | ECL PC at DS:`49ED` | Far caller return and argument | Selected state |
| --- | ---: | --- | --- |
| `3582:0817` | `9C27` | stack `1D16:3EE4` → `3582:2FB5` | DS=`14A0`, party `(14,4,6)` |
| `3582:271F` | `9C28` | stack `1D16:3EE4` → `3582:3151` | DS=`14A0`, factor DS:`49E2`=`4` |
| `0F2D:0DBE` | `9C29` | stack `1D16:3EDE` → `3582:272B` | factor `4` |
| `1264:029C` | `9C29` | stack `1D16:3ED6` → `0F2D:0DD1`; argument at `3EDA`=`900` | DS:`84FC`=`269`, BIOS `0040:0000`=`248` |
| `0F2D:0DD1` | `9C29` | stack `1D16:3EDC` → `3582:272B` | A40 far return reached |
| `3582:272B` | `9C29` | stack `1D16:3EE2` → `3582:3151` | wrapper far return reached |
| `3582:3151` | `9C29` | U1C caller resumes | U1C far return reached |

At the two post-callee stops, the listed current SP points to saved BP;
`POP BP` exposes the far return words at SP+2.

At `1264:02AD`, `ES=0040`, `DI=0`, and the reached instruction reads the
baseline byte at `ES:[DI]`, `248`. At `1264:02B0`, the next instruction reads
the calibration word `DS:84FC`, `269`. The previously byte-verified A40 body
then calls near helper `+02BD` and loops. Individual loop iterations were
not breakpoint-counted in this run.

## Measured timer and memory boundary

The BIOS tick DWORD at `0040:006C` was `1,090,044` at A40 entry and
`1,090,061` on its return: **17 ticks** elapsed. A40-entry to A40-return
RAM differs in exactly 21 bytes: physical `[0x46C,0x46E)` (two BIOS tick
bytes), `[0x21022,0x2102B)` (nine guest-stack bytes), and
`[0x2102C,0x21036)` (ten guest-stack bytes). The A40-return and U1C-return
RAM snapshots are byte-identical. DS:`84FC` and BIOS `0040:0000` remain
`269` and `248` at A40 return. Host-wall timing of a debugger run is not
used as the game delay duration.

The existing synthetic A40 test covers the 900/269 constant-byte case and
the merged C timing boundary. This native measurement neither counts actual
BIOS-read iterations nor establishes C wall-time parity. The next integration
gate is to run the complete C route with the reached native state and compare
its explicitly modelled external timer behavior.

## Private audit and replay procedure

All RAM dumps, save files, emulator output, and screenshots stay ignored in
`resources/oracle-runs/slot-a-native-delay-reach-p9-099/`. The tracked audit
requires their exact hashes and reports only addresses and numeric values:

```sh
python3 tools/replay/slot_a_native_delay_timer_audit.py \
  resources/oracle-runs/slot-a-native-delay-reach-p9-099
```

For another native attempt, prepare a private run with
`tools/oracle/prepare_run.sh`, launch the hash-pinned heavy DOSBox-X build
under `xvfb-run` with a controlling PTY, and arm breakpoints at `3582:0817`,
`3582:271F`, `0F2D:0DBE`, and `1264:029C` before loading slot A. At each
stop record `EV AX BX CX DX CS IP DS ES SS SP BP` and
`MEMDUMPBIN 0000 0000 A0000`; also arm returns `0F2D:0DD1`,
`3582:272B`, and `3582:3151`, and instruction stops `1264:02AD` and
`1264:02B0`. Resume and send Search then Up after the saved frame settles.
The native RNG and DOS clock can change whether a fresh attempt reaches this
branch; the hash-locked private snapshots above are the evidence for this
observed route.
