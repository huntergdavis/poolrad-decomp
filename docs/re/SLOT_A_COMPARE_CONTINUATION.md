# Saved slot-A COMPARE/IF continuation after seeded RANDOM

This is a bounded C script gate from ECL PC `0x9B40` to **the entry** of
EXIT at `0x9B67`. It composes the merged operand loader, COMPARE, IF, and
runner models with the seeded RANDOM transition from
[PR #231](SLOT_A_SEEDED_RANDOM_GATE.md). It matches the six dispatcher
successor states in the controlled native
[PR #232 trace](SLOT_A_ONE_UP_NATIVE_AL10_BRANCH.md). EXIT is deliberately
unbound, so none of its character, box, or heap effects are asserted.
This remains an injected-seed local gate, with no C-native one-Up, strict
full-C, or game-identity count claim.

Before auditing, `deja "Poolrad saved slot A ECL PC 0x9B40 0x9B67 COMPARE IF continuation 0x6DCA seeded RANDOM"`
found no exact session. Reused and checked the [native AL10 branch](SLOT_A_ONE_UP_NATIVE_AL10_BRANCH.md),
the [save transfer map](SLOT_A_NATIVE_SAVE_ADAPTER.md), the
[seeded RANDOM model](U1C_RANDOM_0234.md), the
[ECL runner](U1C_ECL_RUN_3171.md), the
[quiet `0x6DCA` load-hook classification](ECL_LOAD_HOOK_CLASSIFIER.md),
and the [bounded EXIT contract](U1C_EXIT_0001.md).

## Pinned bytes and caller-supplied state

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Saved script transfer, file `0x1401..0x3200` | 7,680 | `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| COMPARE, unit `0x1C:+0x0126..+0x01A2`, file `0x00E6C..0x00EE8` | 125 | `330e74c38244df80bd2af04209126a207b5ebdf5dcf97c4781d4b960e2a52404` |
| IF family, unit `0x1C:+0x0B52..+0x0BC7`, file `0x01898..0x0190D` | 118 | `11830c28e2ca43d40e80db72b6374a46e73b4e0cc0522ba9728e7026e3718061` |

The test restores the hash-pinned save into the modeled word windows. It
supplies only values from the native trace that changed after saving or are
needed to enter this already-reached handler: clock minute word `+0x18E=1`,
run stop byte `[0x49FF]=0`, initial compare flags
`(0,1,1,0,1,0)`, and the measured `RandSeed=3140872353`. The saved
window-1 word `0x6DCA` at byte `+0x594` is zero, independently measured
at the native pre-RNG stop. Seeded RANDOM changes saved window-2 word
`0x980F`, byte `+0x21E`, from 16 to 10 and advances the seed to
3238563622 before this continuation begins. The saved window-0 word
`0x4A0B` is zero. The native trace reports its **comparison result** as
below 255, not its exact post-key value; this gate claims branch/flag
parity at that point, not exact parity for that word.

## Six matched dispatches

The hash-pinned script has three numeric compares. The C test checks the
prior/current latch, handler successor PC, six flags **before and after**
each handler, and five measured window words at every dispatch. The common
flag vector is `(equal, unequal, below, above, below-or-equal,
above-or-equal)=(0,1,1,0,1,0)`. The common window tuple
`(minute,tens,hour,window1 +0x594,window2 +0x21E)` is `(1,5,10,0,10)`.

| Dispatch PC | Operation | Numeric comparison / branch | Successor PC |
| --- | --- | --- | --- |
| `0x9B40` | COMPARE `0x03` | `0x6DCA` (0) below literal 1 | `0x9B46` |
| `0x9B46` | IF `0x16` | equality false; skip the next statement | `0x9B50` |
| `0x9B50` | COMPARE `0x03` | supplied `0x4A0B` (0) below literal 255 | `0x9B56` |
| `0x9B56` | IF `0x16` | equality false; skip the next statement | `0x9B60` |
| `0x9B60` | COMPARE `0x03` | RNG word `0x980F` (10) below literal 12 | `0x9B66` |
| `0x9B66` | IF `0x1A` | below-or-equal true; proceed | `0x9B67` |

The skipped statements leave opcode `0x04` in the latch, matching the
native `0x04/0x03` previous/current pairs at `0x9B50` and `0x9B60`.
The runner then fetches opcode `0x00` at `0x9B67` and returns
`PR_REC_ECL_RUN_UNMODELLED` because no EXIT callback is bound; seven
fetch/dispatch attempts and six completed handlers are recorded. This
is a host stop **before EXIT**, whereas the controlled native trace enters
EXIT and later stops the run at `0x9B68`.

## First unsourced effect and reproduction

EXIT begins by testing `[0x84E2]`, which controls a resident box call, then
can restore a character pointer from `[0x4430]` and clear a heap call list.
Those values and effects are absent from the save adapter and are not
measured at this continuation entry. The test therefore stops before
the EXIT body. The key-dispatch bridge remains separate work; the native
seed was explicitly restored in PR #232 rather than derived from the save.

`bash tests/replay/check_slot_a_compare_continuation.sh` runs a public
synthetic false-then-true IF fixture with different script PC, destination,
and thresholds. Passing the ignored slot-A save as its sole argument adds
the private numeric gate; the script verifies the save and `GAME.OVR`
hashes, compiles under C11 with `-Wall -Wextra -Wpedantic -Werror`, and
writes the executable only under ignored `resources/`. No game bytes or
media are committed.
