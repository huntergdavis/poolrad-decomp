# Saved slot-A first page store: corrected native stop plan

This is a **plan for a future exclusive native slot**, not a new capture.
The first attempt did not produce a saved debugger transcript or a stop at
`0AF8:16A1`; no first-store instruction parity is claimed. The held
first-page-byte C candidate stays on hold.

Bounded `deja "Poolrad RES_0464 1D27 Move 0AF8:16A1 debugger breakpoint
RUN PTY source one byte watch saved slot A"` found no indexed session.
The plan reuses the merged
[all-dirty flush route](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md), the
[first flush caller frame](SLOT_A_RES464_SECOND_FLUSH.md), the accepted
local first-row watch table at `f057c56b503666efca06e37918915722f455911f`,
and the local failed-stop diagnosis at
`de1b5e8fcdb7a84f5eb04b1701aed098bdc892fb`. The latter is a source
and live-PTY diagnostic, not a hash-locked native stop. The conventional
RAM-delta classification is separate local head
`05d4404f040c2f6a7b818366773c3a33c9df1acd`.

## Fixed target and prior failure

Ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The resident post-stage file span `[0x63A0,0x642E)` hashes to
`83acb4eddfdaa0e6323c621a26c691bcef55e6ed7412e3cc8c3a678fbaf77f9a`;
its `+1DA0` far call targets runtime Move `0AF8:1685`.
The Move file span `[0xC2FB,0xC31E)` hashes to
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.
Its forward `REP MOVSB` is `0AF8:16A1`, after source/destination/count
loads, `CLD`, and an offset comparison.

The timed-out attempt armed `BP 0C88:1B52` first and later also armed
`BP 0AF8:16A1`. It paused at `1B52` twice, but the second pause was not
proved to be the same call or an immediate retrigger. The observed second
entry `SS:SP=1D16:3EBE` differs from the documented first entry
`1D16:3E94`. The ignored p5-002 launcher SHA-256 is
`f699a3ff3d6f156a7c7efada1d31bebca33a5bb8c878d9ce672eaf4ddc02003e`;
it exited under the 175-second cap with status 124. Its live PTY text was
not saved. The prior diagnosis pinned DOSBox-X `debug.cpp` SHA-256
`5f6ebeec1fbdb93cc41deb2050de80da8aeae70f5fa54cfb9144a4d359395d93`
and found that `RUN` executes one guest instruction before rearming
breakpoints. That source copy is absent from this checkout now; re-verify
it before any new launch.

## One serial attempt, only after PM grants a slot

1. Verify the original, emulator binary, launcher, and debugger-source
   hashes. Reserve one exclusive heavy emulator slot. Record the **entire
   initial debugger PTY**, including commands and stops, into ignored
   `resources/` before typing `RUN`; retain a bounded timeout and process
   cleanup. Use X11 input only for game navigation, not debugger commands.
2. At the initial debugger prompt, clear stale breakpoints, arm only
   `BP 0AF8:16A1`, confirm the sole code breakpoint with `BPLIST`, and type
   `RUN` in that same PTY. Navigate title, Load, and saved slot A. Do not
   re-arm `0C88:1B52`; avoid its repeated-entry pauses.
3. At each `16A1` hit, record `EV CS IP DS ES SS SP BP AX BX CX DX SI DI
   FLAGS` and `MEMDUMPBIN 0000 0000 A0000`, moving the generated private
   dump before the next command overwrites it. Classify the **live** frame:
   `DS:SI=A200:03C3`, `ES:DI=A000:03C3`, `CX=11`, DF clear, return from
   resident `+1DA0`, saved first-flush caller frame, and dirty row 16 in
   `14A0` video metadata. Earlier unrelated Move calls are not evidence
   for the saved first page. If classification fails, record and resume
   under the time cap.
4. Only at a classified pre-`REP` stop, capture current GC/write-mode and
   read-map state plus all four source and destination plane bytes under
   ignored `resources/`. Restore any debugger-modified read-map register
   before resuming. A code breakpoint after `REP` can compare the whole
   eleven-byte result, but it cannot by itself isolate one iteration.
5. A memory watch on `A000:03C3` is an **experiment**, not a proven
   first-byte stop: prior `BPM` observations detect value changes and may
   first report a pre-existing baseline. Record the watch's post-stop PC,
   `CX`, `SI`, `DI`, GC state, and four planes. Treat it as one-byte parity
   only if the observed CPU and memory state prove exactly one `MOVSB`
   iteration. A changed `CX=1` input is not original-path parity.

If the slot ends before a classified pre-`REP` and measured after-one-byte
state, report the exact stop, process result, transcript hash, and blocker.
Do not infer a saved first-page write from an unclassified `16A1` hit,
whole-`REP` return, or the existing entry-to-return plane equality.
No game bytes, RAM values, EGA bytes, or pixels belong in a public note.
