# Saved slot-A DELAY caller and replay-stability boundary

The hash-pinned native A40-return capture extends the proved DELAY stack-read
boundary through the U1C caller's `3582:3151` continuation. That continuation
jumps to `3582:316F`, executes `POP BP; RETF`, and returns to `3582:31A8` with
`SP=3EEE`. Its two stack reads are above the 17 IRQ-written residual bytes.
The first later read or overwrite of any residual byte remains **unmeasured**.
Three fresh, instrumented slot-A Search→one-Up replays did not reach DELAY;
their identical pre-Up rendered frames led to three different post-Up frames.
These are route nonhits, not evidence of no residual read.

Before debugging, bounded `deja "Poolrad saved slot A DELAY A40 return 3582
3151 IRQ residual stack first later read overwrite replay stability
watchpoint"` timed out without a match. The merged
[IRQ writer](SLOT_A_DELAY_IRQ_STACK_WRITER.md),
[two-epilogue boundary](SLOT_A_DELAY_RESIDUAL_READ_BOUNDARY.md), and
[natural DELAY reach](SLOT_A_NATIVE_DELAY_TIMER_REACH.md) supplied the prior
hash-pinned A40-return capture, residual offsets, and successful call route;
the caller frame and original-byte identity below were checked independently.

## Third reached epilogue

The ignored DOS v1.3 inputs have SHA-256 START.EXE
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and SAVGAMA.DAT
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The successful p9-102 A40-return RAM capture is 655,360 bytes, SHA-256
`ea096da21f27bceddf4f35fe5d9721b81eace9bf0aa853338e04297a7b35c92c`.
The earlier live replay stopped at `3582:3151`; this check decodes its loaded
continuation and saved return frame without substituting a new live trace.

| Loaded instruction span | GAME.OVR source | Length | Loaded/source SHA-256 | Effect |
| --- | --- | ---: | --- | --- |
| `3582:3151..3153` | `[0x3E97,0x3E99)` | 2 | `e4a59458100a8c550ee659500595c93fa9ac0f512393bf7d562d55c5e619b417` | jump to `316F`; no memory read |
| `3582:316F..3171` | `[0x3EB5,0x3EB7)` | 2 | `05303d46935f1d6bc7f492470521a209a51050b261d452a788f042866b5d72cd` | `POP BP; RETF` |

At `3582:3151`, the prior return capture has `SS:SP=1D16:3EE8` and stack
words `SS:3EE8=3EEE`, `SS:3EEA=31A8`, `SS:3EEC=3582`. Thus `POP BP` reads
`SS:[3EE8,3EEA)`, `RETF` reads `SS:[3EEA,3EEE)`, and execution resumes at
`3582:31A8` with `SP=3EEE`. Together with the two previously proved
epilogues, all six read ranges lie above residual `SS:[3EC2,3ECB)` and
`SS:[3ECC,3ED4)`. The loaded and original instruction spans match exactly.
The next caller path from `3582:31A8` is not traced here.

## Bounded replay-stability correction

An ignored private DOSBox-X 2026.01.02 normal-core build, SHA-256
`c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`,
instruments normal-core operand reads/writes and CPU stack helpers at physical
`[0x21022,0x2102B)` and `[0x2102C,0x21034)`. It arms the watch at A40 return
`0F2D:0DD1`; a `3582:271F` breakpoint checks whether DELAY is reached.
The private build completes the title and slot-A load, unlike the earlier
`-O0` watch build. This instrumentation has not been proved complete for every
emulator helper-side memory access, so a missing watch event alone cannot
establish no-read.

Each fresh run loaded the same pinned save at `(15,4,N) 10:50`, sent one
Search, returned to the same 640×400 pre-Up frame, then sent exactly one Up.
The RGB SHA-256 of every pre-Up frame was
`68d60f9ec4bf35e051befe16941ddbe0ca4cabb3c6187956f34c5cc4c900fb21`.
The breakpoint was armed before slot load and did not stop at `3582:271F`
in any of these three attempts. The three post-Up 640×400 RGB hashes were:

| Ignored run | Post-Up RGB SHA-256 | Bounded observation |
| --- | --- | --- |
| p9-107 | `20f291607397a1a9255f612e23ebd9cc13ad549a791677c451dc35ff53adce79` | ordinary movement frame at `(14,4,N) 11:00` |
| p9-108 | `11083c86e4c2fa582ab8a3d55f194915fa3ec2898b49e2fb10c9bc2c89f82d33` | distinct encounter frame |
| p9-109 | `a454eb3186ef2d3e05b49f5acf784284e5a6d92c563613b9bf03d4cc3104b59a` | another distinct encounter frame |

The identical rendered pre-Up state and input sequence therefore do not fix
the later native branch. The successful p9-102 replay reached ECL PCs `9C27`
and `9C28`; these three attempts did not reach the DELAY entry. The source of
the divergence was not isolated. No post-Up UI C parity, first residual read,
first residual overwrite, full-RAM parity, or game-function count follows
from these nonhits.

## Reproduce the bounded audit

Keep the DOS files, screenshots, RAM dumps, private emulator source/binary,
and debugger output under ignored `resources/`. The public audit verifies
the original/loaded span hashes, three return frames, and disjoint read
ranges without emitting game bytes:

```sh
python3 tools/replay/slot_a_delay_residual_read_boundary_audit.py \
  resources/oracle-runs/slot-a-delay-stack-writer-p9-102
python3 tests/replay/test_slot_a_delay_residual_read_boundary_audit.py
```

To repeat a native attempt, prepare a fresh run with
`tools/oracle/prepare_run.sh`, use the pinned private watch build under an
isolated X display, arm `BP 3582:271F` and `BP 3582:3151` before loading
slot A, then send Search and exactly one Up. Preserve the raw captures only
under ignored `resources/oracle-runs/`. A future successful reach must log
per-byte first read and first overwrite after `3582:31A8`, including guest
CS:IP and caller, before the C timer stack boundary can claim liveness parity.
