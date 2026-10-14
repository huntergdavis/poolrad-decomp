# Saved-A live key return to script dependency

`pr_slot_a_key_script_join` composes the existing
`pr_slot_a_key_return_step` and `pr_slot_a_key_gate_next`. The first callee's
new local key is the second callee's input; the host cannot accidentally
provide a stale, independent gate key. On the measured Up slice the join
returns **BLOCKED_SCRIPT_3171**, at unit `0x1C:+0x3458`, before executing
the call to `+0x3171`. It emits a typed dependency containing unit, call PC,
callee PC, return PC, and the live script-entry argument.

Before implementation, `deja "poolrad saved slot A replay join native-save
adapter key dispatch blocker"` was started. It remained blocked in
`locks_lock_inode_wait` for more than six minutes. An MCP recall for
`poolrad saved slot A key return 339A 33BD replay composition` also did not
return during implementation. No prior-session result was reused. Repository
evidence reused and checked: [return bridge](SLOT_A_KEY_RETURN_BRIDGE.md),
[same-invocation native CPU/RAM stops](SLOT_A_NATIVE_KEY_CALL_RETURN.md),
[Up gate and separate script frame](SLOT_A_KEY_GATE_TO_SCRIPT.md),
[live script binder](SLOT_A_ECL_LIVE_BIND.md), and
[raw save adapter](SLOT_A_NATIVE_SAVE_ADAPTER.md).

## Measured comparison

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Return `+339A` RAM | 655360 | `5ab1d3fa57949ce7c3e2b4dbd0ff1d6d2f96787f0dd13bc8cbc10b44713bda37` |
| Gate `+33BD` RAM, same invocation | 655360 | `38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3` |
| Script `+3171` RAM, separate invocation | 655360 | `f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558` |

The harness constructs both inputs from the return RAM, supplies the observed
return-register low byte (`AX=A048`, hence `AL=48`), and runs the new join.
It marshals the returned local key and copied far-pointer words into that
RAM; window 0 already aliases it. **All 655360 bytes then equal the native
gate dump.** Exactly five bytes changed: local `SS:3EF7` and
`DS:4436..4439`. The map-word store leaves its existing value unchanged.
No party coordinates or clock values are assigned by the join or harness.

CPU provenance is the previously captured `CS:IP=3582:339A`, `DS=14A0`,
`SS:SP=1D16:3EF6`, `BP=3EF8`, `AX=A048`. The next native gate has
`ES:DI=3A16:0000`, agreeing with the window-1 pointer used by the query.
These registers establish the live input boundary; this join does not
emulate registers, flags, stack writes, or runtime UpCase execution.
The full-RAM equality applies specifically to return-to-gate marshalling,
not to the gate-to-script instruction interval.

The join yields `(unit, call, callee, return, entry) =
(001C,3458,3171,345B,9914)`. The separate script dump has the matching
stack triple at `SS:3EF0`: return `345B`, code segment `3582`, argument
`9914`. Its local key, exit flag, entry field, and entire 2048-byte window 1
also match the join inputs/outputs. The snapshots come from two native runs;
this is a dependency-contract comparison, not a same-invocation transition
or proof that the script returned.

## Contract and first blocker

The caller must supply live returned AL, active/copy pointers, map ID,
windows, exit flag, and script entry from one invocation. The raw ten save
transfers do not provide that environment: character/sidecar loading,
gameplay initialization, and the actual input callee still precede it.
`pr_slot_a_native_one_up` therefore remains `BLOCKED_KEY_DISPATCH`.

The downstream pC contract is the reached `+3458 -> +3171` call with the
dependency's entry argument and return PC. Continuing requires the matching
live script, windows, record, and handler state. The existing
`pr_slot_a_ecl_bind_ram` and `pr_rec_ecl_run` are available; their rejection,
`UNMODELLED`, and `HANDLER` outcomes must be carried forward when wired.
No callback is fabricated and no script is executed by this join.

Null top-level arguments have no effects. Return-step rejection produces
no return effects. A later gate rejection retains the preceding return
effects, invalidates the dependency, and reports the gate's start boundary
`+33BD`; it does not invent a reached instruction for an unsupported route.

## Adversarial parity review

- `target_function`: bounded state composition, `0x1C:+339A` through the Up
  gate's call dependency at `+3458`.
- `source_candidates`: new `slot_a_key_script_join.c` composing the two
  merged functions in `slot_a_key_dispatch_bridge.c`.
- `signals`: hash-locked original/loaded spans; observed returned AL;
  automatic local-key propagation; complete return-to-gate RAM equality;
  independent script-frame argument and return-PC agreement.
- `score`: 5 supporting signal categories; no numeric probability implied.
- `verdict`: `match` for the tested return-to-gate memory projection and
  Up dependency; `unclear` for complete gate-to-script CPU/RAM parity.
- `review_notes`: one measured Up invocation; script stop from another run;
  no register/stack emulator; no native post-script comparison. Full saved
  one-Up parity and strict game-routine/identity counts do not advance.

## Verification

`bash tests/replay/check_slot_a_key_script_join.sh` runs strict C11 synthetic
tests for key propagation, outside-slice inputs, rejected windows, dependency
invalidation, and preservation of earlier effects after a later rejection.
With five ignored paths (`GAME.OVR SAVGAMA.DAT RETURN_RAM GATE_RAM SCRIPT_RAM`)
it also reuses the existing source/snapshot hash audits and runs the native
comparison above. The raw-save blocker regression is
`bash tests/replay/check_slot_a_native_save.sh`.

No shared CMake edit is included; registration is coordinated with the
integration owner. Only new replay source, header, tests, and this evidence
note are added. Captures and build outputs remain under ignored `resources/`.
