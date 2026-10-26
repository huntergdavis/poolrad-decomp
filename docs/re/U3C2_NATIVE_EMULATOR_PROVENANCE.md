# U3C2 native emulator provenance (t108 batch 3)

`u3c2_caller_0909_native.py` and the native path in
`u3c2_record_sort_002e_audit.py` now use the
[shared executable-provenance helper](NATIVE_EMULATOR_PROVENANCE.md).
Each resolves the selected executable once, launches that returned path,
and attaches `dosbox_x` to its native report. The caller companion is marked
`NOT_A_ROW_CHECKER`; the sort audit keeps its existing `CHECKER_ROWS`.
No case, comparison, C body, flag mask or row registration changes.

## Measured before and after

Native measurement base: fresh `origin/main` at
`ceb665018d911d61891e7f39df778a5e6de1a020`, measured 2026-10-25.
The branch was then refreshed onto `c318fd474d3736e38b277eef4ab4074f4e7945ad`.
Provider and C/header input hashes were checked against the phase receipts;
the measured inputs are unchanged.
Both phases use the existing fixtures unchanged, with isolated output paths.
After removing only `dosbox_x`, both complete reports are identical.

| Fixture | Cases before / after | Comparison scope |
| --- | ---: | --- |
| U3C2:0909 caller | 24 / 24 | CPU14, 864 stack bytes, 65,536 DS bytes; zero flag masks |
| U3C2:002E sort | 40 / 40, including 31 with swaps | CPU14, 64 stack bytes, 768 record bytes; zero flag masks |

All 24 caller capture hashes match. SHA-256 of their ordered JSON array,
encoded with compact separators and no trailing newline, is
`17d6e6f148df4bb0554499bfb2f015c2dd5274d9bc0f575840db2a85a6b3fb27`. The sort RESULT.BIN SHA-256 is unchanged:
`433ef06ab70fd6b4ca3b210920f5cffc9e52e7019cdeaeaf451d37abd11d606a`.
Provider source hashes and all local C/header dependencies are recorded in
the private phase receipts; model inputs match between phases.

The emulator identity was checked before and after both phases:
`DOSBox-X version 2026.01.02 SDL2`, version exit status **1**,
SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`,
`pinned_build_used: false`. The release banner and reference source pin do
not certify the installed binary's build.

## Census and limits

On this branch's stated base, the census has **198 command lists in
183 files**. Removing exactly these two launchers leaves **196 command
lists in 181 files**. These figures describe this base and change; concurrent
provenance batches can change the aggregate census independently. The CSV
remains the inventory to compare against the matching source tree.
Synthetic `test_*.py` command lists are excluded from that inventory.

The native fixtures do not measure a natural game route. The caller still
reports no wrapped-frame coverage; the sort fixture composes SYSTEM Move
at the fixture's loaded segment. Native comparisons cover the listed windows,
not all RAM. Existing Unicorn evidence is separate and unchanged.
Other legacy launchers were not migrated or rerun in this batch.

Receipts and generated programs stay under ignored
`resources/t108-batch3/`. Every native run uses `poolrad-heavy.sh`.
No identity, membership, manifest, strict-credit or REPLACED data changes.
