# OVL_0068_0294 (unit `0x68` `+0x0294`): final disposition

**Verdict: bounded model, no identity, no strict credit.** This is the last
row held back from the pool, and it is recorded here as a disposition rather
than an acceptance. Queue classification stays `game_evidenced` (grade B, not
composable); the reviewed-identity and strict-full-C tables do not list it and
must not until the gaps below are closed.

This is **not a clean sheet**. The model is useful and the semantic work is
real, but three things stand between it and credit: two callees are hooks
rather than translated bodies, one flag is special-cased at a stop boundary,
and the closure gaps are unmeasured. Nothing here claims a completion
percentage or a count.

## Extent (re-verified for this record)

| Item | Value |
| --- | --- |
| unit:offset | `0x68:0x0294`, GAME file `92,511` (`0x1695F`), span `[0x1695F, 0x16BAA)` |
| size | **587 bytes, 247 instructions** |
| body SHA-256 | `8a2393e6e0632118ba5094510ee1d8aa5a0a3a2c84ef2eb1ad27a0f85844ef48` |
| terminator | `CB` (`retf`, no argument) as the final byte at `+0x04DE` |
| next prologue | `55 89 E5` immediately after, so the ledger bound is exact and there is **no trailing data** |
| near callers | exactly two, both in entry 1: `+0x0ECA` and `+0x1009` |
| far calls | `0x3D5:0x2F` (unit `0x3BD` entry 3) x1; `0xB0:0x48` (unit `0x9E` entry 8, the roll) x12 sites |
| near call | `+0x0109` (map updater) x2, after the loop |
| runtime/resident calls | none |

The byte span, the terminal `retf` and the next-prologue boundary were
recomputed from `GAME.OVR` `872964a3…28614` for this record; the recorded
digest and the media agree.

One stale figure is corrected here: [U68_ROLL_GATE_0294.md](U68_ROLL_GATE_0294.md)
states the body starts at file `91,851`, which contradicts both `0x1695F` and
the checked-in ledger row (`92511`). `0x1695F` = **92,511**, and the 587-byte
body taken from `92,511` reproduces the recorded SHA-256 exactly, so the
ledger and the media are right and the prose offset was wrong. No extent,
identity or credit changes: the ledger was already correct.

## Why it is not creditable as-is

1. **Two callees are explicit hooks, not translations.** The roll
   (`0xB0:0x0048` = unit `0x9E` entry 8) and the near map updater `+0x0109`
   are called through hook interfaces, and entry 3 (unit `0x3BD`) is consumed
   as the merged translation. The body is therefore a model of the *driver*,
   not a full-C body with its callees resolved. Compare
   [U68_ROLL_GATE_0294.md](U68_ROLL_GATE_0294.md).
2. **AF exception at the stop boundary.** The shared dice prefix leaves the
   undefined `XOR` `AF` unchanged; only at its LIMIT boundary does the adapter
   clear `AF` to match the measured Unicorn profile. Undefined
   `DIV`/`IDIV`/shift flags keep the dependency's existing profile, and native
   partial-stop flags are not measured. So full-state parity is **not**
   established for `AF`; it is established *within* a stated profile that the
   adapter itself has to touch. See
   [U68_ROLL_GATE_0294_CPU.md](U68_ROLL_GATE_0294_CPU.md).
3. **The closure gaps are unmeasured.** Recorded verbatim by the CPU record:
   no native guard, budget-stop or full-1 MiB capture, asynchronous
   interrupts, MMIO, self-modifying code, wrapping stack-offset frames,
   arbitrary stack/data alias coverage, or end-to-end gameplay claim; oracle
   data/list/map/code and active stack are separate; defensive
   unexpected-return paths remain unmeasured.

## What would change the verdict

Any one of these would move the row, in increasing cost:

* **Accept the hooks as an interface caveat.** Convert the two hook calls into
  a documented `RAW_CPU_INTERFACE_CAVEATS`-style entry for this body, on the
  same reasoning used for the raw-CPU bodies that are credited with a stated
  interface. That is a reviewer decision, not an author one, and it must name
  the hooks explicitly.
* **Measure the AF/DIV boundary.** A native capture at the guard and
  budget-stop boundaries, on a stated CPU profile, so the adapter no longer
  has to clear `AF` to match. This removes the exception rather than
  documenting it.
* **Translate the callees.** Replace the roll and map-updater hooks with
  strict bodies, which also unblocks the callers that wait on them.

Until one of those lands, the row stays a bounded model with its `AF`
exception and its coverage gaps intact, and it stays out of both credit
tables. That is the honest end state of the pool: 235 strict originals / 281
verified identities, with this body and its gaps recorded instead of papered
over.
