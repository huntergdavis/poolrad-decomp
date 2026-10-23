# Folded flags: u7f +00A8 (task t44)

This doc covers the four rows of [FLAG_FOLDED_SITES.tsv](FLAG_FOLDED_SITES.tsv) for `OVL_007F_00A8`
(`overlay_u7f_weighted_00a8_cpu.c`), where the model does not compute an instruction's flags. Each was checked against the
ORIGINAL bytes of GAME.OVR unit `7Fh`, span `+00A8..+0130` (it ends in `RETF 4`, with far calls to SYSTEM `0A3F`, `0A5B` and `0A76`).

## Method

`tools/overlay/flag_fold_liveness.py` (wR:p1's shared walker, pinned DOSBox-X kill and observe rules) follows every successor
path, per flag bit, until a later instruction writes the flag (DEAD), an instruction reads it, or it reaches a far call, a
return or the end of the span (EXPOSED). The model's own stops in this body are its code checks at the three far-call sites
and the return; the walker already counts calls and returns as observers, so no extra host stop is needed.

    flag_fold_liveness.py --unit 0x7F --span 00A8:0130 --site 00C1 --site 00F9 --site 00FC --site 00FF --site 00CF

## Result

| site | instruction | walker | why the fold is sound |
|---|---|---|---|
| `+00C1` | `XOR AX,AX` | DEAD | `SHL DI,1` at `+00D8` writes all six flags before any read |
| `+00F9` | `ADD [BP-24h],AX` | EXPOSED: CF read by `ADC` at `+00FC` | the model folds `ADD`/`ADC` into one 32-bit add and computes exactly that carry, `added < old` (the 16-bit unsigned carry), so the one observed flag is consumed where the model already accounts for it |
| `+00FC` | `ADC [BP-22h],DX` | DEAD | `CMP` at `+00FF` writes all six |
| `+00FF` | `CMP BYTE [BP-25h],4` | BRANCH: only ZF, read by `JNE` at `+0103` | the model implements that branch as `== 4`; every other bit is killed by `ADD` at `+010B`, `INC` at `+00CF` or `SHL` at `+00D8` |
| `+00CF` | `INC BYTE [BP-25h]` | DEAD | `SHL DI,1` at `+00D8` |

**All four folded rows (five instructions) are sound.** No flag a folded instruction writes is observed by anything the model
does not reproduce. Needs the owner GAME.OVR under `resources/` to regenerate.
