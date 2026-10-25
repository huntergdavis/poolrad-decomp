# Native emulator provenance (t107)

The 0400 native receipt previously identified the compared game bytes but omitted
the emulator executable. `native_emulator_provenance.describe_dosbox` now resolves
the selected executable, hashes that file, obtains its version, and returns the
path that the harness actually launches. The report includes `dosbox_x.path`,
`version`, `sha256`, `version_exit_code`, `pinned_source`, and
`pinned_build_used: false`. A release string does not attest a source revision or
build. Existing independently attested pinned-build harnesses remain separate.

DOSBox-X 2026.01.02 returns status 1 for a successful `--version` invocation. The
helper accepts status 0 or 1 only with a DOSBox-X version banner, records that
status, and rejects other statuses or missing banners. Missing executables and
timed-out probes also fail before the native run.

## Scope

The shared `ovl040c_015d_cpu_native` provider supplies 0400, 040C:015D,
03B6:0000, and the 03F1:0000 box audit. Its returned metadata travels with each
consumer's native receipt. The SYSTEM text provider, 0464 video-mode audit,
0464:08D4 composition provider, 08AE:006C/137C/13B1 audits, and U6F/U77
wrapped-return launchers also use the helper.

The three 08AE audits previously launched `dosbox-x` from PATH but hashed
`/usr/bin/dosbox-x`. They now resolve PATH once, hash and launch that same selected
file. Existing flat hash fields remain aliases of the recorded executable hash.
No parity comparisons, case selection, masks, C bodies, or credit tables change.

Per VP scope ruling i24207, legacy bare-PATH launchers are deferred to **t108**.
[The handoff census](native_emulator_path_launchers.csv) started with **200 command
lists in 185 files**. After t108 batch 1 below, **198 command lists in 183 files**
remain. These are command-list
literals whose first element is `dosbox-x`, including lists assigned to a
variable before launch. It is an inventory of remaining PATH selection, not a
claim that every listed harness lacks every metadata field. Dynamically built
commands need separate inspection in t108. The three PATH launchers changed
above now use the helper's resolved path and therefore leave this list.

## Measured identity and receipts

Measured 2026-10-25, on base
`4f31185fd592847c0297c793b4340cefe8d2848a`:

- Version: `DOSBox-X version 2026.01.02 SDL2`; version exit status **1**.
- Executable SHA-256:
  `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`.
- `pinned_build_used` is **false**. The reference source pin remains
  `5a9c7c53481c2e933948039c33256776653f4faf`.
- 0400: **194/194 RETURN**, zero flag masks, result SHA-256
  `904c8468127d352b9247636a38a7bc01ced8be0adb6c0b062e0d6ba78aa81a8e`,
  unchanged from [the prior receipt](OVL_0038_0400_CPU.md).
- Additional native cases: 006C **32**, 08D4 **42**, video mode **16**,
  SYSTEM text **25**, 137C **32**, and 13B1 **32**, all passing with the same
  recorded executable identity. Their existing comparison limits still apply.
- U6F wrapped return: **15 frames**, zero C mismatches. U77 wrapped return:
  **39 frames**, zero C mismatches, retaining its declared undefined-AF mask.

Local reports and generated programs are under ignored
`resources/t107-provenance/`. All executions use `poolrad-heavy.sh`.
The 0400 run redirects only the provider's output directory. The U6F/U77 test
fixture uses a real resources directory because bubblewrap cannot bind through
this worktree's resources symlink; its source/include links point back to the
tested worktree. This is test setup, not a change to the production launchers.

Seven focused unit tests cover executable/symlink resolution, differing PATH
selection, version on stderr, missing/invalid probes, status-1 version output,
and conservative pin status. These tests use synthetic executable bytes and do
not require game media.
The full overlay suite passes **336 tests, 2 skipped**.

The canonical accounting audit reproduces **290** strict originals and queue
SHA-256 `ce7a5462da1f8f8b85c187c1d476a3b8fc8c4ddd0a6877ab5b5c638520ca3049`.
This task adds **zero** credit.

## t108 batch 1: U9E sidecar providers and wrapped-launcher diagnostics

Base: fresh main `c5e9f5d84da641880ce2c70cff6bcedd30430cc8`, measured
2026-10-25. The first family is the two closely related U9E sidecar providers,
`u9e_sidecar_0160_cpu_native.py` and `u9e_sidecar_groups_02fa_cpu_native.py`.
They share deterministic CPU14/stack/data/node comparisons and the same report
structure, making a small first migration whose existing results can be
compared exactly. Both now resolve PATH through the t107 helper, launch its
returned executable, and attach its `dosbox_x` metadata.

| Provider | Native cases before / after | RESULT.BIN SHA-256 before = after |
| --- | ---: | --- |
| U9E:0160 | 113 / 113 | `ae56a0ca8b57e07921f65e169aaf7fa24aa458fd54456c633df75239e1c77921` |
| U9E:02FA | 77 / 77 | `d75264b1686c8b44f6a74f57416ad35586fdca9cdaa7a0a14d433bc64a89dbc6` |

After removing only the new `dosbox_x` field, each entire before/after report
is identical. The existing boundary traps, stop counts, comparison windows,
zero flag masks and original body hashes remain unchanged. Both identify the
same DOSBox-X version/hash and unpinned status recorded for t107 above.

The U6F and U77 wrapped-return launchers now include the process exit code,
log path and final 4,096 log characters in their failure exception. Real runs
through this worktree's resources symlink confirm that bubblewrap's bind
failure is visible in the exception. This improves diagnosis; it does not
change the bind or automatically repair the symlink layout.

Successful runs using an isolated real resources directory also pass:
**U6F 15 frames**, **U77 39 frames**. Their entire reports, including every
capture hash, match the saved t107 baselines. All four touched native harnesses
were rerun. The seven provenance tests and static name-binding checks for the
four changed files pass.

Local before/after reports, expected failure logs, and isolated wrapped-return
fixture are under ignored `resources/t108-batch1/`. The other **183** files in
the remaining PATH census were **not rerun or migrated** in this batch;
dynamic command construction still needs separate inspection as noted above.
No C, credit, identity, membership, manifest, or REPLACED data changes.

The four batch-1 native companions carry explicit `NOT_A_ROW_CHECKER`
markers naming their owning audit. The U9E modules are imported by the audits
under `--native`; the U6F/U77 wrap programs are supplemental directed native
instruments. These comments correct the pairing gate classification without
changing executable Python, row registration, or native measurements.
