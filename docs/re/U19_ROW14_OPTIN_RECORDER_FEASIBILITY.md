# U19 row-14 complete candidate recorder feasibility

This is a **source-only design gate** for an opt-in, complete selected-PC
candidate transcript. It supplies no native capture, first guest writer,
four-plane delta, page, RGB, C parity, or full-frame result. The native host
slot remains held. A bounded `deja "Poolrad U19 row14 complete selected-PC
candidate recorder 2304 arm_token four plane authenticated capture"` query
timed out after eight seconds without a result. This audit reuses the merged
[candidate capacity gate](U19_ROW14_CANDIDATE_COVERAGE_GATE.md) and its
conditional 36-cell C source model; it does not reuse an unverified session.

## Measured capacity and existing contract

On the conditional accepted EGA route, 36 cells times eight glyph rows times
four planes times two page stores yields **2,304** executions of the four
printer PCs `+0412/+0438/+0445/+0451`. The first possible `A000:118F`
candidate is ordinal **897** if the first page maps to `A000`, or **898** if
only the second page does. These are source bounds, not observed native
visits. The existing 128-hit target filter and selected-host event contracts
remain unchanged. Their hit count includes every *supplied* one of the four
PC frames after U19 `+02D8`, even with the wrong `ES:DI`; it is neither a
target-only count nor a count of all guest writes.

The current compact four-WORD JSONL frame is 50 bytes with canonical compact
JSON. A 2,304-candidate stream is about **115,200 bytes** at that size. The
merged reader caps each frame line at 256 bytes and the whole route at 2,307
records, at most **590,592 bytes** under those two bounds. The record count
allows caller, candidates, one successor, and return. Host CPU cost and
native route length remain unmeasured. Worst-case 64-character run, arm, and
stream tokens plus a source hash make a proposed inline start record 349
bytes; a selected-event record with raw hash and plane event ID is 427 bytes.
Those metadata records therefore need a separately bounded sidecar rather
than the existing 256-byte frame JSONL.

## Pinned debugger and CPU-core boundary

The ignored DOSBox-X source is revision
`5a9c7c53481c2e933948039c33256776653f4faf`. Hashes of the files read:

| Private source | SHA-256 | Relevant source lines |
|---|---|---|
| `src/debug/debug.cpp` | `2762251108e54aeec3474b9f495222ce09bec9f246dd0af53a4d10fa7bb2a38d` | `LOGL` setup 2773–2793; long CS:IP and ES:EDI record 5430–5502; flush/close 6011–6019 |
| `src/cpu/core_normal.cpp` | `0a758afabe88992b69b4cd35d2a2ab3d1d84ad53e73e652744c3a333e8139e95` | `DEBUG_HeavyIsBreakpoint` before each interpreted instruction at 245 |
| `src/cpu/core_dynrec.cpp` | `d5135931ebe60a9a48aafd8b01051be13e2679c5faced133c5443d7bdcd7a7cd` | debug callback occurs on dynamic-core paths, including block/trap handling |

`LOGL` counts **instructions**, not selected-PC visits, and closes after its
finite instruction count. It has no measured route-sized count or automatic
U19 return boundary. The private configuration says `core=auto`; a dynamic
core callback is not established as one callback per guest instruction.
Consequently the existing debugger cannot authenticate a full 2,304-visit
selected-PC transcript. A future native recorder must pin its actual CPU
core or independently prove coverage at each selected execution, record a
start and U19 return with an overflow/truncation result, retain a hash of the
raw artifact, and identify the source build and run. A bounded offline
shape check alone cannot certify that an omitted repeated visit was logged.

## Proposed opt-in separation pending owner alignment

Keep legacy cap-128 consumers untouched. A new default-off collector would
count a one-based `candidate_ordinal` for **every** reached one of the four
printer PCs after the U19 call, including wrong `ES:DI`. The selected store's
immediate successor is a separate nonincrementing frame. Candidate 2,305,
missing return, interrupted recording, and any write/flush failure are
typed failures. Do not use a total of 2,304 as the selected target's ordinal.

Keep private metadata separate from the bounded frame JSONL. A manifest can
bind `run_token`, `arm_token`, `stream_id`, pinned source/build identity, raw
artifact SHA-256, candidate count, and terminal state. A selected-event
sidecar must bind the same run, arm, stream, and raw hash to the **selected**
candidate ordinal and `plane_event_id` token. That event is only a CPU-source
selection until a same-run four-plane packet and actual native stop are
associated. Exact field types and packet paths remain under pC/pR/pN review;
no shared schema or native source has been changed by this note.

An independent pN synthetic policy review (private manifest SHA-256
`91967e43b01a8a13869485191af9accde2ae7f80fd997cf9651edc9a770ae474`)
confirmed the 2,304-candidate/2,307-frame source shape, 897/898 first target,
and legacy stop at candidate 129. It did not supply native counts or approve
implementation. The proposed versioned validation policy is:

* `full_return` requires one caller, 2,304 ordered candidates, exactly one
  immediate selected successor that does not increment the candidate count,
  one first return, and no frame after that return. A `selected_prefix`
  terminal can identify a selected CPU source but cannot claim full-route
  coverage.
* Candidate 2,305 is an overflow failure. Since the 2,307-frame cap has no
  room for a 2,308th frame, record overflow in separately bounded metadata
  and fail closed without appending that frame. Missing return, interrupted
  capture, source write failure, and metadata truncation also fail closed.
* Recompute raw artifact SHA-256, record count, and first target ordinal from
  the bounded frame stream. The selected-event sidecar must bind the same
  run, arm, event, stream, raw hash, and *selected* ordinal. A token or hash
  alone does not establish native same-run provenance.
* The new `candidate_ordinal` is not a relabeling of the old
  `store_ordinal`. The existing cap-128 pC event join cannot consume ordinal
  897/898; a distinct versioned join needs separate owner review.

## Local source-only recorder model

`tools/replay/cold_start_codeword_row14_selected_pc_recorder.py` now provides
an explicitly enabled in-memory `CandidateRecorder` and an
`audit_recording` parser. The default is disabled. Its input is a **supplied**
sequence of relevant caller, candidate, selected successor, and return CPU
frames; an opt-in argument must be an exact boolean. It has no emulator hook
or file writer. The raw artifact is canonical
four-WORD JSONL, at most 2,307 lines of at most 256 bytes each. The separate
version-1 manifest is at most 4,096 bytes and identifies the SHA-256 of the
exact raw `src/debug/debug.cpp` source-file bytes, as well as the SHA-256 of
the complete raw JSONL byte string including final newlines. The selected
event repeats the run, arm, stream, plane-event, raw hash, and first target
ordinal. It is emitted only after its immediate successor was supplied.

The parser checks canonical frames and recomputes the source hash, raw hash,
counts, first target, successor, and terminal. A route with no `A000:118F`
target returns `STOP_TARGET_PAGE_ABSENT`, even after all 2,304 candidates
and a return. Manifest version and selected-event fields require exact types;
Python boolean or floating-point equality cannot stand in for an integer.
A successful full route with a paired selected event returns
`SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED`; a selected prefix returns
`SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED`. Overflow and truncation return
typed stops. Even a matching full-route transcript remains unauthenticated
until a pinned native instruction hook proves it supplied every visit.
Synthetic tests cover disabled mode, first target on either page, full and
prefix terminals, candidate 2,305 without a 2,308th frame, truncation, wrong
successor, after-end input, and manifest/source/raw tampering:

```sh
python3 -m unittest \
  tests.replay.test_cold_start_codeword_row14_selected_pc_recorder
```

All raw logs, CPU/RAM/EGA packets, and pixels remain under ignored
`resources/`. Public output is limited to addresses, hashes, counts, and
synthetic records.
