# Folded flags: u77 +28BE (task t44)

This doc covers the six rows of [FLAG_FOLDED_SITES.tsv](FLAG_FOLDED_SITES.tsv) for `OVL_0077_28BE`
(`overlay_u77_weighted_28be_cpu.c`). Each was checked against the ORIGINAL bytes: GAME.OVR unit `77h`, `+28BE..+2938`.
The table regenerates with `python3 tools/overlay/flag_fold_liveness.py --preset u77`. The rules are the pinned
DOSBox-X ones in [FLAG_SEMANTICS_CENSUS.md](FLAG_SEMANTICS_CENSUS.md), and every far call and the return count as
observation.

## Per-site table

| TSV row | Site | Instruction | Next writer | Observer | Verdict |
| --- | --- | --- | --- | --- | --- |
| `:154` | `28C1` | `SUB SP,0Ah` | `XOR AX,AX` at `28C4` | none | dead |
| `:157` | `28C4` | `XOR AX,AX` | `SHL AX,1` at `28D9` | none | dead |
| `:163` | `28D9`, `28DE` | `SHL AX,1`; `ADD DI,AX` | `ADD` at `28DE`; `XOR DX,DX` at `28E5` | none | dead |
| `:180` | `2902` | `ADD [BP-8],AX` | `ADC` at `2905` | `ADC` reads CF | CF consumed by `ADC`, which the model computes as the 32-bit carry `added < old`; the other five are dead |
| `:180` | `2905` | `ADC [BP-6],DX` | `CMP` at `2908` | none | dead |
| `:185` | `2908` | `CMP BYTE [BP-9],4` | `INC` then `SHL` at `28D9` (loop), or `ADD` at `2914` | `JNE` at `290C` reads ZF, modelled as the equality branch | dead apart from the branch (CF survives `INC` and dies at `SHL`) |
| `:187` | `28D2` | `INC BYTE [BP-9]` | `SHL AX,1` at `28D9` | none | dead |

The other flag writers, not in the TSV:

- `XOR DX,DX` at `28E5` and the first `SHL DI,1` at `28F1` are dead.
- The second `SHL DI,1` at `28F3` is exposed at the multiply call `28FD`, and the model computes it.
- `ADD AX,64h` at `2914` gives its CF to `ADC DX,0` at `2917`.
- `ADC DX,0` is exposed at the divide call `2922`, and the model computes it.

No folded flag is observed. The model's folds are sound, and no flag computation changes.

## The failing focused test (#1433 fallout)

`poolrad_overlay_u77_weighted_28be_cpu_tests` failed on main at `test_overlay_u77_weighted_28be_cpu.c:151`. This is
**#1433 fallout**. Commit `1b46855b` made the model's `shl1` set AF, the pinned SHL rule: `get_AF` is `count & 1Fh`,
non-zero. The test and the model's comments still described "AF clear, as Unicorn leaves it".

The test's CODE stop at the multiply site `28FD` (j = 0) exposes the second `SHL DI,1`'s flags. Those are now ZF, PF and
AF, i.e. `0x0054`, not `0x0044`. The change also wrote the AF line four times. This PR makes three changes:

- The test expects `0x0054`.
- The duplicates are removed.
- The `shl1` and header comments say AF is set, and each fold comment names the instruction that overwrites it.

## The checker (#1433 fallout too)

The same change broke `u77_weighted_28be_cpu_audit.py` without anyone noticing. Run unchanged on main through
`poolrad-heavy.sh`, it stops at its first `28FD` CODE stop with `flags mismatch +28BE code-28fd-0: C=7256,
original=7246`. That is AF alone, because raw Unicorn clears AF after `SHL` and the pinned build sets it.

The audit now uses the shared declared split, `pinned_shift_flags.declared_flags_split`, as the other pinned-flag audits
do. It tracks the original's last flag writer over the body and both SYSTEM leaves:

- after a non-zero shift, C must equal Unicorn with AF set;
- after a word `DIV`, the pinned six flags apply;
- everywhere else, the comparison is exact.

It is not a blanket mask. Re-run unmasked through `poolrad-heavy.sh`:

- **Unicorn:** **231** cases pass (223 returns, 4 CODE, 3 CALLEE, 1 TRANSFER), with all 14 CPU words and the full 1 MiB
  equal. The split was used at **2** arrivals, both pinned AF after the second `SHL` at a `28FD` CODE stop.
- **DOSBox-X:** **120** cases pass (49 negative quotients), comparing all 14 CPU words and the stack window, with no mask.
- **Focused test:** 18,028 checks pass.

## Credit

`OVL_0077_28BE` is credited. The **evidence did not depend on the folded flags**, because all six rows are dead or
consumed by a modelled carry or branch. The credited checker has been failing on main since #1433, on the `28FD`
CODE-stop flags, which are computed and not folded. With the declared split it passes again; it compares all 14 CPU
words and the full 1 MiB.
