# Intermediate FLAGS observations

Final CPU equality does not show that earlier arithmetic FLAGS were correct.
An ADD can produce wrong flags and a later CMP can overwrite all six before
return. A callback may consume the earlier flags even when a test double does
not. Compare the flags at that interface, before substituting the callback.

`tools/overlay/intermediate_flags.py` provides the shared check. An audit declares
named observation sites, records ordered 16-bit FLAGS from original and C, and
constructs a `Contract` from its passing baseline corpus. It then calls
`require_wrong_flags` for each site using a compiled candidate that flips
`0x08D5` immediately after the relevant arithmetic operation. `receipt()` refuses
success until every declared site has an observed wrong-FLAGS kill.

The mutation must run the complete baseline corpus with the same original
observations. A changed event order/count, compile error, crash, unrelated
flag change, or final-state mismatch is not a flag-specific kill. Empty corpora,
unreached declared sites, missing controls and surviving mutants fail. The
original and candidate FLAGS are compared exactly, with no masks. This helper
does not implement declared native/Unicorn undefined-flag splits.

## Choosing and wiring sites

For each arithmetic producer offered as verified, identify an observation point
after its flag write and before the next overwrite. A callback entry qualifies
only when the intervening instructions preserve those flags. If they do not,
add an earlier instruction watchpoint on the original and an equivalent explicit
C observation. Comparing a later CMP's flags does not cover the earlier ADD.
Multiple flag writers require distinct witnessed sites when one site cannot
observe all of them. This correspondence needs source and disassembly review;
the helper does not infer it or establish whole-body coverage.

```python
original = Trace(required_sites)
candidate = Trace(required_sites)
# Register before a hook that substitutes the callee or changes its CPU state.
handle = hook_unicorn(uc, original, {(body_cs, callback_ip): "add-before-dispatch"})
# First action of the C callback, before calling the double:
candidate.record("add-before-dispatch", cpu_ptr.contents.flags)
```

`hook_unicorn` checks logical CS:IP and physical address. An aliased address in
another segment is not a hit. `Trace.record` retains invalid-site/value and
event-limit errors rather than throwing inside a ctypes callback, where Python
exceptions can be swallowed. `checked`, `compare` and `Contract` surface those
errors outside the callback. Install and check both recorders for each case.
Keep the existing full final CPU/RAM, termination and callback-count comparison.

## The separate 3171 reproduction

`tools/overlay/check_intermediate_flags_3171.py` imports an explicitly supplied
candidate audit and builds its C model. It does not modify the author checkout.
The t135 reproduction uses PR #1564 head
`72c1625a5cc6d63fe618b378def13160dbecb3d4`; source/header/audit hashes and media
hashes are included in the output receipt. Original media stays under this
checkout's ignored `resources/DOS/POOLRAD/`.

The reviewed ADD is `001C:319A` (`ADD DI,AX`); its flags survive two MOVs, PUSH
and CALL to the dispatcher at `001C:2F2D`. The companion checks that instruction
sequence against the hash-checked body, installs an original-side hook before
the author's dispatcher substitution, and records the C flags before the same
double. The temporary mutant XORs the six arithmetic bits immediately after
the C `add16_flags` call. A successful run requires that the author's final-state
comparison still passes for every mutant case, while the new boundary comparison
kills the mutant. Thus an unrelated final-state failure cannot establish this
specific improvement.

```sh
../poolrad-heavy.sh python3 tools/overlay/check_intermediate_flags_3171.py \
  --candidate-root /path/to/candidate-checkout \
  --output resources/intermediate-flags-3171.json
```

The command writes INCOMPLETE before starting, FAIL on a handled error and PASS
only after baseline and control completion; a stale passing receipt cannot
survive an aborted rerun. The synthetic tests in `test_intermediate_flags.py`
are automatically discovered by `tools/run_media_checks.sh`. They use independently
specified flag values at carry, overflow, zero, parity and auxiliary-carry edges,
including ADD values that are erased by CMP before return.

The wrong-CS/same-address test follows a prior-session callback counterexample
found through Dunking Sheep memory, session
`01a1386e-bea7-7883-8ddd-06fd2be8db9b` (2026-10-16). Memory supplied the test idea;
the current synthetic execution supplies its verification.

## Scope and outstanding evidence

This is reusable harness infrastructure and a single draft-body reproduction.
It does not retrofit every existing checker or certify every arithmetic producer
in 3171. Its real dispatcher remains substituted. Native execution, natural game
reachability, real-dispatcher behavior and row acceptance remain unverified.
No credit or accounting change is proposed. Measured run results belong in the
exact-head PR receipt; the reproduction is independently rerunnable as above.
