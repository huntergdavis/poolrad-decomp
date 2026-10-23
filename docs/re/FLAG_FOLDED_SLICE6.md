# Folded flags: t44 slice 6 (the rest of the TSV; 49 rows, 42 files)

This doc covers every remaining row of [FLAG_FOLDED_SITES.tsv](FLAG_FOLDED_SITES.tsv), apart from two files: par-d2 owns
`overlay_u7f_weighted_00a8_cpu.c` and codex-poolrad2 owns `overlay_u3c6_caller_10a7`. Each row was checked against the
ORIGINAL bytes. The rows regenerate with `python3 tools/overlay/flag_fold_liveness.py --preset slice6`: 211 sites, of
which 186 are DEAD, 3 BRANCH and 22 EXPOSED, and each exposed site is explained below. The rules are the pinned
DOSBox-X ones in [FLAG_SEMANTICS_CENSUS.md](FLAG_SEMANTICS_CENSUS.md), and every call, return and model stop point
counts as observation.

Four parallel analysis passes did the first mapping (read-only; one per group of files). I checked the preset output,
the credit ids and the model claims marked below.

**Result: no folded flag is observed, and no model logic changes.** Two model comments that named the wrong
overwriting instruction are corrected.

## Verdicts by file

| File (TSV rows) | Original span(s) | Verdict |
| --- | --- | --- |
| `system_str_1366_cpu.c` (:119, :130) | SYSTEM `0AF8:[1366,13B1)` + `[1254,12A6)` | `SUB CX,DI` at `12A3` reaches the RET into `137A`, and `SUB SP,20h` at `1369` reaches the CALL to `1254`. Both die on arrival (entry rows: `CMP` at `1388`, `OR DX,DX` at `1258`). The `1254` helper model has no stop before it. |
| `system_longmul_0a5b_cpu.c` (:62, :75) | `0AF8:[0A5B,0A76)` | dead: `MUL` → `MUL` → `ADD` → `ADD`. The last `ADD` (`0A73`) reaches the RETF and is computed (`add16`). |
| `system_strstore_fullstate_0b25.c` (:34) | `0AF8:[0B25,0B49)` | `CMP AL,CL`: only `JBE` reads it, as the modelled clamp. Then the computed `XOR CH,CH` follows. |
| `system_string_move_leaves_cpu.c` (:95) | `0AF8:[0B57,0B98)` | dead: `OR CX,CX` at `0B6C` |
| `system_longdiv_0a76_cpu.c` (:98) | `0AF8:[0A76,0AE4)` | The quotient loop's carry chain (`RCL`/`SUB`/`SBB`/`ADD`/`ADC`/`CMC`) is read only inside the chain. The model carries it in `carry`, `remainder >= divisor` and `bit`. `JAE` and `JNE` are modelled, and `POPF` at `0ABC` kills everything leaving the loop. |
| `system_ioresult_assign_cpu.c` (:84) | `0AF8:[13E2,1410)` | `CMP AL,4Fh`: only `JBE` reads it, as the modelled clamp |
| `system_getmem_023f_cpu.c` (:103) | `0AF8:[0308,03D3)` | `ADD AL,0F0h`'s CF is read only by `ADC DX,0`, which is computed as `(low & 0xFF) >= 0x10`. The rest is dead, and the final `AND` is computed. |
| `system_freemem_0254_cpu.c` (:103) | `0AF8:[0491,04A2)` | `ADD AX,7`'s CF is read only by `RCR`, which is computed as the 17-bit shift. The shifts are dead, and the final `AND` is computed. |
| `resident_character_target_select.c` (:214) | `00BA:[2E8B,2F57)` | dead (`AND AL,10h` at `2ED9`) |
| `resident_0802_05cd_cpu.c` (:187) | `0802:[05CD,06E7)` | dead (`MUL` keeps SF/AF; `ADD` at `0626`) |
| `resident_0709_glyph_01df_cpu.c` (:3) | `0709:[01DF,05EB)` | 31 sites (`IDIV`, 23 shifts, 7 `MUL`s), all dead before every model stop (LIMIT, PORT, CODE) |
| `resident_0709_clear_0dd3_cpu.c` (:96) | `0709:[0DD3,0F82)` | 19 sites (13 `SHL`, 6 `MUL`), all dead |
| `resident_03f1_clear_0137_cpu.c` (:96) | `03F1:[0137,02E8)` | 19 sites, all dead |
| `resident_00ba_sidecar_lookup_2199_cpu.c` (:91) | `00BA:[2199,2208)` | dead (`OR` at `21BF`) |
| `resident_00ba_class_value_26c2_cpu.c` (:46) | `00BA:[26C2,2729)` | dead |
| `resident_00ba_byte_string_1037_cpu.c` (:78) | `00BA:[1037,1073)` | dead (`XOR` chain; `1047` is computed) |
| `overlay_u9e_entry16_10e4.c` (:86) | unit `9Eh` `+[10E4,110D)` | dead (`CMP` at `10F5` is computed) |
| `overlay_u6a_record_weight_0000.c` (:111) | unit `6Ah` `+[0000,0159)` | `XOR`s dead. The `MUL`s are in fact computed (CF/OF/ZF/PF; SF/AF kept). |
| `overlay_u3ce_bitmap_pair_release_014e_cpu.c` (:36) | unit `3CEh` `+[014E,01B0)` | dead (computed `OR`/`ADD`) |
| `overlay_u3b4_case_maps.c` (:64) | unit `3B4h` `+[00C7,00FB)`, `[00FB,012F)`, `[012F,016D)` | dead (computed `CMP`) |
| `overlay_u3b0_dispose_11f3_cpu.c` (:26) | unit `3B0h` `+[11F3,1245)` | dead |
| `overlay_u3b0_cursor_step_0cc5_cpu.c` (:102) | unit `3B0h` `+[0CC5,0D88)` | dead |
| `overlay_u33_stamp_block_02ba.c` (:114) | unit `33h` `+[02BA,0377)` | `SUB SP,2` reaches the CALL to `0221` and dies on arrival (`SUB SP` at `0224`). The `0221` model has no stop before it. |
| `overlay_u33_stamp_square_0377.c` (:109) | unit `33h` `+[0377,040F)` | dead |
| `overlay_u38_gated_append_001f_cpu.c` (:28) | unit `38h` `+[001F,0058)` | dead |
| `overlay_u38_roll_gate_2915_cpu.c` (:182) | unit `38h` `+[2915,2976)` | dead |
| `overlay_u38_target_index_gate_2a87.c` (:264, :298) | unit `38h` `+[2A87,2AE6)` | dead |
| `overlay_u38_target_mask81_25bc.c` (:254) | unit `38h` `+[25BC,2605)` | dead |
| `overlay_u3b0_space_scan_cpu.c` (:180, :205) | unit `3B0h` `+[08D7,0923)`, `[0923,0969)` | dead |
| `overlay_u3b9_bitmap_set_release_048f_cpu.c` (:63, :74) | unit `3B9h` `+[048F,04F8)` | dead |
| `overlay_u6f_party_insert_1446_cpu.c` (:239, :331) | unit `6Fh` `+[1446,157F)` | `SUB SP` dead. In loop C, the `CMP`s' CF reaches the LCALL at `1574` (the `INC`s keep CF). It is 0 on both loop exits (taken `JAE`, or `CMP x,0`), and the model sets exactly that. The branches are modelled. |
| `overlay_u86_entry3_0729.c` (:67, :75) | unit `86h` `+[0729,07D6)` | dead |
| `overlay_u1c_compare_and_0aed_cpu.c` (:226) | unit `1Ch` `+[0AED,0B52)` | dead |
| `overlay_u1c_return_0a9f_cpu.c` (:26) | unit `1Ch` `+[0A9F,0AED)` | dead |
| `overlay_u1c_text_2e7d_cpu.c` (:92) | unit `1Ch` `+[2E7D,2F2D)` | dead |
| `overlay_u20_party_award_0355_cpu.c` (:141) | unit `20h` `+[0355,04C9)` | `ADD`'s CF is read only by the next `ADC` (the model's carry); the rest is dead |
| `overlay_u25_alloc_1951_cpu.c` (:179) | unit `25h` `+[1951,19DB)` | dead |
| `overlay_u25_entry23_1fb1.c` (:204) | unit `25h` `+[1FB1,207E)` | dead |
| `overlay_u25_step_probe_0482.c` (:98) | unit `25h` `+[0482,0511)` | dead |
| `overlay_u25_store_string_0f53.c` (:239) | unit `25h` `+[0F53,1153)` | dead |
| `overlay_u33_cell_pair_01ad.c` (:140) | unit `33h` `+[01AD,0221)` | dead (`IDIV` at `01BF`, whose pinned flags are computed) |
| `overlay_u33_neighbourhood_0810.c` (:164) | unit `33h` `+[0810,08A4)` | dead (`ADD` at `0832`) |

## Side findings

1. **u9E +10FE `XOR AH,AH` (not a TSV row).** Its CF reaches the `RETF 4`, because the `INC AX` after it keeps CF. It
   is **equal by construction**. That arm runs only after `CMP BYTE [BP+8],12h` at `10F5` compared equal, so CF is
   already 0, and the model's `inc16_ax` keeps that 0: the value the `XOR` would leave. I checked this against the
   bytes and the model. It is left as a fold; a one-line `CF := 0` can be added if the PM prefers it explicit.
2. **Two corrected comments.**
   - `overlay_u33_cell_pair_01ad.c` said the `XOR`/`ADD` flags were overwritten "inside the first call" and that
     "IDIV leaves the flags alone". In fact the `IDIV` at `+01BF` overwrites them, and the model computes its pinned
     flags.
   - `overlay_u33_neighbourhood_0810.c` said `SUB SP`'s flags were overwritten by `+01AD`; it is the `ADD` at `+0832`.
   - Both changes are comment-only, and `flag_census.py --check` stays clean.
3. **Tool fix.** A folded shift by CL whose count isn't known at the site used to be reported DEAD without being
   checked, because `writes()` returned no flags. Such a site is now analysed with every flag it can write. `MOV CX,n`
   just before a shift now sets the count, as `MOV CL,n` already did. Glyph `04DB` (`MOV CX,4`; `SHL`) was re-checked
   both by hand and with the fixed tool: it is dead (`XOR` at `04E9`). The earlier presets give unchanged verdicts.
   There are two new unit tests.

## Credit

For each of these credited bodies, the evidence did not depend on the folded flags:

`RES_00BA_2E8B`, `RES_0802_05CD`, `RES_0709_01DF`, `RES_0709_0DD3`, `RES_03F1_0137`, `RES_00BA_2199`, `RES_00BA_26C2`,
`RES_00BA_1037`, `OVL_009E_10E4`, `OVL_006A_0000`, `OVL_03CE_014E`, `OVL_03B4_00C7`, `OVL_03B4_00FB`, `OVL_03B4_012F`,
`OVL_03B0_11F3`, `OVL_03B0_0CC5`, `OVL_0033_02BA`, `OVL_0033_0377`, `OVL_0038_001F`, `OVL_0038_2915`, `OVL_0038_2A87`,
`OVL_0038_25BC`, `OVL_03B0_08D7`, `OVL_03B0_0923`, `OVL_03B9_048F`, `OVL_006F_1446`, `OVL_0086_0729`, `OVL_001C_0AED`,
`OVL_001C_0A9F`, `OVL_001C_2E7D`, `OVL_0020_0355`, `OVL_0025_1951`, `OVL_0025_1FB1`, `OVL_0025_0482`, `OVL_0025_0F53`,
`OVL_0033_01AD`, `OVL_0033_0810`.

The eight SYSTEM leaf models have no credit rows of their own; they are composed by credited bodies, and nothing in
their behaviour changes.
