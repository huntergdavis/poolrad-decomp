# RES00BA native emulator provenance (t108 batch 4)

The native companions for `00BA:17E3`, `00BA:1693`, and `00BA:1816`
resolve the selected emulator once through the shared provenance helper,
launch the returned path, and attach `dosbox_x` to their reports. Their
existing cases, recovered C, comparison windows and masks are unchanged.
The companions are explicitly marked `NOT_A_ROW_CHECKER`; row registrations
remain in their existing audit entry points.

Native measurement base: fresh `origin/main` at `c318fd474d3736e38b277eef4ab4074f4e7945ad`.
The branch was refreshed onto `86c987c6c4bc101a74e240c759e2ef44ed4b12c4`;
measured provider and C/header hashes remain unchanged.
Before and after native runs use isolated output paths. Complete reports
match after removing only `dosbox_x`; every RESULT.BIN hash is unchanged.
Provider and recursively included C/header hashes are recorded privately.

| Fixture | Cases before / after | Compared OUT events per phase |
| --- | ---: | ---: |
| `res00ba_clear_17e3_cpu` | 61 / 61 | 1505 |
| `res00ba_display_1693_cpu` | 43 / 43 | not captured |
| `res00ba_display_1816_cpu` | 44 / 44 | 4760 |

Unchanged capture SHA-256 values:

- `res00ba_clear_17e3_cpu`: `1b7100eeb0daaa42e8779e1fe10de3ac94af9887ae50a673f591072191360fdc`
- `res00ba_display_1693_cpu`: `1d133a3dc40de451f3aa778fa0abe7767c18051fcc3f36b06b977f0799450d54`
- `res00ba_display_1816_cpu`: `c914b38f56168a1357a01b4910c74a323e6227c8df74fddb8435b16a5a130734`

All three fixtures compare CPU14, 65,536 DS bytes, and 262,144 EGA plane
bytes with zero flag masks. The clear fixture captures 256 stack bytes;
the two display fixtures capture 1,024 stack bytes each and also compare
65,536 font bytes and 65,536 string bytes. The `1693` fixture reports
its masked backing-RAM ranges explicitly and does not capture native
controller state. These are existing bounded native comparisons, not
full-RAM or natural-game-route evidence. Existing guest INT1 OUT tracing
and capture traps remain unchanged.

Executable: `DOSBox-X version 2026.01.02 SDL2`, version exit status
`1`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`,
`pinned_build_used: false`. Identity was checked before and after each phase.
A source revision or release banner does not certify the installed build.

On the stated base the launcher census is 198 command lists in 183 files;
removing these three launchers leaves 195 command lists in 180 files.
These are branch-specific counts; other pending batches can change the
aggregate independently. Synthetic test command lists are excluded.

Receipts and generated programs remain under ignored `resources/t108-batch4/`.
Every native run uses `poolrad-heavy.sh`. No C body, identity, membership,
manifest, strict-credit or replacement data changes.
