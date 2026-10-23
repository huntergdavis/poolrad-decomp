# Folded flags: u3b0 +00CB (task t44)

This doc covers the five rows of [FLAG_FOLDED_SITES.tsv](FLAG_FOLDED_SITES.tsv) for `OVL_03B0_00CB`
(`overlay_u3b0_item_split_00cb_cpu.c`). Each was checked against the ORIGINAL bytes: GAME.OVR unit `3B0h`,
`+00CB..+01B9`. The table regenerates with `python3 tools/overlay/flag_fold_liveness.py --preset u3b0`. The rules are the
pinned DOSBox-X ones in [FLAG_SEMANTICS_CENSUS.md](FLAG_SEMANTICS_CENSUS.md), and every far call and the return count
as observation.

## Per-site table

| TSV row | Sites | Instructions | Next writer | Observer | Verdict |
| --- | --- | --- | --- | --- | --- |
| `:109` `pair_index` (not live) | `0135`/`0137`/`013C`, `014C`/`014E`/`0153`, `0168`/`016A`/`016F`, `017F`/`0181`/`0186` | `XOR AH,AH`; `SHL AX,1`; `ADD DI,AX` | each by the next; then `CMP` at `013E`/`0190`, or `INC` at `0176` and `XOR` at `017F` | none | dead |
| `:156` | `013E` | `CMP BYTE SS:[DI-8Fh],0` | `XOR` at `014C`/`015F` | `JNE` at `0144` reads ZF, modelled as the branch | dead apart from the branch |
| `:156` | `015F`, `0161`, `0162`, `0176` | `XOR AH,AH`; `DEC AX` ×2; `INC BYTE [BP-2Ah]` | `DEC`; `DEC`; `XOR` at `0168`; `XOR` at `017F` | none | dead |
| `:219` | `010B` | `CMP AL,[BP-2Eh]` (AL = 1) | `XOR` at `011E` or `019B` | `JBE` at `010E` reads CF/ZF, modelled as `[BP-2Eh] != 0` | dead apart from the branch |
| `:235` | `0190` | `CMP AL,[BP-2Eh]` | `INC` at `0118` (CF survives), then `XOR` at `011E`; or `XOR` at `019B` | `JNE` at `0193` reads ZF, the modelled loop test | dead apart from the branch |
| `:238` | `0118` | `INC BYTE [BP-2Bh]` | `XOR` at `011E` | none | dead |

The writers that ARE exposed are all already computed by the model:

| Site | Instruction | Exposed at | Model |
| --- | --- | --- | --- |
| `00CE` | `SUB SP,2Eh` | `LCALL` at `00DF` | `arith_flags` |
| `00E7` | `ADD DI,0FF73h` | `LCALL` at `00F4` | `arith_flags` |
| `011E` | `XOR AH,AH` | `LCALL` at `012B` | the constant `0044h`, since the result is always 0 |
| `01A2` | `ADD DI,AX` | `RETF 6` | `pair_index(..., live = 1)` |

The `JE` at `0130` reads the SET-IN callee's returned ZF, which the model takes from the callee.

## Result

No folded flag is observed, and no model, checker or test change is needed.

`OVL_03B0_00CB` is credited. Its **evidence did not depend on the folded flags**: every
fold is dead or consumed by its modelled branch, and every exposed writer is computed.
