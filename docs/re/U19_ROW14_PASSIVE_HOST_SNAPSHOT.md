# U19 row-14 passive host-backed plane snapshot gate

This is a source-only capture contract and synthetic record checker. It adds no
native stop or EGA bytes. The first live selected `A000:118F` writer, displayed
page, host RGB event, and natural timing remain open. Heavy native is held.

Before this work, bounded `deja "Poolrad U19 A000:118F four plane host-backed
snapshot nonperturbing GC Seq latch capture"` failed because the local index
lock could not be opened on the read-only cache. The checked-in
[plane-integrity note](U19_ROW14_PLANE_CAPTURE_INTEGRITY.md),
[internal resolver](U19_ROW14_INTERNAL_PLANE_RESOLVER.md), and
[same-run event plan](U19_ROW14_SAME_RUN_EVENT_PLAN.md) supply the prior route,
read-path warning, and host index model reused here.

## Exact source boundary

The private DOSBox-X source is revision
`5a9c7c53481c2e933948039c33256776653f4faf`. Its
`src/hardware/vga_memory.cpp` SHA-256 is
`f506017948503fa22ce40715205293c987473a938247cbd09ba409ffbb652c14`.
The earlier private `P5FIRST` probe at lines 2491–2564 reads four consecutive
bytes from `vga.mem.linear + 4 * page_index`; it is a design reference for a
different address, not an U19 observation. Generic guest A000 reads at line
318 load `vga.latch.d`, so a debugger `MEMDUMPBIN` cannot be used to observe
the selected store and then resume without an independent latch restoration
proof. The generic and fast unchained write paths are at lines 331–385 and
580–665; both map through `bank_read_full`, and they differ in the final index
mask. The private `include/paging.h:431–433` physical-address helper only
reads the TLB array. `MEM_GetPageHandler` in `src/hardware/memory.cpp:852–866`
can take `MEM_SlowPath` and fill `memory.phandlers[]` on a miss. Thus a
stopped-state probe that queries the handler may alter a host cache even
though it leaves guest GC, Sequencer and latch untouched.
The merged [host snapshot source gate](U19_ROW14_HOST_PLANE_SNAPSHOT_GATE.md)
already has an ignored fake-host adapter with this stopped-state lookup; it
has not been integrated into a live DOSBox-X build. This gate uses an actual
in-handler index to avoid that extra lookup at the selected write.

## One-write capture mechanism for a future native slot

The printer FAR call is U19 `38D3:02D8`; its return is `+02DD`. It is not an
EGA store. Arm the private probe only after a separately authenticated drawer
candidate whose pre-instruction frame is `0F2D:0412/0438/0445/0451` and whose effective
`ES:DI` is `A000:118F`. Each site has one pinned successor:
`0412→0415`, `0438→043B`, `0445→0449`, `0451→0455`.

The current target filter's `candidate_hits` counts every **supplied** drawer
PC frame after the call, including wrong `ES:DI`, and caps at 128. Under the
conditional exhaustive accepted EGA path, 14 preceding cells × 4 planes ×
8 rows × 2 pages give 896 earlier candidate-PC executions, so the first
possible witness would be supplied-frame ordinal 897. This is a source bound,
not a native count. The present 128-hit exhaustive recipe is therefore a
typed capture blocker. A larger coordinated cap or authenticated sparse
selection must be reviewed before a native run; `store_ordinal` must not be
reinterpreted as target-only ordinal or total guest writes.

Inside the actual `uvga` or `uvga_fast` write path, use the handler's already
mapped backing index. Immediately before the backing-word write, copy the
four bytes from `vga.mem.linear + 4 * index` into private probe storage;
repeat immediately after. The hook must not call guest memory accessors,
emulated ports, `MEM_GetPageHandler`, or file/log output while the instruction
runs. Record the actual handler and index and all inputs needed by the
independent [resolver](../../tools/replay/cold_start_codeword_row14_internal_plane_resolver.py).
Reject an unsupported handler, out-of-range index, missing/duplicate target
write, wrong instruction successor, or resolver disagreement.

Bracket **each copy** with host reads of the full ten `VGA_Gfx` bytes, six
`VGA_Seq` bytes, `vga.latch.d`, and the sampled CS/IP/ES/DI frame. The two
brackets for one copy must match; the before-write and after-write brackets
need not match each other because the guest instruction runs between them.
The arm and readout debugger commands must verify the pre-store site and
matching successor. Export the small probe record only after reaching that
successor, in the same ignored run as the CPU/stack and RAM record. A host
write hook avoids a second translation/handler lookup at the observation
point; source review of the eventual private patch must still confirm the
hook's own writes are confined to probe storage.

The new [in-handler core](../../tools/replay/u19_row14_in_handler_capture.hpp)
is a fixed-size, allocation-free state machine. `arm` accepts only the four
selected pre-store PCs with `ES:DI=A000:118F`. `observe_write` accepts one
byte write at linear `A118F` from a supported unchained handler, copies the
four-byte word around the original write callback, and marks a duplicate,
wrong width/address/handler, out-of-range backing, or changed observation
state as a stop. The original write callback runs exactly once even when the
probe fails, so a capture error does not suppress that handler operation.
`finish` requires the matching successor and one captured write. The core is
tested against a fake host only; it is not yet installed into DOSBox-X. The
observed write frame must retain the armed CS/ES/DI and show either the armed
site IP or its exact successor IP; an unrelated stable drawer frame is a
typed failure. The before/after sample frames must match each other, and the
stopped successor must retain the armed ES/DI. A live adapter still needs a
source audit of when DOSBox-X advances `reg_eip` during the memory callback
before narrowing that two-value IP allowance or claiming decoder-PC parity.
The private adapter must pass the **actual final** plane index from the generic
unchained write after its odd/even remap or from the fast write's masked
index. The generic helper is also used by chained handlers, which must not be
silently labeled `uvga`.

The [offline checker](../../tools/replay/cold_start_codeword_row14_passive_snapshot.py)
accepts a supplied JSON record with `probe="host_backing_word"`,
`extent="one_byte_per_plane"`, the fixed witness `(122,112,A000:118F,bit 20)`,
one selected site/successor, measured `host_map`, actual
`handler_index`, `write_count=1`, opaque `plane_event_id`, bounded
`store_ordinal`, shared `arm_token`, and `before`/`after` samples. These join
fields allow p5's separate host event packet to refer to this plane event
without carrying EGA bytes or claiming RGB causation. Each sample has four
`planes` bytes encoded as eight uppercase hex digits and
`state_before_copy`/`state_after_copy` objects with four CPU WORDs, ten GC
bytes, six Sequencer bytes, and one latch DWORD. It requires equality around
each copy, resolves the index independently, and reports only changed-plane
count. This is one backing word, one byte per plane; it cannot satisfy the
older same-run event plan's four 320-byte windows per side. The record and its
state fields are private under ignored `resources/`.

The host packet's raw PIC ticks, CPU cycles, and frame-start value are from
its `latch_enter` phase. A later guest packet must not compare them for
equality with arm, store, or successor samples without a phase-specific
source proof. No emulated retrace port read is part of this probe.

Synthetic tests exercise latch/controller mutation, bank-dependent index
disagreement, route mismatch, and duplicate write rejection.

This validator cannot authenticate a self-reported private record or prove
that the hook captured the first actual writer. The later authorized native
slot must pin the patched source/binary hashes, show the debugger arm and
successor transcript, preserve the complete candidate stream, and compare a
separate uninstrumented run before any execution/timing parity claim. The
equal retained U19 page words `0/0` at `+02DD/+02E2/+02FD/+0302` do not
establish an RGB redraw; page/CRTC/retrace and host frame events are separate
follow-ons. No full-C or full-frame credit follows.

Run the focused synthetic gate without launching DOSBox-X:

```sh
python3 -m unittest tests.replay.test_cold_start_codeword_row14_passive_snapshot
c++ -std=c++17 -Wall -Wextra -Werror -I. \
  tests/replay/test_u19_row14_in_handler_capture.cpp \
  -o /tmp/poolrad_u19_in_handler_capture_test
/tmp/poolrad_u19_in_handler_capture_test
```
