# Flag-semantics census (pinned DOSBox-X 5a9c7c53)

A flag computed by a helper is only as good as the rule it assumed; check the rule against the PINNED build, not a general x86 reference.

**Two flag paths in the pinned build.** Lazy getters (`get_CF/AF/ZF/SF/OF/PF`, `flags.cpp:75-480`) are read only by
conditional instructions (`TFLG_*`: Jcc, SETcc, CMOVcc). Every stored image (PUSHF, INT, any hook/stop/return snapshot)
goes through `FillFlags` (`flags.cpp:~700-1100`, `DOFLAG_PF` at `:504`). The two differ: word PF is 16-bit parity in
`get_PF` but low-byte in `FillFlags`; SHR-by-1 OF is `var1 > 8000h` lazily and `>= 8000h` stored; SHL OF for counts above 1
is `(res ^ var1) & 8000h` lazily and `MSB(res) ^ CF` stored. A model that BRANCHES on a flag must take the branch from the lazy
rule while its FLAGS image follows the stored rule. Models here follow the STORED rule; the "branched on?" column of
`FLAG_SEMANTICS_CENSUS.tsv` says where that is not enough.

Read from `src/cpu/instructions.h` (SHLW/SHRW/SHRB, MULW), `src/cpu/flags.cpp`
(`get_AF`, `FillFlagsNoCFOF`, `parity_lookup`). Nothing here comes from a summary.

## Pinned behaviour (normal/386 core)

- SHL/SHR/SAR with a non-zero count: `get_AF` returns `count & 1Fh`, so AF = 1.
  SF/ZF/PF come from the result (PF from the low byte), CF/OF from the lazy shift flags.
- MUL r/m16 (`MULW`): `FillFlagsNoCFOF()` first, so SF and AF keep their prior values.
  ZF = (AX == 0); PF = `PARITY16(AX)^PARITY16(DX)^FLAG_PF`. `FLAG_PF` is the constant
  mask, the three terms cancel to the even parity of all FOUR bytes of DX:AX, with no
  dependence on the prior PF. CF = OF = (DX != 0).

## Fixed here

| file | defect | fix |
|---|---|---|
| overlay_u3bd_lookup_fullstate.c `shr16` (also 0350 via +047B) | AF cleared | AF = (count & 1Fh) != 0 |
| system_blockread_14fb_cpu.c `mul16` | PF folded from bytes 0 and 2 only | fold all four bytes |
| system_seek_1563_cpu.c `mul16` | same | same |

## Census: helpers that differ from the pinned convention (NOT fixed here)

Observability at a return, hook or stop is NOT established by this table. Source
comments claim some are overwritten ("later ADD defines all six"); those claims are
unverified until a checker exposes the flag.

SHL/SHR AF (pinned: set):
- AF cleared: overlay_u7f_weighted_00a8_cpu.c `shl1`, overlay_u77_weighted_28be_cpu.c `shl1`,
  overlay_u7d_entry_05f0_cpu.c `twice_di` (explicitly clears), overlay_u5e_neighbor_1bf8.c `shr_ax`
  (logic16), overlay_u38_effect_scale_clear_2d88.c (SHR AX,1 via logic16), overlay_u3c2_entry5_0422.c
  shift ("choose zero").
- AF retained from input: overlay_u1c_compare_and_0aed.c `shift_di_one`, overlay_u5e_entry2_0123.c
  `shl_ax_one`, overlay_u25_entry1_0136.c `shl_ax_eight` (also retains OF; pinned OF = (res^var1)&8000h).

MUL flags (pinned: ZF from AX, PF from all four bytes, SF/AF kept):
- Only CF/OF written, SF/ZF/PF/AF left as input: overlay_u33_0b97_cpu.c, overlay_u3bd_place_0211.c,
  overlay_0077_275b_cpu.c, overlay_u7f_caller_1010.c, overlay_u3c2_entry5_0422.c, overlay_u3c6_rebuild_03a0.c.
- No flag update at all: overlay_u33_place_search_1577.c, overlay_u33_cell_claim_1434.c,
  overlay_u6a_record_weight_0000.c (byte MUL).
- PF from AX low byte, SF from AX, AF cleared: overlay_u33_grid_driver_11b4.c, resident_08ae_time_0000_cpu.c,
  overlay_u40c_tile_015d_cpu.c, resident_0464_compose_08d4_cpu.c.

Match the pinned convention: overlay_u86_roll_value_07d6.c, resident_00ba_character_helpers_cpu.c,
resident_00ba_character_0bb8_cpu.c, overlay_u3c6_04be_cpu.c, overlay_u3c6_0cb9_cpu.c,
overlay_u5e_neighbor_1bf8.c `mul_dx`, overlay_u86_string_node_028d.c, and every `shl` that sets 0x10
(U7F 1010/0A01/0508, U3C6, U3C2 0909, U5E 32E6, U9E 0160/0026, system_val_13b1, system_real, dos_intr_000b).

## "branched on?" column (original bytes)

Last column of `FLAG_SEMANTICS_CENSUS.tsv`: a capstone recursive descent over the ORIGINAL bytes (GAME.OVR units via
`overlay_inventory.find_units` function bounds; SYSTEM/resident routines from the entry to the first RET at or beyond the last
forward jump), reporting JO/JNO, JP/JNP, JL/JGE/JLE/JG (read OF) and SETcc/CMOVcc forms reachable from the model's entry.
JL/JG-family sites after CMP are omitted (lazy and stored rules agree). Sites after OR/AND/XOR/ADD/SUB/INC/DEC are listed as
"identical in both paths". The only JP/JO sites outside those cases were disassembled and are ASCII text that the descent
over-approximated into (strings after a routine's last RET); they are marked. No confirmed branch on PF, or on OF after a
shift, MUL or NEG, was found in the scanned models. "not scanned" means no function or extent could be resolved from the model's
name or header; it is NOT evidence of no branch. Entries are derived from the model's file name and header (`+XXXX`), so a model
that covers a different span than it names may be mis-bounded.

## Keeping the census complete (regenerable)

`python3 tools/overlay/flag_census.py --check` fails when a flag-writing `static` function in `src/recovered` has no row in
`FLAG_SEMANTICS_CENSUS.tsv`, or its row is still `UNCLASSIFIED`; `tools/overlay/test_flag_census.py` runs it in CI
(repository-hygiene). Rows are keyed by the md5 of the helper's normalized body, so a copy of a classified helper needs no new
row, and any CHANGE to a helper body needs a fresh verdict. Workflow for a new or edited model:

1. `flag_census.py --carry-from <rev>` after editing a helper (keeps its row, marks a `disagree` row FIXED), then
2. `flag_census.py --regen` adds `UNCLASSIFIED` rows for new bodies,
3. read the pinned STORED rule (`FillFlags`, `flags.cpp`; also `instructions.h` for MUL/DIV) and fill the row's verdict columns.

Scope boundary: the census covers helpers that WRITE a flags value. A model that FOLDS flags (comments such as "flags folded",
"IDIV's flags are undefined") computes none and asserts they are dead before the next observation; those are deadness claims, not
rules, and are NOT verified here. Pinned `DIVW/IDIVW/DIVB` store explicit values (AF=SF=OF=0, ZF=(rem==0 && quo&1), CF=((rem&3) in 1..2),
PF over rem:quo, `instructions.h:655-673`), so a folded DIV is only safe when nothing observes those flags.

DIV/IDIV sites now written to the pinned values (six flags overwritten): `overlay_u6c_record_roll_sum_4235.c`,
`system_random_fullstate_11c0.c` (DIV BX), `overlay_u6c_record_tier_3fb3.c`, `overlay_u6c_record_weighted_408c.c`,
`overlay_u68_movement_flag_04df_cpu.c`, `overlay_u33_cell_pair_01ad.c`, `overlay_u31_step_gate_0675.c` (IDIV CX). The last
carried the claim "IDIV leaves the flags alone in both tested engines", which contradicts `instructions.h:745-761`; its checker
re-run is what settles it. `overlay_u33_party_attach_12e1_cpu.c` already modelled IDIVW.

`FLAG_FOLDED_SITES.tsv` lists every folded-flags site by file and line (109 sites in 54 files at this commit, found by comment
text, so a fold with no comment is not listed). Each is an unverified deadness claim: show from the ORIGINAL bytes that no branch,
PUSHF, INT, hook/stop point or return observes the folded flags, or compute them. `classified_by` in the census TSV says whether a
row was read only by a sub-agent ("agent only") or also by hand when fixed.
