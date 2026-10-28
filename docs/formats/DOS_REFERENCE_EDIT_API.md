# Reference DOS save edit API (t174 / t230)

`tools/character/edit_save.py` is a local JSON adapter for two historically
observed edits to exact reference records. It is not a general character
editor or advancement engine. The existing positional no-op interface in
`disk_save.py` remains unchanged. Interface agreement: design33 inbox i29513,
following proposal i29501.

Run `python3 tools/character/edit_save.py` with one JSON request on stdin.
Success exits 0 with one JSON object on stdout and no stderr. Refusal exits 1
with a JSON error on stderr and no stdout:

```json
{"schema_version":1,"error":{"code":"profile_mismatch","message":"..."},"applied_changes":[],"rejected_changes":[]}
```

On a parsed edit request, `rejected_changes` contains the requested changes;
none are applied when any preflight check fails. Malformed JSON has no parsed
changes. Duplicate JSON keys, unknown keys, unsupported fields/profiles,
boolean numeric values and positional CLI arguments are refused.

## Inspect before enabling edits

```json
{"schema_version":1,"operation":"inspect","source":"/private/captured-copy"}
```

The response includes `writes: 0`, `expected_source_hashes` for the complete
imported save-file set, `characters`, and `validation_scope`. Each character
has its basename and SHA-256, the six display-operand `current_values`, all
eight raw `level_slots`, and `reference_fighter_level` (null for unrelated
records). These reads do not establish valid editing ranges.

Each profile has `validation_profile`, `field`, `expected_before`, `value`,
`eligible`, `reason_unavailable`, `proof_scope`, and `evidence`. Enable only
an eligible profile. `eligible_after_application` names profiles that become
eligible after a successful application of that profile, allowing the
strength-then-level sequence to be presented explicitly. It does not grant
permission to apply an unavailable first profile. Unknown records remain
inspectable with edits disabled and a reason. Other stats, HP, abilities,
spells and party composition are not writable through this API.

## Ordered edits to a new copy

```json
{
  "schema_version": 1,
  "operation": "edit_copy",
  "source": "/private/captured-copy",
  "destination": "/private/exports/new-copy",
  "expected_source_hashes": {"CHRDATA1.SAV": "<hash from inspect; include every imported file>"},
  "changes": [
    {"file":"CHRDATA1.SAV","field":"strength","expected_before":17,"value":16,"validation_profile":"reference_strength_17_to_16"},
    {"file":"CHRDATA1.SAV","field":"level","expected_before":1,"value":2,"validation_profile":"reference_fighter_level_1_to_2"}
  ]
}
```

The example hash map is a placeholder, not a runnable request. The map must
contain exactly every recognized imported save filename and its current
SHA-256. Changes apply in array order, in memory, before any destination is
created. Each profile checks the **whole intermediate record hash**, its
before value and its exact after hash. Reversing the example fails. Duplicate
file/field changes, general offset writes and unsupported values fail.

| Profile | Sole allowed change | Before SHA-256 | After SHA-256 |
| --- | --- | --- | --- |
| `reference_strength_17_to_16` | STR byte `0x10`, 17 → 16 | `d7ba1321e1fca8464d1220e77c26ce92173d60cb67b0f155c71868926cfa6c8e` | `e29e16f7dfc4abb10ca25fc701166bab71f24e402276390fd6e202d8db5ccf5a` |
| `reference_fighter_level_1_to_2` | This fighter's level slot `0x98`, 1 → 2 | `e29e16f7dfc4abb10ca25fc701166bab71f24e402276390fd6e202d8db5ccf5a` | `92d1129e9b37bb26bd705ad6602a5eafad7d9e7aa9d86c8ce3e5559e8deefe6f` |

Historical evidence is [the strength display-field experiment](DOS_CHARACTER_DISPLAY_FIELDS.md)
and [the level experiment](DOS_CHARACTER_LEVEL_EDIT.md). Those documents
report game-loaded copies and sheet effects; this adapter does not rerun
those experiments. XP/HP/spells are not recalculated. A fighter byte changing
from 1 to 2 is not coherent advancement and establishes no general class
mapping, legal range, combat behavior or spell correctness.

## Copy and receipt guarantees

The source must be a stable captured directory. Recognized save-file symlinks
are refused. The destination must not exist, including as a broken symlink,
must be outside the source, and must have an existing parent. The writer
exclusively creates files in a new mode-0700 directory, rereads each output,
and re-imports the source to detect changes to both bytes and file membership.
On failure it removes only paths created by that invocation. This is not a
filesystem lock, crash-atomic publication protocol, or protection against a
hostile concurrent path replacement; do not point it at a running game's
mutable save directory.

Success returns `output_copy`, `files_written`, `bytes_written`,
`byte_identical_files`, `receipt`, ordered `applied_changes`, empty
`rejected_changes`, post-edit inspect metadata, and `validation_scope`.
`write-receipts.json` records each file's before/after hashes, changed offsets,
before/after bytes, byte-identical status and intermediate change hashes.
Unedited sidecars, world files and unknown record bytes are preserved.

Every newly exported copy has `game_validation: "UNVERIFIED"` and
`game_build_loaded: null`. Historical proof stays separately attributed to
its evidence document; matching a reference hash does not mean this new copy
was game-loaded or its party was accepted.

The desk backend resolves server-listed source IDs and a new copy name to
private paths, invokes Python with an argv array and JSON stdin, and keeps
private filesystem paths out of the browser. This CLI supplies no HTTP route,
authentication, upload/download handler or source-ID registry. The UI must
show which copy was written and the unverified state. Browser integration,
new game-load acceptance and arbitrary character editing remain t174/t230
work; this bounded API does not close either task. Test evidence belongs in
the PR/task receipt, not an unmeasured pass claim here.
