# Saved-A replay caller-input provenance

The canonical [hash/address table](SLOT_A_REPLAY_INPUT_PROVENANCE.json) contains
five measured input rows, one original-produced static endpoint, and five
synthetic rows. It tracks identities and lineage only: no CPU/RAM values,
register dumps, fixture payloads, media bytes or private filesystem paths.
This adds no execution, parity, full-function, count or playable evidence.

| Row ID | Exact loaded PC | Classification | Evidence boundary |
| --- | --- | --- | --- |
| measured_d8f_baseline | 2A8E:0D8F | measured | Existing baseline original capture |
| measured_d8f_file_absent | 2A8E:0D8F | measured | Controlled file-absence original capture; not baseline |
| measured_allocator_entry | 131C:023F | measured | Actual wrapper entry; allocator body excluded |
| measured_allocator_first_return | 131C:0248 | measured | Actual first return input; not a C allocation result |
| measured_ccc_seed | 2A8E:0CCC | measured | Existing native CCC input only |
| static_ce1_original_endpoint | 2A8E:0CE1 | original_produced_static | Prior original-byte CCC->CE1 report; endpoint snapshot not retained |
| synthetic_ccc_controlled_input | 2A8E:0CCC | synthetic | Controlled modifications to the measured seed |
| synthetic_ce1_input | 2A8E:0CE1 | synthetic | Independently initialized C fixture |
| synthetic_ced_call_input | 2A8E:0CED | synthetic | C-only caller-frame fixture |
| synthetic_b0b_return_input | 131C:0B20 | synthetic | Fresh independent return-edge fixture |
| synthetic_cf2_return_boundary | 2A8E:0CF2 | synthetic | C-only result of that return fixture |

CCC's existing seed is measured. Its later CE1 endpoint is static evidence,
not newly observed native output. The static endpoint has no retained CPU/RAM
snapshot, so the table pins its seed hashes, source and execution-report hashes
without inventing an output-state digest. Synthetic CE1 fixtures are separate
from that original-produced endpoint. CED/CF2 fixtures remain synthetic even
when their shape resembles an earlier native D8F frame.

## Hash and lineage policy

Every measured row pins CPU JSON, full RAM and raw transcript hashes, with an
exact transcript row index and captured PC. The controlled D8F row explicitly
records file absence. The static endpoint pins its measured CCC seed and the
prior report/source; its source PC must match its parent. Synthetic rows pin
fixture source/test logs and any declared parents. Their state snapshots are
marked not_retained. Complete original executable/overlay/save hashes are
pinned once in the canonical table; original files stay ignored.

A measured row must be a direct native_capture witness with no derived parent,
a retained state snapshot and native-artifact hash fields. A C fixture or
static derivative cannot gain measured status by relabeling. Parent IDs must
exist; cycles and mismatched static seed hashes are rejected. Strict field
allowlists reject added value/dump fields. The validator checks internal
consistency, not the truth of arbitrary untrusted capture claims.

The optional private verifier reads only an ignored artifact manifest, checks
23 existing artifact hashes, and binds each measured CPU JSON to its pinned
raw transcript row and loaded PC. It executes neither C nor original code.
Static report/source pins attest the retained record of prior work; they do
not recreate lost endpoint snapshots or supply unknown runtime effects.
Allocator effects and the earlier25 changed bytes remain excluded. None of
the static or synthetic rows establishes a continuous allocator-to-caller join.

## Lightweight validation under host-load hold

~~~sh
nice -n 19 python3 tests/replay/check_slot_a_replay_input_provenance.py
nice -n 19 python3 tests/replay/test_slot_a_replay_input_provenance.py
# Optional read-only local artifact validation; manifest is ignored/private.
nice -n 19 python3 tests/replay/check_slot_a_replay_input_provenance.py \
  resources/replay-provenance-private-manifest.json
~~~

All11 rows and23 read-only artifact checks pass. The one requested consistency
test validates the real table, deliberately relabels the synthetic CED input
as measured, and confirms rejection. No emulator, Unicorn, heavy oracle or
push was used; the active PM/p5 slot and host-load hold are preserved.

## Reused evidence

Measured rows reuse baseline D8F evidence at00755c3, controlled zero-path D8F
at199280b, allocator wrapper/return at0eecc14, and baseline CCC seed at9ba56a8.
The static endpoint and controlled CCC fixture cite full source commit890c0ae;
CE1 C fixtures cite0f9bd45, and CED/CF2 fixtures citec7a06e6. Their exact full
source commits and artifact hashes appear in the canonical table. Existing
original byte executions are historical reports, not operations performed for
this table. The required low-priority deja query completed with no indexed
matches (zero sessions searched); these local records were read directly.

CCC/CE1/CED remain on their separately held exact heads. This provenance-only
branch does not restack them, implement runtime effects, alter publication
scope or modify CMake/shared code, printer/key, immutable PR410 map,0425 guards
or any function/identity ledger. New native or original-byte validation still
requires its own explicit PM slot and review; no row here grants that credit.
