# Flag-semantics census (pinned DOSBox-X 5a9c7c53)

A flag computed by a helper is only as good as the rule it assumed; check the rule against the PINNED build, not a general x86 reference.

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
