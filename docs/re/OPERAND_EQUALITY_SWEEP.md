# Operand-equality sweep of the body audits (VP i20454/i20562, 2026-10-21; INTERIM, sweep paused)

**The question, shared with the dirty-register probe ([DIRTY_REGISTER_SWEEP.md](DIRTY_REGISTER_SWEEP.md)) and the load-base census
([LOAD_BASE_CENSUS.md](LOAD_BASE_CENSUS.md)): did the fixture ever take the value the instruction tests.** For an ordering compare
(`JB JAE JBE JA JL JGE JLE JG`) the value is EQUALITY: a model that tests CF where the original's `JBE` tests CF|ZF is wrong only
when the two operands are equal, and branch accounting cannot see that, because both sides of the branch are taken through the
unequal cases (275B: 8/10 outcomes, 45/45 starts, both sides of the `JBE` taken, and the model wrong at equality).

## The instrument: `tools/overlay/operand_equality_probe.py`

It runs an audit's own `main()` with every Unicorn emulation hooked, and at each ordering conditional in the **root segment**
(the CS of the emulation's first instruction; callee bodies in other segments belong to their own audits) it reads ZF and
counts runs with ZF set (equal operands, or a zero test) and with ZF clear. It judges nothing else. **How a boundary case should
be derived** (the technique that is the reusable part): do not invent constants; read the actual operands at the compare from the
ORIGINAL (a hook at the instruction after the `CMP`), and build one-below / equal / one-above from them, so the fixture adapts to
the body. That is how 275B's equal case was built (operands `S+P` and `R+1500` measured at `27C2`) and how 0239's capacity
boundary is pinned.

**Validated on the defect it exists for:** run on the 275B audit as it stood before the compare fix, it reports `27CA jbe`
`equal 0 / unequal 46` (and `2779 jbe`, the 15-node chain), i.e. it would have caught the CF-vs-CF|ZF defect before the model
was ever composed. After the fix the site reads `equal 1`.

## Result so far (178 audits probed; sweep paused, see the end)

Audits that run Unicorn and finished: **178**; ordering conditionals in root segments: **763** across 98 audits (the rest have
none); **118 never ran with equal operands**, in **31 audits**. The same site is often counted several times (the same shared body
under several composing audits, and under several emulation roots), so 118 is an upper bound on distinct sites.

| Audit | ordering sites | never equal | the sites (`CS:IP`, mnemonic, unequal runs) |
| --- | ---: | ---: | --- |
| `res03f1_clear_0137_cpu_audit` | 2 | 2 | `8000:017E` ja (26 unequal), `8000:0259` jbe (26 unequal) |
| `res0709_clear_0dd3_cpu_audit` | 3 | 3 | `8800:011A` ja (180 unequal), `8800:0E1A` ja (28 unequal), `8800:0EF3` jbe (30 unequal) |
| `res0709_msgbox_0bf5_cpu_audit` | 15 | 4 | `1709:05F8` ja (123 unequal), `1709:05FE` jb (123 unequal), `1709:0645` ja (123 unequal), `1709:064B` jbe (123 unequal) |
| `res0709_rect_fill_00b7_cpu_audit` | 1 | 1 | `8800:011A` ja (181 unequal) |
| `res0709_screen_0cab_cpu_audit` | 1 | 1 | `8800:011A` ja (146 unequal) |
| `res0802_025d_cpu_audit` | 1 | 1 | `1802:0136` ja (18 unequal) |
| `res0802_05cd_cpu_audit` | 1 | 1 | `1802:0136` ja (17 unequal) |
| `res0802_0716_composed_audit` | 10 | 2 | `1802:0136` ja (13 unequal), `1802:0810` jbe (191 unequal) |
| `res0802_0716_cpu_audit` | 10 | 2 | `1802:0136` ja (3 unequal), `1802:0810` jbe (164 unequal) |
| `res08ae_delay_006c_cpu_audit` | 12 | 1 | `FFFF:00B0` jbe (1 unequal) |
| `system_close_1491_cpu_audit` | 2 | 1 | `8800:14A8` jae (96 unequal) |
| `system_getmem_023f_audit` | 8 | 1 | `0AF8:03C9` jb (45 unequal) |
| `system_reset_1410_cpu_audit` | 3 | 1 | `6000:14A8` jae (20 unequal) |
| `u31_candidate_check_0255_cpu_audit` | 2 | 1 | `8800:2297` ja (1 unequal) |
| `u31_step_gate_0675_audit` | 3 | 1 | `3800:07C7` jb (134 unequal) |
| `u31_table_value_128d_audit` | 6 | 3 | `8800:12CF` jle (413 unequal), `8800:12ED` jle (413 unequal), `8800:132E` jle (148 unequal) |
| `u33_byte_flags_08a4_audit` | 100 | 2 | `F000:092F` jb (2277 unequal), `F000:0933` jbe (1742 unequal) |
| `u33_grid_roll_0000_audit` | 9 | 2 | `3000:00CD` jb (25 unequal), `3000:00F3` jb (18 unequal) |
| `u33_neighbourhood_0810_audit` | 14 | 4 | `3000:00CD` jb (1 unequal), `3000:00D1` ja (1 unequal), `3000:00F7` ja (1 unequal), `3000:0147` jg (15519 unequal) |
| `u33_place_search_1577_audit` | 27 | 2 | `3000:0147` jg (299 unequal), `3000:17B4` ja (102095 unequal) |
| `u38_roll_gate_2915_audit` | 2 | 1 | `3800:29A2` ja (214 unequal) |
| `u3b0_cursor_step_0cc5_audit` | 13 | 2 | `8800:1CF0` jge (2 unequal), `8800:1D11` jge (2 unequal) |
| `u3b0_entry_0c33_cpu_audit` | 11 | 2 | `9000:087B` jae (141 unequal), `9000:0B8A` jge (27 unequal) |
| `u3b0_item_split_00cb_audit` | 3 | 2 | `8800:110E` jbe (1 unequal), `8811:010E` jbe (59 unequal) |
| `u3b0_step_scan_0b03_audit` | 7 | 1 | `8800:1B21` jge (1 unequal) |
| `u3c2_caller_0909_audit` | 133 | 60 | `3000:0642` jle (358 unequal), `3000:069E` jle (331 unequal), `3000:06C4` jle (330 unequal), `3000:0720` jge (40 unequal), `3000:077C` jge (328 unequal), `3000:07BA` jg (32 unequal), +54 more |
| `u3c6_caller_10a7_cpu_audit` | 11 | 4 | `3000:04C5` jl (174 unequal), `3000:04CB` jg (174 unequal), `3000:04D1` jl (174 unequal), `3000:04D7` jg (174 unequal) |
| `u3c6_cell_04be_audit` | 8 | 4 | `FFFF:04C5` jl (1 unequal), `FFFF:04CB` jg (1 unequal), `FFFF:04D1` jl (1 unequal), `FFFF:04D7` jg (1 unequal) |
| `u3c6_rebuild_03a0_audit` | 9 | 4 | `7000:03C2` jbe (1 unequal), `7000:03DF` ja (2 unequal), `FFFF:03C2` jbe (1 unequal), `FFFF:03DF` ja (2 unequal) |
| `u77_275b_cpu_audit` | 5 | 1 | `0814:2779` jbe (50 unequal) |
| `u9e_character_pair_115c_audit` | 56 | 1 | `6000:112D` ja (12 unequal) |

## Triage (three buckets, as ruled; only the sites triaged so far are placed)

(a) **Closure artefacts: the OWNING body's own audit has the equal case.** `0802:0136 ja` (0105; its own audit 4 equal, 0193's 7),
`08AE 006C jbe` (72 equal in the same audit, the other root is another scenario), `07A4 087B jae` (07A4's own audit: 10 equal).
`0709:05F8/05FE/0645/064B` (the 0BF5 audit) belong to `0709:05EB`, whose own audit is now registered and holds the `x` 0/39, `y` 0/24 equalities.
(b) **Not applicable.** The flag is a DOS service's carry, not a compare: `1491` and `1410`, `jae` after `INT 21h 3Eh`. **Infeasible by dominance:** `U33 08A4 092F jb / 0933 jbe`
compare AL with 3 and 4 after the chain at `0900/0904` has already sent 3 and 4 to `0926`, so AL = 3 or 4 never reaches them.
`0709:00B7 011A ja` compares `0` with `8*rows - 1` (always odd), which is never 0.
`SYSTEM GetMem 03C9 jb` follows the indirect HeapError call; the model stops there for a non-default handler, so `AL = 1` is outside its contract.
(c) **Constructible gaps (to fix as their own small PRs, each by the capture-at-compare technique):** 275B `2779` (needs a 15-node chain; **275B is
RIGHT at 15**: 0239's `count-15` case passes through it, so this is an audit gap, not a model defect), `0716 0810 jbe` (`[bp-2C]` vs `8CA0`: a block header of exactly 36000),
`115C 112D ja` (a pair field of exactly `65h`), `0B03 jge 0B8A`. **Everything else in the table is still UNTRIAGED**: a site listed above and not placed
here has no verdict. The long runs of `jl`/`jg` in `u3c2_caller_0909` and `u3c6` are signed bound checks that nobody has examined for equality yet.

## What was NOT probed, stated as zero coverage

A timed-out or unrun audit contributes **no coverage**, not partial coverage.
- **Timed out at 30 minutes (4):** `u25_init_01b6` (it prints a line per case and needs longer), `u5e_caller_32e6` (likewise), `u33_grid_driver_11b4` and `u33_grid_pair_0d13` (**opaque**: no output at all). The cap was the probe's, so this is a statement about the probe's cost model for units 25, 33 and 5E, not about the audits.
- **Not yet run (45 audits, paused at the fourth timeout):** the `u6a`, `u6c`, `u77`, `u7d`, `u7f`, `u86`, `u9b`, `u9e` and `u68` families and `u6f_*`, listed in the sweep log.
- **Failed to run:** `party_reorder_oracle` (not an audit: `KeyError: 'entry'`); `u20_list_walk_11ac` produced no result record and needs a rerun.
- **Never probed because it declares no Unicorn run or no registered row:** the declared-boundary audits (no emulation).

## Composition is the best audit found so far

275B is 132 bytes and carries four findings tonight (the BP word store, the CF-vs-CF|ZF compare, a missing equality case, and the constant CS):
the first body to be **composed under another** rather than tested alone. The first three were found by 0239 calling it; the probe's equal-operand table
found the third independently. The fixture families ([dirty registers](DIRTY_REGISTER_SWEEP.md), equality, [load base](LOAD_BASE_CENSUS.md)) are three answers to one question, and a
composer asks it for free because it supplies values the callee's own audit never chose.

## Reproduction

    # from a separate worktree with the resources symlinked (never switch branches under a running sweep)
    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/operand_equality_probe.py out.json u77_275b_cpu_audit
    # arguments after the module name are the audit's own (for example --unicorn)
