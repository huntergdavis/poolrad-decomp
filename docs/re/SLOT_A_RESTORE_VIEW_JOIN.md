# Saved-A restore/view data join and native close gate

This distinct replay join starts from a world-save file and an already
unpacked GEO block. `pr_slot_a_restore_view_prepare` calls the existing
`pr_slot_a_native_restore`, then feeds that same candidate world into
`pr_slot_a_view_prepare`. It publishes the prepared inputs only after both
calls succeed. Save rejection, mismatched GEO ID, invalid GEO size or party
position leave the destination byte-identical.

Preparation is a data operation. It does not execute the DOS loader or
render a first view. `pr_slot_a_restore_view_gate` accepts these inputs as
const and returns **BLOCKED_CLOSE_1491** without changing them. The first
unsupported native call after the ten transferred spans is unit
`0x6F:+0x18E5 -> SYSTEM 0x0AF8:0x1491`, returning to `+0x18EA`.
Its actual local file handle at `SS:[BP-0x1C8]` is not present in the raw
save transfers. The gate exposes its requirement and caller-frame offset;
it creates no handle or successful-close token.

## Provenance and branch ownership

The preceding PR #376 head was recorded as
`39bcf90ca8cbbabbe293b0fde73661323c740dfd`. This task uses fresh branch
`codex/restore-view-join`, based on main `1e1b13a`, and has no dependency on
PR #376. Before implementation,
`timeout 10s deja "poolrad saved A native restore post transfer first view
18E5 close 191E blocked composition"` expired while recall remained
unavailable; no prior-session result was reused.

Reused repository evidence: [raw save transfers](SLOT_A_NATIVE_SAVE_ADAPTER.md),
[post-transfer caller](SLOT_A_POST_TRANSFER.md),
[party-prefix handoff](SLOT_A_PARTY_ENTRY.md),
[GEO buffer copy](U3BD_WALLSET_ENTRY7.md), and the previously captured
[first-view CPU/RAM boundary](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md).
The existing static-view tests supplied synthetic fixture layout. No key,
art, UI, recovered implementation, or shared CMake file is edited.

## Native data boundary

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| `GEO2.DAX` | 2572 | `2035246014efdb26de3114f4737d52a935ad8834f79b4a8e4880da2922b6d595` |
| First-view `+3382` RAM | 655360 | `31424ea33ec4abce1967fff7032be17b95d4b70b4cfab0d9624c29a54e3072c3` |
| Numeric CPU transcript | hash pinned | `4c7f2193e77befaf44464fd3f427bc96bf60d49d67f41b195af26a5112a899a8` |

The native snapshot and transcript are reused from ignored run
`slot-a-native-first-view-049`, copied into this clone's ignored resources.
They establish the actual first-view caller boundary `3582:3382`,
`DS=14A0`, `SS:SP=1D16:3EF6`, `BP=3EF8`. The harness checks that complete
CPU row, rather than generating a guest CPU tuple from file fields.

The existing C DAX decoder selects the directory ID from the saved map ID
and unpacks the 1026-byte block. Its 1024 bytes after the header equal the
entire native map buffer reached through `DS:[6A5C]`. The composed context
matches **seven native saved fields** (map ID, x, y, facing, and three clock
words), plus **nine native map-derived fields** (four walls, four door
fields, and event). No expected party tuple is assigned by the harness.
The gate leaves all prepared inputs byte-identical. Native RAM is a
read-only audit input; it is not passed to a guest-state emulator.

These comparisons verify shared input data at a later measured native
boundary. They do not prove that C executed the intervening native loader,
that a native close returned in this harness, or that a first-view call ran
in C. No pixel parity, playable one-Up, or full-C count follows from them.

The close setup/call span `+18DF..+18E9`, file `0x1F8C5..0x1F8CF`, has
11 bytes and SHA-256
`d0cbac4e6333d378dfbf3dd28383b3415e4c09038e9d03027a9fd9681d4ef63f`.
It forms `SS:BP-0x1C8` and far-calls `0AF8:1491` at `+18E5`.
The corresponding loaded span at `2A8E:18DF` hashes
`ada4db0674dcc5e36dd76e191df15bd07d6ca9a0364cf8354237572881dddb65`;
only the target segment differs, relocated to `131C`. A later RAM image
containing that code is evidence of loaded identity, not a close-call trace.

## Queued downstream dependency

After a real close, the existing post-transfer step begins at `+18EA` and
the existing party-entry prefix begins at `+191E`. The gate does not run
either through the unresolved close. For a nonzero saved count it queues
the next external call: `0x6F:+1965 -> SYSTEM 0AF8:0B0B`, return `+196A`,
source `CS:15E7` and destination `SS:BP-0x214`. The literal and live frame
must come from the host; their bytes are never embedded in this code.
For a zero count it queues no call and reports prefix boundary `+19F7`.
The queued call is a dependency, not an execution result. Its original
five-byte SHA-256 is
`7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae`.

The future first-view target remains the measured caller
`0x1C:+3382 -> unit 0x3B4 entry 1 (+016D)`. Loader, party/sidecar,
post-load and gameplay initialization effects must be resolved before a
native-route join can reach it. Raw-save key dispatch also remains blocked.

## Checker verdict and reproduction

- `target_function`: file-derived restore/view composition and its first
  missing native call contract.
- `source_candidates`: `slot_a_restore_view_join.c`, composing the existing
  native-save adapter and static-view preparation function.
- `signals`: exact input hashes; complete native map equality; saved/context
  field agreement; captured CPU provenance; original/loaded call identity.
- `score`: five checked signal categories, not a confidence probability.
- `verdict`: `match` for measured data preparation; `unclear` for native
  execution from the save transfer to the first-view call.
- `review_notes`: map-derived fields share one map input; CPU/RAM capture is
  a later native stop, not a new trace through the unresolved close. No
  callee side effects or identity counts are inferred from input agreement.

`bash tests/replay/check_slot_a_restore_view_join.sh` runs focused strict
C11 tests for failure atomicity, blocked-state immutability, and counts
6, 0, and 255 (including invalidation of the previous queued call).
Passing five ignored paths (`GAME.OVR SAVE GEO_DAX VIEW_RAM CPU_TSV`) adds
the source/input hashes, CPU provenance, and native data comparison.
Shared CMake registration is coordinated with the integration owner.
