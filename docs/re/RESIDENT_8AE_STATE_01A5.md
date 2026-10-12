# RES_08AE:01A5 state setter: bounded control flow and full-C blocker

`RES_08AE:01A5` conditionally changes DOS interrupt and hardware timer
state before writing its WORD argument to DS `[0x5374]`. This note proves
the setter's instruction paths and nine static callers. It does **not**
claim that the restored slot-A one-Up executes this procedure, select a
live branch, resolve source ownership, or add full-C or game-identity
credit. No C translation is submitted because the reached helper effects
need a DOS vector and port-I/O contract that is absent from the portable
runtime.

## Provenance and exact bounds

Before decoding, `deja "Poolrad RES_08AE:01A5 DS 0x5374 state setter
dispatch gate"` found no matching session. The prior
[slot-A dispatch-gate note](SLOT_A_RESIDENT_DISPATCH_GATE.md),
[dispatch model](RESIDENT_8AE_00DC.md), and
[adjacent leaves](RESIDENT_8AE_LEAVES.md) supplied address leads and the
unsampled-route constraint. The procedure, its direct callees, and all
overlay callers were checked independently against the ignored DOS
inputs: `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `GAME.OVR` SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

| Procedure | Loaded image, exclusive end | START file, exclusive end | Instructions / bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_08AE:01A5..01D2` | `0x08C85..0x08CB2` | `0x08C9C..0x08CC9` | 18 / 45 | `7307263a3cf75d350b4ac4c02226dc74790e437194fb31922620ef24074e0c2f` |
| local `+137C..+13B1` | `0x09E5C..0x09E91` | `0x09B59..0x09B8E` | 24 / 53 | `2dbbe197d8c276b5cb7dc04dc19e0156b06d1772f880396de147fc244470696d` |
| local `+13B1..+13EA` | `0x09E91..0x09ECA` | `0x09B8E..0x09BC7` | 27 / 57 | `0fa5df2cbe185bdb43d4ff225e14b5b23a6893e85721aac20a99f86b34a0b019` |

All spans decode to their final `RETF` and have contiguous copied-byte
provenance. The resident inventory's `0x08A9C` for this target is the
*packed source* image offset; the loader expands that source to loaded
image `0x08C85`. Both map to START file `0x08C9C`.

## Exact setter paths

The far Pascal caller supplies one full WORD at `[BP+6]`; the setter
returns with `RETF 2`. The `PUSH CS` before each near call constructs
the far return expected by the two local helpers. The compare at
`+01B1` is unsigned `JBE` against zero, so its taken path is exactly
argument zero.

| Gate / site | Reached effects before return |
| --- | --- |
| `+01A8`: DS byte `[0x5372] == 2` | Jump at `+01AF` to `+01CE`; no helper call and no `[0x5374]` store. |
| Mode differs, argument `0` at `+01B1/+01B5` | Call `+137C` at `+01C5`. After it returns, load the argument at `+01C8` and store WORD zero to `[0x5374]` at `+01CB`. |
| Mode differs, argument nonzero, old `[0x5374] == 0` at `+01B7/+01BC` | Call `+13B1` at `+01BF`. After it returns, store the full argument WORD at `+01CB`. |
| Mode differs, argument nonzero, old `[0x5374] != 0` | Skip the helper at `+01C2`; store the full argument WORD at `+01CB`. |

The store is after the reached helper, so helper effects must precede
it. The setter itself has no other nonstack memory write. There is no
error return branch around either call and no return-register value
used by the setter.

The `+137C` helper queries and replaces DOS interrupt vector 8 using
`INT 21h`, saves the former vector in its code segment, and programs
timer ports. The `+13B1` helper restores that saved vector through
`INT 21h` and writes timer, speaker, and related ports. These are
observable system effects, including external DOS and device state.
Replacing either with a no-op or a callback whose contract only reports
success would omit original effects. A faithful portable translation
needs a specified host DOS-vector and port-I/O interface, including
ordering and failure behavior, or complete separately reviewed helper
translations. This is the concrete full-C blocker. The behavior is
consistent with a sound or timer service, but its game/library source
ownership is unresolved.

## Static caller context and live limit

Fixup-confirmed overlay calls supply literal WORDs immediately before
all nine setter calls:

| Overlay unit | Call offsets | Pushed WORDs |
| --- | --- | --- |
| `0x036` | `+0464` | 100 |
| `0x05E` | `+2816`, `+286E` | 0, 10 |
| `0x086` | `+0DF0`, `+0E3E`, `+16DD`, `+1736` | 0, 10, 0, 10 |
| `0x3C6` | `+0E1A`, `+109C` | 0, 10 |

Unit `0x036:+045B/+045D` clears DS `[0x5374]` before it pushes 100
at `+0460/+0463` and calls the setter at `+0464`. Thus, **if** its
mode is not 2 and no intervening writer changes the active WORD, this
site selects `+13B1` before storing 100. This corrects the potential
impression that the zero in AX just before `+045D` is the setter's
argument. The call at unit `0x05E:+286E` follows its separate
`RES_08AE:00DC` call at `+2865`; that ordering alone does not establish
the saved one-Up route.

The native one-Up watch proves a return from the coordinate writer's
call to `RES_08AE:00DC`, not a call to this setter. Its mode, active
WORD, and dispatch argument were not sampled. No caller listed above
is the observed unit `0x068:+0742` callsite. A live trace at the setter
entry or at its callers is needed before assigning any one-Up branch.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_8ae_state_01a5_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR
```

The audit prints only addresses, counts, argument WORDs, and hashes.
Original executable and overlay bytes remain under ignored `resources/`.
