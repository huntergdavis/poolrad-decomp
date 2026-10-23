# The game's random generator (Turbo Pascal `Random`)

A reference for body evidence that consumes random draws. Nothing here is a credit claim.

## Identity

**One generator: the Turbo Pascal runtime's `Random`, in the resident SYSTEM segment.** "0AF8" is the segment label
the hybrid dispatch and other docs use.

| Piece | Where | Effect |
| --- | --- | --- |
| `RandSeed` | `DS:4428..442B` (32-bit) | the whole generator state |
| seed step `+120F..+1244` | START.EXE file `[0xBE85,0xBEBB)`, 54 bytes / 22 instructions | `seed := seed * 08088405h + 1`. The multiplier is built from the word `8405h` at `CS:+1245` plus shifts. `RandSeed` is written back low word first, then high word |
| `Random(n)` wrapper `+11C0` | START.EXE from file `0xBE36` | calls the step, then returns (high word of the new seed) mod n via `xchg dx,ax; div bx`. `n = 0` returns 0 |
| random bits `+11D6` | SYSTEM | the step, used for bit draws |
| `Randomize` `+1247` | SYSTEM | `mov ah,2Ch; int 21h` (DOS get time) into `RandSeed` |

**Sources:**
- [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md): the runtime contract, the multiplier and `Randomize`;
- [SYSTEM_SEED_FULLSTATE_120F.md](SYSTEM_SEED_FULLSTATE_120F.md): the step's file span;
- [OVL_006F_0EC4_CPU.md](OVL_006F_0EC4_CPU.md): five draws from `RandSeed` at `DS:[4428]/[442A]`;
- [U2F_PARTY_PICK_0131.md](U2F_PARTY_PICK_0131.md): the generator bytes `0xBE36..0xBEBC`.

The step's first 22 instructions were re-decoded from START.EXE for this note, from `mov ax,[4428]` through `ret` at
`+1244`.

## Seeding

**`Randomize` has one call site in GAME.OVR: unit `0x36` `+0602` (file `0xEBD5`),** per SLOT_A_PARTY_INPUT.md.

- It runs at start-up, so the seed is the DOS clock (hours, minutes, seconds, hundredths) at that moment.
- After that only draws advance it.
- The seed is not in a saved game and cannot be derived from one.

## Why this matters for the hybrid routes

Route 3 (combat, [../HYBRID_GLUE_BATCH7.md](../HYBRID_GLUE_BATCH7.md)) differs from run to run because of this seed.
Pinning it reproduces a run only if every draw is tied to a game event, not to wall-clock waiting. That was measured as
stage 1 of the seed-pin price (PM i21894 and i21897, VP i21896); the result is below.

## Stage 1 measurement (log-only, 2026-10-23)

**Method.** The hybrid observer (in `tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch`, `pr_hybrid_observer.h`) gained a `DRAW_SYS_120F` registry row. The row is appended to a copy of the registry: ID `DRAW_SYS_120F`, IP `120F`, the 24 signature bytes `a128448b1e2a448bc82ef7264512d1e1d1e1d1e102e903d1`, mask all `ff`.
Those are the step's first 24 bytes, from file `0xBE85`. At every arrival the observer
logs the instruction count, the wall-clock ms and `RandSeed` before the step, to `<run>.draws`,
uncapped and flushed.

`tools/hybrid/walk_driver.py` stamps every key and gate with absolute ms. `WALK_IDLE_S=60` holds a 60 s no-key window at
the main menu and another on the area prompt at 14,4.

**Runs.** Two observer-only route-3 runs, `hybrid-seed1-a` and `-b` (build `f3ee6c53`). Both completed with driver_rc
0.

| Draws (`+120F` arrivals) | run a | run b |
| --- | --- | --- |
| total | 31,592 | 31,903 |
| start-up, before the save loads | 30,532 | 30,532 |
| **(a) idle at the main menu, 60 s** | **0** | **0** |
| **(a) idle at the area prompt 14,4, 60 s** | **0** | **0** |
| **(b) the walk: the save-load step 15,4 -> 14,4** | **1** | **1** |
| **(b) the walk: the seek steps up to the encounter** | **17** (26 steps) | **50** (92 steps) |
| **(c) from the encounter screen to the first decision key** | **5** | **5** |
| after the first decision: the fight and its aftermath | 1,037 | 1,315 |

**Per step, in both runs:** every step onto 14,4 draws once, every step back onto 15,4 draws 0, and the step that
finds the encounter draws 5. The run-b sequence is `0,1,0,1,...,0,1,0,5` over 92 steps. The seed differed between the
runs, as `Randomize` predicts: `C83DAA44` and `CC295420` before the load.

**What the counts say:**
- No draw happens while the game waits for a key: 0 in four 60 s windows.
- Every walk draw coincides with a step event. The number of draws per step is the same in both runs.
- Start-up drew exactly 30,532 times in both runs.

**Limit.** Draws in (b) were attributed by wall-clock window. The encounter screen is recognised a few seconds after
it appears, so the 5 draws of the encounter step may include its setup. **Under the VP's rule (i21897), (b) is
non-zero**; the decision belongs to the VP and the PM.

## Stage 2: the seed pin (2026-10-23, PM i22231, VP i22230)

**Mechanism.** This is a logged test-harness poke, off by default.
- **Observer:** `pr_hybrid_observer.h` reads `PR_HYBRID_SEED_PIN=<hex>`. At the first RandSeed step (`DRAW_SYS_120F`)
  after the driver has created `<run>/observer.pin`, it writes the value to `DS:4428..442B` before the step executes,
  and logs `# PINNED` in `<run>.draws`.
- **Driver:** `walk_driver.py` creates `observer.pin` at `SAVE LOADED`.
- **Registry:** the `DRAW_SYS_120F` row is now part of the registry every run uses, so every run logs its draws.

**Runs.** Build `a0e1c66f`, all observer-only, all with pin `12345678`. Each pin landed at draw 30,533.

| Pair | Pre-pin draws | Post-pin RandSeed sequence | Keys sent | Final frame |
| --- | --- | --- | --- | --- |
| walk `hybrid-pin2-walk-a` vs `-b` | 30,532 / 30,532, seed sequences **different** | 1 / 1, **identical** | 5 / 5, identical | **0 px** apart, and 0 px vs the reference walk frame |
| route 3 `hybrid-pin2-r3-a` vs `-b` | 30,532 / 30,532, seed sequences **different** | 1,447 / 1,447, **identical** | 132 / 132, identical | **0 px** apart |

**What it shows:**
- Start-up draws the same number of times in every run, from a different clock seed: the same counts, different
  values. This is the benign cause behind the walk run that met an encounter on the 14,4 step.
- Once pinned at the save-load point, the walk and the whole combat route reproduce draw for draw and pixel for pixel.
- With a pin, route 3 can carry a pixel bar like the walk. That bar is not adopted here; the ruling is the PM's.
