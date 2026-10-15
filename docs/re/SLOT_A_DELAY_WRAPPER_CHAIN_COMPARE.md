# Saved slot-A DELAY wrapper-return C composition

This separate follow-on gate starts from the native `U1C:+271F` DELAY entry
on the saved slot-A Search then Up route. It composes the existing bounded C
models of `U1C:+271F`, resident `0709:+0DBE`, and resident `A40:+029C`, plus
their byte-verified far-call and return frames. The composed C route reaches
ECL PC `0x9C29` and matches all 11 recorded CPU fields at both the wrapper
and U1C returns. Supplying the **observed** 17-tick external BIOS timer
advance leaves 17 lower guest-stack bytes different from native. Their first
writer was not captured. SI, DI, and FLAGS were not recorded at the native
returns. This is a bounded return-path comparison, with no end-to-end C/native
parity or game-function count claim.

Every byte of captured physical `[0x00000,0xA0000)` RAM is compared. The
native dump does not include `[0xA0000,0x100000)`; the C wrapper's one-MiB
RAM requirement is satisfied by padding that uncaptured range. No comparison
is claimed for the upper 384 KiB.

Before the follow-on work, `deja "Poolrad RES 0709 0DBE U1C 271F native
wrapper return C frame composition A40 timer stack"` timed out after 20
seconds without a hit. The original-byte semantics in
[U1C_DELAY_271F.md](U1C_DELAY_271F.md),
[RES_0709_DELAY_WRAPPER_0DBE.md](RES_0709_DELAY_WRAPPER_0DBE.md), and
[RES_A40_DELAY_029C.md](RES_A40_DELAY_029C.md), plus PR #437's reached native
frames and PR #444's isolated A40 comparison, supplied the candidate route.
This probe independently executes the three merged C models from the earlier
native DELAY-entry frame. It does not edit printer `RES_0709:+0626/+0768`.

## Hash-locked route

The ignored DOS v1.3 inputs are START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and SAVGAMA.DAT SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private 640-KiB DELAY-entry RAM hashes
`007c1679b4899123e0b5ad29a274534befbcd8dc76e551c5faac6f64e79660b4`;
wrapper-entry RAM hashes
`1675857cbf3c0a2e729d0fc134ae7799f02e5290946660cabdcf2015835c84af`;
A40-entry RAM hashes
`c088ddf8cbc7cf3321a3afdf8a79e012be5cb996ff19a5d2c0a4fe46fd8a7162`;
U1C-return RAM hashes
`7ff60cfe5abe05bf78fc0d0bf78277c792c4bd22bb024a063f153edd5215e84a`.

The audit separately verifies these loaded spans in the A40-entry RAM:

| Loaded span | Length | SHA-256 |
| --- | ---: | --- |
| `3582:+271F` U1C DELAY | 14 | `ecb6e7a981de47539c4809d0338832f0c107bf96a9e83d65e2847bf5a2f14ec2` |
| `0F2D:+0DBE` wrapper | 21 | `7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be` |
| `1264:+029C` A40 body | 33 | `7ee3c156a50e06d1f23e4da738158d3cb48179223d3135c55b7976eae426e75e` |
| `1264:+02BD` A40 helper | 12 | `d2df9bf05a361b9f27a4fe2591ea9938c31fb9d319ea1640d73bfbc52d93a2ea` |

The native DELAY-entry → wrapper-entry RAM diff is exactly three bytes:
`[0x193ED,0x193EE)` for the ECL PC advance and `[0x2103E,0x21040)` in the
guest stack. Wrapper-entry → A40-entry changes eight stack bytes at
`[0x21036,0x2103E)`. No other RAM byte changes before the A40 call in these
captured stages. The C composition writes the PC before the wrapper callback,
pushes the far return words in original order, and lets the merged wrapper
model construct and unwind the A40 frame.

## Return comparison and boundary

The C composition reads factor `4` at DS:`49E2`, pushes A40 argument `900`,
and executes the merged A40 body against the captured BIOS byte `248` and
DS:`84FC` word `269`. Its **C callback counts** are 900 completed units,
242,101 BIOS reads, 900 calibration reads, and 242,100 spin calls. These
counts were not measured in DOS. The external timer callback receives the
native observed 17-tick delta; it is not a C duration prediction.

| C comparison against native U1C-return RAM | Differing physical intervals | Count |
| --- | --- | ---: |
| No external timer advance | `[0x46C,0x46E)`, `[0x21022,0x2102B)`, `[0x2102C,0x21034)` | 19 |
| Observed 17-tick timer supplied | `[0x21022,0x2102B)`, `[0x2102C,0x21034)` | 17 |

The C wrapper-return frame matches the recorded `CS:IP=3582:272B`,
`BP=SP=3EE2`, `AX=00F8`, `BX=CX=DX=0`, `DS=14A0`, `ES=0040`,
`SS=1D16`. The U1C epilogue then matches recorded
`CS:IP=3582:3151`, `BP=SP=3EE8`, with the same recorded general and segment
registers; DS:`49ED=9C29`. The native recording omitted SI, DI, and FLAGS,
so those comparisons remain open.

The 17 remaining bytes lie below A40's near-call return word at
`[0x21034,0x21036)`. The bounded U1C, wrapper, and A40 instructions do not
write those lower addresses. The first writer and any interrupt frame are
unobserved, so the composition stops at that effect instead of copying the
native return bytes into the C frame.

## Reproduce

The standalone shim compiles the three existing C sources under
`-std=c11 -Wall -Wextra -Wpedantic -Werror`. The focused synthetic test
checks PC advance, every reached far-call frame word, both return frames,
the external timer advance, and an invalid-entry stop. No game files are
needed:

```sh
python3 tests/replay/test_slot_a_delay_wrapper_chain_compare.py
```

The private comparison requires the ignored hash-pinned run directory and
prints only addresses, hashes' validity, counts, and numeric differences:

```sh
python3 tools/replay/slot_a_delay_wrapper_chain_compare.py \
  resources/oracle-runs/slot-a-native-delay-reach-p9-099
```

The full strict CMake/CTest suite and repository public-hygiene checks are
separate PR gates. This branch does not change CMake or the strict C count.
