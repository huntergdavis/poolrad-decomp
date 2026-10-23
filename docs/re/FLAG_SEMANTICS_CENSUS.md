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
- `PARITY16(x)` (`instructions.h:589`) is `parity_lookup[hi]^parity_lookup[lo]^FLAG_PF`, and `FLAG_PF` is the
  CONSTANT mask 4 (`regs.h:25`), not the live flag: `PARITY16(x)` is just the even parity of the 16 bits. So DIVW/IDIVW
  (`instructions.h:655-673, 743-761`), whose PF line is `PARITY16(rem)^PARITY16(quo)^FLAG_PF`, set PF to the even parity
  of all four bytes of rem:quo, again with no dependence on the prior PF. A rule written as `...^PF` in this census means
  that mask. The one row worded that way (`overlay_u33_party_attach_12e1_cpu.c`, IDIV CX=8) is reworded; its verdict
  (agree) does not move, and no other row's classification changes.

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

## Inline FLAGS writes in non-static bodies (2026-10-23)

The detector scanned only `static` helpers, so a flag computation written inline in a body function was invisible:
OVL_005E_0E52 cleared AF after its final SHR and no row existed (PM i21766; fixed in #1461). `flag_census.py` now
also scans unindented non-static definitions. Static bodies keep their exact normalization (no existing hash or
classification changed; the one row the regen dropped was 28BE's, merged into 00A8's identical SHL row by #1458).

**137 new rows** (59 in credited bodies). Verdicts were drafted by read-only agents against the pinned source, as
for the first census. par-d2 then hand-confirmed the mechanism of 11 of the disagreements in the C source (marked
`agent; mechanism hand-confirmed (par-d2)`), linted every agree row for an AF-keeping `08C5h` mask or an AF-less
shift (7 credited hits, all folded shifts with no live flags, which are t44's scope), and classified 0E52 by hand.

**17 rows disagree on the stored path** (real model-vs-pinned differences; fixes are separate work, credited first):
- AF kept across OR/XOR (mask `08C5h`; pinned OR/XOR clear AF, `flags.cpp:629-691`): OVL_0025_0756 (credited),
  OVL_009E_0DE3 (credited), OVL_009E_0E05, OVL_03BD_07F9, `overlay_u3b0_0883_xor_ah`, `overlay_u3b0_089f_xor_ah`.
- AF cleared after SHR (pinned sets AF = `count & 1Fh`): OVL_0038_2A87, OVL_0038_2AE6, OVL_0038_25BC (credited; their
  files already set AF under `PR_TARGET_SHIFT_DOSBOX386` in a sibling function), OVL_0033_0BDE (credited),
  OVL_005E_0E52 (credited; FIXED in #1461), `resident_loader_0553` (with OF).
- MULW ZF/PF/SF (pinned: SF/AF kept, ZF from AX alone, PF over DX:AX, `instructions.h:605-619`): RES_0709_0DBE
  (credited; both files), RES_0709_06A7 (credited), OVL_007D_0000, `slot_a_u6f_1937_precall`.
Several are reported unobservable (a later instruction overwrites the bit before any read); each row's `detail` says
which. A further 11 agent disagreements are the established word-op PF path split (stored path agrees).

**Detector false positives (2):** `pr_rec_res_00ba_2e8b` (a game-data byte named `flags`) and `pr_u3b9_0708_show`
(a comment), marked `n/a (no FLAGS write)`.

**Pinned path split found while classifying:** for SHR by 1, stored OF is `var1 >= 8000h` (`flags.cpp:767`) but lazy
`get_OF` is `var1 > 8000h` (`flags.cpp:401-404`; bytes `> 80h`). They differ only for an input of exactly 8000h/80h,
and only a JO/JNO reads the lazy path. No row here depends on it.

## The row hash covers macro definitions (2026-10-23)

A flag rule can live in a `#define` (OVL_0025_0756's AF fix in #1465 edited only `DEFINED_OR_FLAGS`), and a hash of the
body text alone could not see it: the row did not move and had to be marked FIXED by hand. The hash now covers the
normalized body **plus the definition of every macro the body names**, taken from the file and from every file it
`#include`s directly (**one level deep**). A body that names no macro keeps exactly the hash it had before.

Limits, stated: includes nested deeper than one level are not read; `#ifdef` branches are not evaluated (every
`#define` in the read files counts, and the last one wins); a function the body calls has its own row. The migration
re-keyed 279 rows (388 of 854 helpers name a macro) with **no classification changed**, no row split and no merge.
`test_flag_census.py` shows that a changed `#define`, in the file or in a directly included header, moves the hash.

## Uncredited disagreements fixed (2026-10-23)

The seven stored-path disagreements in uncredited bodies from the inline census are fixed to the pinned rules:
`overlay_u9e_entry8_fullstate_loop_0e05`, `overlay_u3b0_0883_xor_ah`, `overlay_u3b0_089f_xor_ah` (XOR clears AF);
`overlay_u3bd_leaf_call_07f9` (OR clears AF); `resident_loader_0553` (`SHR AX,CL` with CL=4 sets AF and clears OF);
`overlay_u7d_list_timer_0000_cpu` (`SHL DI,1` sets AF; `MUL` sets ZF from AX and PF over DX:AX); and
`slot_a_u6f_1937_precall` (`MUL DX` sets ZF from AX and PF over DX:AX). Their C tests pass with asserts enabled;
three of them (0883, 089F, `loader_0553`) had encoded the old AF and now expect the pinned value. The Unicorn checks that
compile 0E05 (`0e2e`: 16 cases, `2b20`: 54 cases) and the `u7d_list_timer_0000` audit pass unchanged. Limit:
`resident_loader_0553_audit.py` needs a native capture that is absent on this host and was not run.

Tooling note: `--carry-from` did not carry the rows of non-static body functions (here u7d_list_timer_0000 and 0E05;
earlier 0BDE and 0DBE `_cpu`); they were carried by hand. Fixing the matcher is a follow-up.
**Fixed (2026-10-23):** the cause was that `--carry-from` archived only `src/recovered` from the old revision, so bodies naming a
macro from `include/` hashed differently there; it now archives `include/` too (`rev_digests`), and a test pins the four rows by name.

