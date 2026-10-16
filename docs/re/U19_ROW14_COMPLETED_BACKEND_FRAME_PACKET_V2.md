# U19 row-14 supplied completed-backend RGB24 packet v2

This source-only gate validates a **supplied** canonical 640×400 RGB24 byte
buffer and a small manifest. It does not capture a backend frame, authenticate
the declared completion event, join the selected CPU store to a frame, select a
display page, or compare an observed image with the expected row-14 crop.

A bounded `deja` lookup hit the local read-only index lock. PM canceled the
elevated retry under host load; no indexed session finding was reused. The
design reuses the checked-in
[completed-frame source boundary](U19_ROW14_RGB24_COMPLETION_CROP_PRODUCER_SOURCE.md),
[same-run join contract](U19_ROW14_SAME_RUN_JOIN_CONTRACT.md), and
[typed visual witness](U19_VISUAL_WITNESS_QA_CONTRACT.md). The private
DOSBox-X source revision remains
`5a9c7c53481c2e933948039c33256776653f4faf`; original source copies,
backend buffers, and RGB images remain under ignored `resources/`.

## Exact supplied input

`parse_completed_backend_frame_v2(raw_manifest, rgb24)` checks the original
UTF-8 JSON manifest bytes, at most 4,096 bytes, as one object with no
duplicate keys at any nesting level. The separate `rgb24` argument is the
**actual supplied byte buffer**, at most 4 MiB. The object API
`validate_completed_backend_frame_v2(record, rgb24)` checks the same fields
and a compact manifest extent; callers with raw input must use the parser so
the original wire bound and duplicate-key rule apply.

The manifest has exactly these fields:

| Field | Validation |
| --- | --- |
| `version` | Exact integer `2`. |
| `selected_event` | Exact nested eight labels: `run_token`, `arm_token`, `stream_id`, `plane_event_id`, `raw_sha256`, `candidate_ordinal`, `pc`, `successor_pc`. Their token, hash, 1..2304 ordinal, and paired `0F2D` PC bounds match the passive v2 packet. They remain unauthenticated declarations. |
| `backend_event_id`, `backend` | Separate bounded tokens. The backend event ID is not inferred equal to the plane event ID. |
| `completion_kind` | Exact string `backend_complete`, a supplied declaration rather than proof of a reached backend. |
| `width`, `height`, `row_stride` | Exact integers; width 640, height 400, stride at least 1,920 bytes. Row padding is allowed. |
| `pixel_format`, `channel_order` | Exact strings `RGB24` and `RGB`; the parser cannot prove the source backend used that order. |
| `rgb_byte_length`, `rgb_sha256` | Exact integer length and lowercase SHA-256, both checked against the actual supplied bytes. Length must equal `row_stride × 400` and remain within 4 MiB. |

The immutable `CompletedFrameV2` result retains the eight selected labels,
backend event ID, layout, computed digest, and supplied bytes. Its status is
always `SUPPLIED_FRAME_NO_NATIVE_BACKEND_ASSERTION`. The parser rejects
`frame`, `real_start`, palette, phase, and clock fields as extras because no
concrete v2 host event producer currently sources them. p5's v2 producer
audits only the selected CPU stream; the older `HostWitness` is a protocol
for the 1..128 route, not a v2 completed-backend producer.

## Runtime handoff still required

A future native producer must pin the selected backend and its runtime pixel
format, pitch, bytes-per-pixel, channel masks/shifts or OpenGL upload format,
clip, scaling/aspect path, and actual conversion to canonical RGB24. It must
also source a frame/backend event ID, latched `real_start`, palette identity,
and a common run-wide host event clock showing CRTC sample, latch,
renderer output, backend completion, and RGB capture in order. A prior-frame
completion inside the latch callback or an end-update entry without output
cannot stand in for the captured frame. The selected CPU store has no common
host clock in the current source-only packets, so this validator cannot assert
its order relative to renderer end.

pC's later join must audit p5's actual recorder stream, compare all eight
declared selected labels, and bind both distinct event IDs to a sourced
same-run host witness. Only a sourced frame, page, palette, decoder,
scanline/scaler geometry, and backend conversion can establish which host
pixels cover the expected `(16,224,576,16)` crop. p9 owns that source map
and independent crop comparison. No native frame, first writer, page/RGB
parity, C parity, or full-frame credit follows from this packet.

Focused synthetic verification:

```sh
python3 -m unittest \
  tests.replay.test_cold_start_codeword_row14_completed_backend_rgb24_v2
```
