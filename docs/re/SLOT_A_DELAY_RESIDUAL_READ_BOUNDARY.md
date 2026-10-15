# Saved slot-A DELAY residual stack read boundary

The reached A40 return frame proves that the next four **game** instructions
read only stack words above the 17 IRQ-written residual bytes. The resident
wrapper resumes at `0F2D:0DD1` with `POP BP; RETF`; U1C resumes at
`3582:272B` with the same two instructions. Execution then reaches the U1C
caller at `3582:3151`. The first later game read of each residual byte is
**unmeasured**. This is a bounded epilogue result, not a no-read result for
the whole route or full-RAM C/native parity.

Before this check, bounded `deja "Poolrad slot A DELAY A40 return stack 3EC2
3ED3 read before overwrite 0F2D 0DD1 3582 272B 3151"` and a second
residual-read query each timed out after 20 seconds without a match. The
merged [IRQ writer audit](SLOT_A_DELAY_IRQ_STACK_WRITER.md) supplied the
hash-locked RAM capture and the 17-byte interval. The earlier
[natural DELAY reach](SLOT_A_NATIVE_DELAY_TIMER_REACH.md),
[wrapper](RES_0709_DELAY_WRAPPER_0DBE.md), and
[U1C DELAY](U1C_DELAY_271F.md) notes supplied the candidate return chain;
the frame and loaded continuations below were checked again independently.

## Hash-locked frame and exact read range

Ignored DOS v1.3 inputs: START.EXE SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and SAVGAMA.DAT SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The reached `0F2D:0DD1` private RAM capture is 655,360 bytes, SHA-256
`ea096da21f27bceddf4f35fe5d9721b81eace9bf0aa853338e04297a7b35c92c`.
Its loaded two-byte spans at `0F2D:0DD1` and `3582:272B` each hash to
`05303d46935f1d6bc7f492470521a209a51050b261d452a788f042866b5d72cd`.
The full resident and U1C spans were independently hash-locked in the
linked notes. No original bytes or RAM captures are tracked.

At A40 return, `SS:SP=1D16:3EDC`. The residual consists of
`SS:[3EC2,3ECB)` and `SS:[3ECC,3ED4)` (17 bytes), physical
`[0x21022,0x2102B)` and `[0x2102C,0x21034)`. The six stack words above it
are numeric return-frame evidence:

| SS offset | Value | Reached use |
| --- | ---: | --- |
| `3EDC` | `3EE2` | wrapper saved BP, read by `POP BP` |
| `3EDE` | `272B` | wrapper far-return IP |
| `3EE0` | `3582` | wrapper far-return CS |
| `3EE2` | `3EE8` | U1C saved BP, read by `POP BP` |
| `3EE4` | `3151` | U1C far-return IP |
| `3EE6` | `3582` | U1C far-return CS |

Thus the wrapper reads `SS:[3EDC,3EE2)` and leaves `SP=3EE2`; U1C reads
`SS:[3EE2,3EE8)` and leaves `SP=3EE8`. Neither intersects the residual.
The exact next boundary is the caller continuation `3582:3151`. The
merged natural-reach note observed byte-identical A40-return and U1C-return
RAM in its separate run; unchanged RAM alone cannot rule out reads.

The later [caller and replay-stability audit](SLOT_A_DELAY_RESIDUAL_CALLER_AND_REPLAY.md)
extends this bounded read proof through `3582:3151` to `3582:31A8`; the
first subsequent residual access is still unmeasured.

## Attempted extension and open gate

Two fresh private, hash-pinned Search→one-Up replays with a confirmed
`3582:271F` breakpoint settled at `(14,4,N) 11:00` without hitting DELAY.
They show route nonreach for those attempts, not absence of residual reads.
A private DOSBox-X normal-core read/write watch was built for the 17 physical
addresses and armed at `0F2D:0DD1`; its `-O0` build, SHA-256
`5841fd8e403d09cec12c1fce046284fb901568acc782a08d53341d67896a12b8`,
did not finish loading the game within the bounded replay window. It yielded
no watched access and supplies no read conclusion. All emulator changes,
screens, save inputs, and traces remain under ignored `resources/`.

To close the gate, reach `0F2D:0DD1` again on the pinned save and log guest
reads and writes of both residual intervals from that return through the
later caller route, with CS:IP and a per-byte first-read/first-overwrite
order. A value-change `BPM` cannot establish a read or detect a value
restored before polling. The C timer callback must not mask these 17 bytes
as equal or claim full-route RAM parity until that later access behavior is
measured or a separately proved stack-liveness bound excludes it.

Run the numeric audit only against the ignored writer replay:

```sh
python3 tools/replay/slot_a_delay_residual_read_boundary_audit.py \
  resources/oracle-runs/slot-a-delay-stack-writer-p9-102
python3 tests/replay/test_slot_a_delay_residual_read_boundary_audit.py
```
