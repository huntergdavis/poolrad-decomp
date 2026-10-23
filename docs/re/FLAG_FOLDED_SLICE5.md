# Folded flags: t44 slice 5 (six models, 19 rows)

This doc covers the rows of [FLAG_FOLDED_SITES.tsv](FLAG_FOLDED_SITES.tsv) for six models, each checked against the
ORIGINAL bytes. The table regenerates with `python3 tools/overlay/flag_fold_liveness.py --preset slice5`. The rules are
the pinned DOSBox-X ones in [FLAG_SEMANTICS_CENSUS.md](FLAG_SEMANTICS_CENSUS.md), and every call and return counts as
observation. `overlay_u7f_weighted_00a8_cpu.c` is left out on purpose, because par-d2's batch 3 touches it.

| Model | Original span |
| --- | --- |
| `overlay_u38_helper_cluster_cpu.c` | GAME.OVR unit `38h` `+1490..+14EA` (entry 50) |
| `overlay_u31_table_value_128d_cpu.c` | unit `31h` `+128D..+13C9` |
| `overlay_u3b0_step_scan_0b03_cpu.c` | unit `3B0h` `+0B03..+0C32` |
| `overlay_u3c2_record_sort_002e_cpu.c` | unit `3C2h` `+002E..+019F` |
| `overlay_u3c6_slot_offsets_0068.c` | unit `3C6h` `+0068..+00E2` |
| `system_textassign_insert_cpu.c` | START.EXE SYSTEM `0AF8:04F7..053B` (Text Assign) and `0AF8:0C68..0CBB` (Insert) |

## Per-row verdicts

| TSV row | Sites | Instructions | Next writer | Verdict |
| --- | --- | --- | --- | --- |
| u38 `:166` | `1493` | `SUB SP,2` | `AND AL,1` at `1499` | dead |
| u38 `:179` | `14B4` | `SUB BYTE [6818h],2` | `CMP` at `14BC` | dead |
| u38 `:198` | `14B1` | `INC BYTE [BP-1]` | `SUB BYTE` at `14B4` | dead |
| u38 `:202` | `14D0` | `ADD BYTE [6816h],4` | `AND AL,8` at `14D8` | dead |
| u31 `:111` | `12E2`/`12E4`, `12FA`/`12FC`, `134A`/`134C`, `1371`/`1373` | `XOR AH,AH`; `ADD AX,DX` | the next `ADD`, then a `CMP`/`SHL`/`AND`/`XOR` | dead |
| u31 `:135` | `129A` | `XOR AH,AH` | `SHL DI,CL` at `12A0` | dead |
| u31 `:168` | `12C0`, `12C2` | `XOR AH,AH`; `MUL DX` (pinned MULW keeps the XOR's SF/AF) | `MUL`; then `CMP` at `12CA` | dead |
| u3b0 `:140` | `0B5A`/`0B5E`, `0BF5`/`0BF9` | `ADD AX,SS:[DI-59h]`; `DEC AX` (`DEC` keeps `ADD`'s CF) | `DEC`; then `CMP` at `0B66`/`0C13` | dead |
| u3b0 `:148` | `0B7F`, `0C0B`, `0C1F` | `DEC AX` | the next `CMP` or `DEC` | dead |
| u3b0 `:196` | `0B06` | `SUB SP,2` | `XOR AX,AX` at `0B09` | dead |
| u3c2 `:90` | ten `XOR AH,AH`; `SHL DI,1`; `ADD DI,SI` triples (`0078`…`0175`) | as listed | each by the next; then `CMP`/`IDIV`/`ADD DI,6718h` | dead |
| u3c2 `:269` | `0072` | `INC BYTE [BP-2]` | `XOR AH,AH` at `0078` | dead |
| u3c2 `:279` | `0056` | `INC BYTE [BP-1]` | `XOR AH,AH` at `0062` | dead |
| u3c6 `:67` | `0091` `0095` `0097` `009E` `00A5` `00BB` `00BF` `00C1` `00C8` `00CF` | `XOR AH,AH`; `SHL DI,1` ×2; `SUB AX,DX` (×2) | each by the next; then `CMP` at `00DA` | dead |
| u3c6 `:99` | `006B` | `SUB SP,2` | `CMP` at `0076` | dead |
| u3c6 `:126` | `0080` | `INC BYTE [BP-1]` | `XOR AH,AH` at `0091` | dead |
| SYSTEM `:78` | `0529` | `CMP AL,4Fh` | `XOR CH,CH` at `0531` | only `JBE` at `052B` reads CF/ZF, the modelled `length > 4Fh` clamp |
| SYSTEM `:145` | `0C6B` | `SUB SP,200h` | `DEC AX` at `0C81` (all but CF) | **CF reaches the `CALL 0B57` at `0C84`**, then dies in the callee (below) |
| SYSTEM `:152` | `0C81` | `DEC AX` | none before `CALL 0B57` at `0C84` | **reaches the call**, then dies in the callee (below) |

**Insert's two rows.** The flags that `SUB SP,200h` (CF) and `DEC AX` (the other five) leave are live when Insert
`PUSH CS; CALL 0B57`s SYSTEM Copy.

`entry_liveness`, added to the tool in this PR, follows the flags that arrive at Copy's entry. `CLD`, `MOV BX,SP`,
`MOV DX,DS`, `LES`, `LDS` and `MOV AL,[SI]` keep them, and **`XOR AH,AH` at `0B66` overwrites all six before any
read**. The preset's last row shows this.

Inside the model, Insert composes `pr_system_strcopy_0b57`. Its only refusal is the argument check (`valid()`: RAM, size
and the entry IP), which Insert always satisfies, so it cannot stop at or before `0B66`. The arriving flags are
therefore never observed, and the folds are sound.

## Exposed writers outside the TSV

These are all already computed. The liveness walk found these writers whose flags reach a call or a return. A read of
each model confirmed every one is computed:

- u38 `OR AL,AL` at `149B` and `14DA` (`and_or_al`);
- u31 `ADD DI,5582h` at `12A2` and `CMP` at `13B0` (`arith_flags`);
- u3b0 `CMP`s at `0B1D`, `0B40`, `0BA5` and `0BCB` (`cmp_flags`);
- u3c2 `CMP`s at `0034`, `0049` and `0194`, and `ADD DI,6718h` at `0122`, `0156` and `0177` (`arith_flags`, `record3`);
- u3c6 `CMP`s at `0076` and `00DA` (`cmp8`);
- Text Assign's `XOR AL,AL` at `0535` (the constant `0044h`).

## Result and credit

No folded flag is observed, and no model, checker or test changes. For each credited body, the evidence did not
depend on the folded flags:

- `OVL_0038_1490`
- `OVL_0031_128D`
- `OVL_03B0_0B03`
- `OVL_03C2_002E`
- `OVL_03C6_0068`

SYSTEM Text Assign (`0AF8:04F7`) and Insert (`0AF8:0C68`) have no credit rows: they are composed leaf models, and
nothing in their evidence changes.
