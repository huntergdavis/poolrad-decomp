# U19 same-run visual witness QA contract

This offline gate checks supplied typed observations; it launches no emulator
and supplies no new native evidence. Recall was attempted before implementation
but its cache lock was denied and the PM declined a retry because the shared
index was unhealthy. No indexed finding was reused.

The contract reuses the [same-run CPU/store plan](U19_ROW14_SAME_RUN_EVENT_PLAN.md),
[latch and plane integrity conditions](U19_ROW14_PLANE_CAPTURE_INTEGRITY.md), and
[completed frame/backend boundary](U19_ROW14_PRESENTATION_EVENT_BOUNDARY.md).
The printer call is `38D3:02D8`, its return is `+02DD`, and a selected store is
`0F2D:0412/0438/0445/0451` with effective `ES:DI=A000:118F`. The existing strict
CPU frame filter chooses the store and immediate successor.

`tools/replay/u19_visual_witness.py` accepts typed in-memory plane, completed
frame and RGB24 records. Every record carries the CPU document's run token.
Before/after plane observations retain four 320-byte windows and passive
controller/latch state. State must stay identical across each observation;
the guest may change state between observations. Unknown/failed plane reads
and debugger-memory-read capture methods are rejected. Host state declarations
must ultimately come from a separately verified passive collector.
The merged [host snapshot gate](U19_ROW14_HOST_PLANE_SNAPSHOT_GATE.md) checks a
selected four-byte direct-backing read; its synthetic latch tests also run in
this CI job. It does not yet supply the complete 320-byte windows or host RGB.

One monotonic run-wide event clock orders selected store, successor, new frame
start, completed backend and RGB observation. The frame names the successor
plane observation and the RGB record names that frame. Plane/frame/RGB page and
palette identity must agree. A preassignment old-frame completion is rejected.
Equal page starts are allowed: the saved U19 endpoints have page indexes `0/0`,
so a page change cannot be a prerequisite for a new completed RGB frame.

The [independent expected row14 source](U19_ROW14_EXPECTED_RGB_CROP.md) is
**36 cells**, not the separate U17 459-cell map. Its numeric bounds are
288x8 source pixels, 802 foreground and 1502 black; doubled RGB24 crop
origin `(16,224)`, size `576x16`, 27648 bytes. `RGB24Patch` is page-neutral and
tightly packed RGB channel order; its origin is inclusive, with width/height
extents. `RGB24Observation` includes host width/height and byte row stride.
`compare_patch` checks bounds and returns differing pixel count, ignoring host
padding. Public tests use invented uniform colors and small synthetic arrays.
Expected text, fonts and derived RGB remain private under ignored `resources/`.

Run the focused, media-free checks:

```sh
python3 -m unittest tests.replay.test_u19_visual_witness tests.replay.test_cold_start_codeword_row14_event_plan tests.replay.test_cold_start_codeword_row14_host_snapshot_gate
```

The dedicated Python CI job runs that command without native builds or media.
Passing means internal consistency only. Matching run labels cannot authenticate
captures, establish coverage, prove the full renderer used the supplied plane
window, or establish native/C pixel parity. CRTC/retrace callback identity and
an original-source expected-patch comparison remain separate follow-on gates.
