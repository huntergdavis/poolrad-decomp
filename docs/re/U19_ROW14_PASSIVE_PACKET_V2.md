# U19 row-14 source-only v2 four-plane packet

This gate validates one supplied packet for the selected `A000:118F` byte
write. It adds no DOSBox-X hook, native plane bytes, first-writer finding,
displayed-page proof, RGB timing, or full-frame parity. The existing v1
packet, its 1..128 `store_ordinal`, and the legacy route filter are unchanged.

A bounded `deja "U19 row14 v2 four plane packet selected_event stream_id
raw_sha256 candidate_ordinal source only"` lookup could not open the local
read-only index lock. This gate reuses the checked-in
[v1 plane contract](U19_ROW14_PASSIVE_HOST_SNAPSHOT.md),
[complete candidate capacity gate](U19_ROW14_CANDIDATE_COVERAGE_GATE.md), and
[v2 producer alignment](U19_ROW14_V2_PRODUCER_ALIGNMENT.md). The conditional fixed-row path has
2,304 candidate-PC visits; the first possible target is ordinal 897 or 898
by page. These are source bounds, not native visit counts.

## Packet and validation boundary

`tools/replay/cold_start_codeword_row14_passive_snapshot_v2.py` exposes
`parse_capture_v2(raw_packet)` for actual private UTF-8 bytes and
`validate_capture_v2(record)` for an already parsed synthetic object. The raw
entry checks a single JSON object of at most 4,096 bytes and rejects duplicate
keys. The object entry checks the exact key set and a compact serialized bound;
callers with raw input must use `parse_capture_v2` to enforce its original
byte extent.

The packet has exact integer `version=2` and these eight **flat** selected
identity fields, matching p5's nested `selected_event` names:

| Field | Required form |
| --- | --- |
| `run_token`, `arm_token`, `stream_id`, `plane_event_id` | Separate `[A-Za-z0-9_-]{1,64}` strings. |
| `raw_sha256` | Exactly 64 lowercase hexadecimal digits. |
| `candidate_ordinal` | Exact integer 1..2304; every supplied candidate PC counts, including wrong `ES:DI`. |
| `pc`, `successor_pc` | Canonical uppercase `WORD:WORD` strings at `0F2D`, with one of the four pinned site/successor pairs. |

Other exact fields are `probe="host_backing_word"`,
`extent="one_byte_per_plane"`, fixed witness `(122,112,A000:118F,bit 20)`,
measured `host_map`, actual `handler_index`, `write_count=1`, and `before` and
`after` samples. Each sample carries exactly four plane bytes encoded as eight
uppercase hex digits plus states immediately before and after the copy. Each
state has CS/IP/ES/DI, ten GC bytes, six Sequencer bytes, and one latch DWORD.
The checker requires state equality around each copy, sampled frame equality
across the write, selected CS and `ES:DI`, and either the selected site IP or
its paired successor IP. It resolves the final host backing index independently.
Pinned DOSBox-X revision `5a9c7c53481c2e933948039c33256776653f4faf`,
`include/vga.h` SHA-256
`93a5a1cd5f891a6637f6807f728a5bbbca914e15fc43fb3e841dd065fec3c464`,
declares `vga.seq.memory_mode` and `vga.gfx.miscellaneous` as `uint8_t` at
lines 504 and 562. V2 rejects values outside 0..255 even when the fast
handler's index calculation would ignore those registers.
It does not require a plane delta: a selected write can leave bytes unchanged.

`SnapshotResultV2` retains all eight identity fields and derives integer
`site`/`successor` for pC. p5's separate version-2 producer envelope has a
nested selected event and audited source, count, and terminal provenance.
The pR packet does not duplicate `source_sha256`, terminal, status, or clock
fields. Equality of self-reported tokens or hashes is not native provenance.

## Separate follow-ons

1. pC's versioned join must call p5's `audit_recording` on the actual raw
   candidate stream, manifest, and pinned source bytes, accept only the two
   unauthenticated selection-bearing statuses, then compare all eight
   selected fields with this packet. It must retain full-return versus
   selected-prefix status and audited source/count/terminal provenance.
2. A later native adapter must establish immutable run/build/decoder/arm
   identity, reset and rearm invalidation, actual callback/index attribution,
   and stale-arm/cross-run negatives. Any measured host clock needs an
   explicit source and phase; latch-entry and store-time clocks are separate.

Neither follow-on is implemented by this packet. The private packet stays
under ignored `resources/`. The synthetic gate is:

```sh
python3 -m unittest \
  tests.replay.test_cold_start_codeword_row14_passive_snapshot_v2 \
  tests.replay.test_cold_start_codeword_row14_passive_snapshot
```
