# Dirty-register sweep of the body audits (VP i20417, 2026-10-22)

**Why.** `OVL_0077_275B`'s C model stored two byte locals with a word writer, so the store at `[bp-1]` zeroed the low byte of the
saved BP ([U77_CAPACITY_CHECK_275B_CPU.md](U77_CAPACITY_CHECK_275B_CPU.md), #1351). Every fixture entered with `BP = 7F00`, whose
low byte is already zero, so the 31-case differential, the unit test and the native run all passed: a **fixture-value** blind
spot that no start or outcome accounting can show. This page records the same question asked of the ten other audits the
`7F00` search found, measured on main `dc24b0d2`.

**Method.** `tools/overlay/dirty_register_probe.py` runs an audit's own `main()` with its `Cpu` constructor wrapped and counts the entry
registers the differential really used. It judges nothing; the verdict is the audit's own (`rc 0` = every case agreed with the
original on 14 CPU words and 1 MiB). The one audit that fixed BP and zeroed the scratch registers (11AC) was changed, not just
counted. **What PASS means here, and does not:** the differential entered with these dirty values and agreed with the original
bytes. I did **not** mutate each model to prove each audit could catch a word-for-byte store; the 275B pair (fails on the old
model, passes on the new) is the only such demonstration.

## Results

| Audit (body) | Verdict | CPUs run | BP low byte != 0 | distinct BP low bytes | SI / DI / ES != 0 | odd SP |
| --- | --- | ---: | ---: | ---: | --- | ---: |
| `u77_275b_cpu_audit` (OVL_0077_275B) | **FAIL, fixed in #1351** (word stores over `[bp+0]`) | 46 | 15 added | 5 added | n/a | 0 |
| `u20_list_walk_11ac_cpu_audit` (OVL_0020_11AC) | **PASS** after the audit was made dirty (was BP=7F00, scratch=0: blind) | 45 | 20 added | 4 added | 5 added (the all-registers set) | 0 |
| `system_string_move_leaves_audit` (SYSTEM string/Move leaves) | PASS | 511 | 508 | 219 | 511 / 511 / 511 | 0 |
| `u1c_goto_00f4_cpu_audit` (OVL_001C_00F4) | PASS | 179 | 179 | 132 | all | 0 |
| `u7f_weighted_00a8_audit` (OVL_007F_00A8) | PASS | 231 | 231 | 153 | all | 0 |
| `u1c_compare_and_0aed_cpu_audit` (OVL_001C_0AED) | PASS | 184 | 184 | 129 | all | 0 |
| `u7f_roll_class_0fd2_cpu_audit` (OVL_007F_0FD2) | PASS | 311 | 310 | 187 | all | 0 |
| `u31_candidate_check_0255_cpu_audit` (OVL_0031_0255) | PASS | 174 | 174 | 131 | all | 2 |
| `u6f_party_insert_1446_cpu_audit` (OVL_006F_1446) | PASS | 333 | 332 | 185 | all | 0 |
| `u7f_record_subtract_0020_audit` (OVL_007F_0020) | PASS (fixed dirty constants `BP=7777`) | 558 | 558 | **1** | all | 9 |
| `u7f_record_word_add_003a_audit` (OVL_007F_003A) | PASS (fixed dirty constants `BP=7777`) | 2,212 | 2,212 | **1** | all | 33 |

**FAILS:** one, `275B`, already corrected. **PASSES:** ten. **NOT APPLICABLE:** none: every one of these bodies builds a frame
with `push bp`, so the saved BP is live in each. DS, SS and CS are inputs (the data, stack and code segments), so they are not
dirtied; SP is covered separately below.

`11AC` after the change: 20 dirty cases (`BP` 7F01, 7F80, FFFF and an all-registers set, each over the nil-head, walk,
freed-tail, freed-all and type-7 shapes), all agreeing; stops 34 return, 10 RunError, 1 walk limit.

## What the table does not say, and what I recommend

- **Two audits have a single BP value** (`0020`, `003A`: `7777`). A nonzero low byte is what exposes a word store, so they are
  covered for that defect, but a second value would also catch an error that depends on the byte (a store of the wrong byte
  into `[bp+0]`, say). Cheap to add; I have not.
- **Odd SP is nearly absent:** eight of the eleven audits never enter with an odd SP (only `0020`, `0255` and `003A` do) (the `sp8001` family is the same blind
  spot for stack alignment). A standing odd-SP case would close it.
- **The native harnesses** enter with a fixed register file (`ENTRY_REGS`, `BP=7777`), so for these bodies the native runs
  are dirty in BP but cover one value.
- **Should the dirty-register case be a STANDING requirement in the authoring checklist?** Yes, but on the evidence it is a
  precaution, not a systematic failure: the sweep found **no second failure** (one fail in eleven, and that one a strict
  credit). It costs one case per body (BP low byte nonzero, SI/DI/ES nonzero, one odd SP), and the one place it mattered was a
  strict credit whose C the corpus counted as done. I would add it as: *every body audit enters with at least two distinct
  nonzero-low-byte BP values, nonzero SI/DI/ES, and one odd SP, and the audit says so*, and let `dirty_register_probe.py`
  be the way to read what an audit really did. My own bodies were already dirty (`BP=7777`), which is why composing them
  found the 275B defect.

## Reproduction

    /tmp/gate-venv/bin/python tools/overlay/dirty_register_probe.py u1c_goto_00f4_cpu_audit --unicorn
    /tmp/gate-venv/bin/python tools/overlay/u20_list_walk_11ac_cpu_audit.py
