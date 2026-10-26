# DOS save no-op reader/writer gate (t174, step 2)

`tools/character/disk_save.py` imports the game's complete serialized files
into immutable byte buffers and exports them into a **new, separate copy
directory**. It implements no semantic edits. The original source is never
opened for writing. Every successfully exported game file has an entry in
`write-receipts.json`, including the exact output path, source, before/after
SHA-256, changed offsets and before/after bytes (both empty for no-op), and
`game_build_loaded: null` / `UNVERIFIED` until an actual game load occurs.

The parser recognizes uppercase `CHRDAT*.SAV`, `.ITM`, `.SPC`, and
`SAVGAM*.DAT` names using the restricted ASCII spelling in its filename
pattern. It checks 285-byte character and 13,137-byte world frames, 63-byte
item and 9-byte spell-sidecar multiples, sidecar pairing, and occupied world
basename references. Missing optional sidecars are preserved as absent.
Other files are outside this export format and are not copied. This is a
save-set export, not a full game installation copy.

The reader retains every byte, including unused basename slots, name tails,
unknown character fields, and stale far pointers. Unlike the name accessor
in the existing C API, this raw export path does not require a semantically
valid name length. A correctly sized record with an uninterpreted name byte
can round-trip without being accepted as a valid game character. Structural
acceptance is not gameplay validation.

## API and safety boundary

`DiskFile.read(path)` owns the complete serialized bytes and validates the
file's frame. `serialize()` returns those exact bytes without running game
load fixups. `import_directory(source)` checks the set's character/sidecar
and world references before any output is created.
`export_directory(source, destination)` returns output-copy and receipt paths
plus counts after rereading and comparing each output. It also rechecks the
source snapshots before reporting success.

The destination must not exist, must have an existing parent, and must be
outside the resolved source directory. Existing and broken symlinks at the
destination are refused, and a parent symlink into the source is detected
by path resolution. Files are created exclusively. A failure rolls back
only the files and directory created by that invocation. A terminated
process or host I/O failure can still leave an incomplete private copy;
the command must complete successfully with its receipt before using a copy.
The implementation assumes ordinary local filesystem ownership; it does
not promise protection against an adversary concurrently replacing path
components. It does not lock a game that is actively writing its saves:
use stable captured inputs and retain the independent source/output proof.

These mechanisms do not patch stats or permit unvalidated semantic writes.
They do not assemble a new party, reconstruct missing files, interpret
spell IDs, or update runtime counters. Those remain separate gates.

## Measured original corpus gate

The supplied DOS v1.3 source remained read only. Output copy:
`resources/chared-t174/roundtrip-noop/`. Private receipts are in that directory;
independent per-file hashes and comparisons are in
`resources/chared-t174/roundtrip-proof.json`.

| Independent measurement | Result |
| --- | ---: |
| Character files | 12 |
| Item sidecars | 12 |
| Spell sidecars | 10 |
| World files | 2 |
| Output files compared with original source | 36 |
| Bytes compared | 31,656 |
| Differing files | 0 |
| Original files still matching pre-investigation hashes (includes executables) | 38 |
| Game loads performed | 0 |

For each side, sort all 36 paths by basename, form ASCII lines
`basename + " " + decimal_size + " " + sha256(file_bytes) + "\n"`, concatenate,
and SHA-256 the result. Independently calculated values:

```text
input  781c8c827f4566b24b31b440596239e743588fdc5b593951c86d98a3be0fff19
output 781c8c827f4566b24b31b440596239e743588fdc5b593951c86d98a3be0fff19
compared_files=36 compared_bytes=31656 different_files=0 originals_unchanged=38
```

DOS build inputs (not the DOSBox-X host build):

- `START.EXE`: `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
- `GAME.OVR`: `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`

The exporter itself does not require game executables or assert their
identity. The above build provenance was checked separately against the
original corpus. No original bytes or character names are published.

## Reproduction

Choose a stable input directory and a destination that does not exist:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests/character -v
python3 tools/character/disk_save.py "$private_source" "$new_copy"
```

Independently compare the files without importing exporter code:

```sh
python3 - "$private_source" "$new_copy" <<'PY'
from pathlib import Path
import sys
source, output = map(Path, sys.argv[1:])
files = sorted(p for pattern in ('CHRDAT*.SAV', 'CHRDAT*.ITM',
                                 'CHRDAT*.SPC', 'SAVGAM*.DAT')
               for p in source.glob(pattern))
if not files:
    raise SystemExit('FAIL: no input files')
size = 0
for path in files:
    original = path.read_bytes()
    if original != (output / path.name).read_bytes():
        raise SystemExit('FAIL: output differs')
    size += len(original)
print(f'compared_files={len(files)} compared_bytes={size} different_files=0')
PY
```

Nine synthetic tests passed. They exercise exact preservation and receipts,
existing-copy refusal, source/descendant refusal, symlink resolution,
malformed frames, absent referenced characters, opaque name/pointer bytes,
failed-write rollback, and absent optional sidecars. No heavy job was started.
`ran run_media_checks.sh: not run`; targeted tests and the original-save
round trip are the checks for this change.

## Reuse and remaining validation

This reuses the measured framing and load/save boundary in
[DOS_CHARACTER](DOS_CHARACTER.md), [DOS_SAVGAM](DOS_SAVGAM.md), and
[SLOT_A_PARTY_INPUT](../re/SLOT_A_PARTY_INPUT.md). The task's initial Dunking
Sheep recall found prior DOS save sessions
`01a12855-addb-78d1-ad73-89ae2590dc75` and
`01a12b3e-8cea-7de2-bfd1-ba3a8310754f`; those linked repository artifacts
provided the evidence. The independent layout investigation is PR #1589.
This PR is based directly on main and has no code dependency on #1589.

NOT VERIFIED: game-facing stat/level/HP/ability/spell meanings or valid edit
ranges; any edited value loaded by the owner's game; party assembly; web UI.
The byte-identical no-op gate is green for this held corpus only. VP requires
a Play-pane slot before the subsequent controlled edited-copy game proof
(inbox i26449). No game has been started by this change.
