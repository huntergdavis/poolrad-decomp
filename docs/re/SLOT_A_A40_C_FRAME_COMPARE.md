# Saved slot-A A40 C frame comparison

This first gate compares the existing bounded C body
`pr_a40_delay_029c_run` with the native entry and return of resident
`A40:+029C` reached in the private slot-A Search then Up replay. The C body
completes all 900 units on the reached constant-byte/calibration inputs. Its
instruction-derived return frame matches **all 11 CPU fields recorded at the
native return**. After supplying the **observed** 17-tick external BIOS timer
advance, the RAM comparison still differs in 17 lower-stack bytes whose
first writer was not observed. SI, DI, and FLAGS were not recorded at the native return, so this is
not full CPU or C/native route parity.

Before this work, `deja "Poolrad A40 029C calibrated external BIOS timer
callback native entry return frame full RAM CPU parity wrapper 0709 0DBE U1C
271F"` timed out at 20 seconds without a hit. The original-byte decode and
bounded model in [RES_A40_DELAY_029C.md](RES_A40_DELAY_029C.md) and the
reached call/timer evidence in PR #437 supplied the inputs; this comparison
checks the actual C body and private snapshots independently. The printer
owner's `RES_0709:+0626/+0768` files are outside this scope.

## Locked inputs and execution

The private input hashes are START.EXE
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and SAVGAMA.DAT
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The `1264:+029C..+02C9` loaded 45-byte body/helper SHA-256 is
`d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb`.
The ignored 640-KiB entry RAM hashes
`c088ddf8cbc7cf3321a3afdf8a79e012be5cb996ff19a5d2c0a4fe46fd8a7162`;
return RAM hashes
`7ff60cfe5abe05bf78fc0d0bf78277c792c4bd22bb024a063f153edd5215e84a`.

At `1264:029C`, `SS:SP=1D16:3ED6` holds return `0F2D:0DD1` and word
argument `900`; `DS:84FC=269`, BIOS `0040:0000=248`, and BIOS clock
`0040:006C=1,090,044`. The C probe compiles the existing A40 source under
`-std=c11 -Wall -Wextra -Wpedantic -Werror`, invokes it through constant
read callbacks, and obtains 900 completed units, 242,101 BIOS-byte reads,
900 calibration reads, and 242,100 spin calls. These C callback counts are
not native loop counts: individual DOS loop iterations were not measured.

The frame adapter applies the instruction-verified near `CALL +02BD` write
of return IP `+02B7` at `SS:3ED4`; it reads the A40 far return words from the
entry stack and advances SP by six for `RETF 2`. The external timer callback
receives a tick delta separately from the C model. Its private comparison
supplies the native measured delta `17`, making timer state an explicit input,
not a C timing prediction or a forced party/result tuple.

## Exact differences

| Comparison against native A40 return | Differing physical RAM intervals | Count |
| --- | --- | ---: |
| C frame with no external timer advance | `[0x46C,0x46E)`, `[0x21022,0x2102B)`, `[0x2102C,0x21034)` | 19 |
| C frame with observed 17-tick timer callback | `[0x21022,0x2102B)`, `[0x2102C,0x21034)` | 17 |

The 17 remaining bytes are below the A40 near-call word at physical
`[0x21034,0x21036)`. No instruction in the bounded A40 body writes them.
Interrupt activity is plausible, but no first writer or IRQ frame was
captured, so their cause remains unresolved. The two tick bytes at
`[0x46C,0x46E)` are supplied by the
external callback; the C body does not read this clock DWORD.

The instruction-derived C return values match all native recorded fields:
`AX=00F8`, `BX=CX=DX=0`, `BP=SP=3EDC`, `CS:IP=0F2D:0DD1`,
`DS=14A0`, `ES=0040`, `SS=1D16`. A40 sets DI to zero and does not change SI,
but the native return recording lacks SI, DI, and FLAGS. No claim is made for
those three native comparisons. The reached wrapper/U1C return composition
is a separate follow-on gate.

## Reproduce

The synthetic test needs no game files:

```sh
python3 tests/replay/test_slot_a_a40_frame_compare.py
```

The private audit accepts only the ignored hash-pinned run directory and
prints numeric evidence, never original bytes or capture contents:

```sh
python3 tools/replay/slot_a_a40_frame_compare.py \
  resources/oracle-runs/slot-a-native-delay-reach-p9-099
```

The synthetic test covers C callback counts, a completed far return frame,
the lasting near-call stack write, an externally supplied timer advance,
timer wrap, and the first reached operation-budget stop. Full repository
CTest is run separately; no CMake target or game-function count changes here.
